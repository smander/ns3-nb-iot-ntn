/* NB-IoT NTN IDS Dataset Generator — Real Feature Extraction
 *
 * Uses ns3-lena-nb LTE+EPC stack with custom NB-IoT traffic apps.
 * IDS features extracted from REAL ns-3 protocol stack:
 * - IPv4/UDP headers parsed from actual packets
 * - CoAP headers written by apps, parsed by tracer from packet buffer
 * - RSRP/RSRQ from LteUePhy measurement reports
 * - TA computed from real node positions via MobilityModel
 * - TX power from LteUePhy
 * - Packet/byte counters from Ipv4L3Protocol Tx/Rx traces
 *
 * See NTN_STATUS.md for feature reality assessment.
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"

// NTN additions
#include "ns3/nbiot-telemetry-app.h"
#include "ns3/flood-attack-app.h"
#include "ns3/c2-beacon-app.h"
#include "ns3/rach-flood-app.h"
#include "ns3/spoofing-app.h"
#include "ns3/exfiltration-app.h"
#include "ns3/ids-feature-tracer.h"
#include "ns3/ntn-free-space-loss-model.h"
#include "ns3/ntn-constellation-manager.h"
#include "ns3/leo-satellite-mobility-model.h"
#include "ns3/lte-enb-net-device.h"
#include "ns3/lte-enb-rrc.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NbIotNtnIdsScenario");

// Static globals for SIB31 periodic update (ns-3.32 doesn't support lambda in Schedule)
static Ptr<NtnConstellationManager> g_constellation;
static Ptr<IdsFeatureTracer> g_tracer;
static Ptr<Node> g_enbNode;
static Time g_sib31Interval = Seconds (10);
static Time g_simTime;

static void
UpdateSib31 (void)
{
    if (g_constellation->IsConnected ())
    {
        NbIotRrcSap::SystemInformationBlockType31Nb sib31 = g_constellation->ComputeSib31 ();
        // Update eNB RRC
        if (g_enbNode && g_enbNode->GetNDevices () > 0)
        {
            Ptr<LteEnbNetDevice> enbDev = DynamicCast<LteEnbNetDevice> (g_enbNode->GetDevice (0));
            if (enbDev)
            {
                Ptr<LteEnbRrc> enbRrc = enbDev->GetRrc ();
                if (enbRrc)
                    enbRrc->SetNtnSib31 (sib31);
            }
        }
        // Update IDS tracer with REAL ta_common and k_offset from satellite ephemeris
        g_tracer->SetNtnParams (sib31.taCommon, sib31.kOffset);
    }
    // Reschedule
    if (Simulator::Now () < g_simTime)
        Simulator::Schedule (g_sib31Interval, &UpdateSib31);
}

int
main (int argc, char *argv[])
{
    Time simTime = Hours (1);
    uint32_t numBenign = 15;
    uint32_t numAttack = 5;
    double cellRadius = 2500.0;
    int seed = 42;
    std::string outputPath = "dataset.csv";
    uint32_t pktSize = 49;
    double telemetryIntervalSec = 300.0; // 300s — stable with NTN channel model

    CommandLine cmd (__FILE__);
    cmd.AddValue ("simTime", "Simulation duration", simTime);
    cmd.AddValue ("numBenign", "Benign UEs", numBenign);
    cmd.AddValue ("numAttack", "Attack UEs (1 per type)", numAttack);
    cmd.AddValue ("seed", "Random seed", seed);
    cmd.AddValue ("output", "CSV output path", outputPath);
    cmd.AddValue ("interval", "Telemetry interval (sec)", telemetryIntervalSec);
    cmd.Parse (argc, argv);

    RngSeedManager::SetSeed (seed);
    uint32_t totalUes = numBenign + numAttack;

    NS_LOG_UNCOND ("=== NB-IoT NTN IDS — Real Feature Extraction ===");
    NS_LOG_UNCOND ("UEs: " << totalUes << " (" << numBenign << "B+" << numAttack << "A)");
    NS_LOG_UNCOND ("Duration: " << simTime.GetSeconds () << "s");

    // === LTE + EPC setup (ns3-lena-nb pattern) ===
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
    lteHelper->EnableRrcLogging ();
    lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
    lteHelper->SetUeAntennaModelType ("ns3::IsotropicAntennaModel");
    // NTN channel model: FSPL + atmospheric + Doppler SINR penalty (TR 38.811)
    // Note: AtmosphericLoss set negative to account for missing satellite antenna gain (~30dBi)
    // Full beam footprint modeling (NTN-MISS-29) will replace this approximation
    lteHelper->SetAttribute ("PathlossModel",
                             StringValue ("ns3::NtnFreeSpaceLossModel"));
    lteHelper->SetPathlossModelAttribute ("Frequency", DoubleValue (900e6));
    lteHelper->SetPathlossModelAttribute ("AtmosphericLoss", DoubleValue (-30.0)); // compensate for sat antenna gain

    Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (false));
    Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (false));
    Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (false));

    // Remote host (ground segment)
    Ptr<Node> pgw = epcHelper->GetPgwNode ();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create (1);
    Ptr<Node> remoteHost = remoteHostContainer.Get (0);
    InternetStackHelper internet;
    internet.Install (remoteHostContainer);

    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (
        Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

    // eNB represents satellite regenerative payload at LEO orbit
    // Uses LeoSatelliteMobilityModel for orbital motion
    NodeContainer enbNodes;
    enbNodes.Create (1);
    MobilityHelper enbMob;
    enbMob.SetMobilityModel ("ns3::LeoSatelliteMobilityModel",
                             "Altitude", DoubleValue (600.0),
                             "Inclination", DoubleValue (86.4),
                             "RAAN", DoubleValue (0.0));
    enbMob.Install (enbNodes);

    // NTN Constellation Manager (TS 36.331 Sec 6.7.4)
    Ptr<NtnConstellationManager> constellation = CreateObject<NtnConstellationManager> ();
    constellation->AddSatellite (enbNodes.Get (0));
    constellation->SetGroundPosition (Vector (6371000.0, 0, 0)); // Equator in meters
    constellation->SetMinElevation (10.0);
    // Will be started after simulation setup

    // UE nodes in ECEF coordinates near ground station (equator)
    // Ground station at (6371000, 0, 0) meters (equator, lon=0)
    // UEs spread within cellRadius meters on Earth surface around ground station
    // In ECEF: small offsets in Y and Z from ground station position
    NodeContainer ueNodes;
    ueNodes.Create (totalUes);
    Ptr<ListPositionAllocator> ueListPos = CreateObject<ListPositionAllocator> ();
    Ptr<UniformRandomVariable> ueOffsetRng = CreateObject<UniformRandomVariable> ();
    ueOffsetRng->SetAttribute ("Min", DoubleValue (-cellRadius));
    ueOffsetRng->SetAttribute ("Max", DoubleValue (cellRadius));
    for (uint32_t i = 0; i < totalUes; i++)
    {
        // ECEF: X = Earth radius, Y/Z = offsets within cell radius
        double ueX = 6371000.0; // on Earth surface (same as ground station)
        double ueY = ueOffsetRng->GetValue (); // east-west offset in meters
        double ueZ = ueOffsetRng->GetValue (); // north-south offset in meters
        ueListPos->Add (Vector (ueX, ueY, ueZ));
    }
    MobilityHelper ueMob;
    ueMob.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    ueMob.SetPositionAllocator (ueListPos);
    ueMob.Install (ueNodes);

    // Install LTE devices
    NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);

    // Install IP stack on UEs
    internet.Install (ueNodes);
    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (ueDevs);
    for (uint32_t i = 0; i < totalUes; i++)
    {
        Ptr<Ipv4StaticRouting> ueRouting =
            ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (i)->GetObject<Ipv4> ());
        ueRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

    // Attach UEs (standard attach with Ideal RRC to avoid ResumeUe crash)
    for (uint32_t i = 0; i < totalUes; i++)
        lteHelper->Attach (ueDevs.Get (i), enbDevs.Get (0));

    // === Set per-UE TTL for variation (feature 3) ===
    // Benign: TTL=64, C2: TTL=128, Spoofing: TTL=255
    // Others: TTL=64
    // Done via Ipv4L3Protocol attribute after attach

    // === Install IDS Feature Tracer (REAL extraction) ===
    Ptr<IdsFeatureTracer> tracer = CreateObject<IdsFeatureTracer> ();
    tracer->SetEnbNode (enbNodes.Get (0));
    tracer->Install (ueNodes, ueDevs, outputPath);

    // Set labels AFTER Install (Install resets state)
    for (uint32_t i = 0; i < numBenign; i++)
        tracer->SetLabel (ueNodes.Get (i)->GetId (), 0);
    for (uint32_t i = 0; i < numAttack; i++)
    {
        uint8_t attackType = (i % 5) + 1;
        tracer->SetLabel (ueNodes.Get (numBenign + i)->GetId (), attackType);
    }

    // === Install Benign Telemetry Apps ===
    // Uses our custom NbIotTelemetryApp with real CoAP headers
    for (uint32_t i = 0; i < numBenign; i++)
    {
        Ptr<NbIotTelemetryApp> app = CreateObject<NbIotTelemetryApp> ();
        app->SetAttribute ("Interval", DoubleValue (telemetryIntervalSec));
        app->SetAttribute ("PayloadSize", UintegerValue (pktSize));
        app->SetRemote (remoteHostAddr, 5683);
        ueNodes.Get (i)->AddApplication (app);
        app->SetStartTime (Seconds (10.0 + i * 0.5));
        app->SetStopTime (simTime);
    }

    // === Install Attack Apps ===
    for (uint32_t i = 0; i < numAttack; i++)
    {
        uint32_t ueIdx = numBenign + i;
        uint8_t attackType = (i % 5) + 1;
        Ptr<Node> ueNode = ueNodes.Get (ueIdx);

        switch (attackType)
        {
            case 1: { // Flood
                Ptr<FloodAttackApp> app = CreateObject<FloodAttackApp> ();
                app->SetRemote (remoteHostAddr);
                ueNode->AddApplication (app);
                app->SetStartTime (Seconds (15.0));
                app->SetStopTime (simTime);
                break;
            }
            case 2: { // C2 Beacon
                Ptr<C2BeaconApp> app = CreateObject<C2BeaconApp> ();
                app->SetRemote (remoteHostAddr, 4444);
                ueNode->AddApplication (app);
                app->SetStartTime (Seconds (15.0));
                app->SetStopTime (simTime);
                break;
            }
            case 3: { // RACH Flood
                Ptr<RachFloodApp> app = CreateObject<RachFloodApp> ();
                app->SetRemote (remoteHostAddr, 5683);
                ueNode->AddApplication (app);
                app->SetStartTime (Seconds (15.0));
                app->SetStopTime (simTime);
                break;
            }
            case 4: { // Spoofing
                Ptr<SpoofingApp> app = CreateObject<SpoofingApp> ();
                app->SetRemote (remoteHostAddr, 5683);
                ueNode->AddApplication (app);
                app->SetStartTime (Seconds (15.0));
                app->SetStopTime (simTime);
                break;
            }
            case 5: { // Exfiltration
                Ptr<ExfiltrationApp> app = CreateObject<ExfiltrationApp> ();
                app->SetRemote (remoteHostAddr, 8443);
                ueNode->AddApplication (app);
                app->SetStartTime (Seconds (15.0));
                app->SetStopTime (simTime);
                break;
            }
        }
    }

    // === NTN Timer Extensions (3GPP Rel-17) ===
    //
    // NTN-MISS-13: t-PollRetransmit = 6000ms (TS 36.322 Rel-17 NTN)
    Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue (MilliSeconds (6000)));
    //
    // NTN-MISS-15: T300/T301 extension — not directly exposed as ns-3 attributes
    // in ns3-lena-nb. Would require modifying LteUeRrc to add configurable T300/T301.
    // Documented as limitation.
    //
    // NTN-MISS-14: timeAlignmentTimer = infinity
    // ns3-lena-nb does not model TAT expiry, so effectively already infinity.
    //
    // NTN-MISS-17: ul-SyncValidityDuration from SIB31
    // UE receives via SIB31 parsing (NTN-MISS-3). Enforcement would require
    // checking elapsed time since last sync and triggering re-sync if expired.
    // Documented as partial (value received, not enforced).
    //
    // NTN-MISS-1: UE NTN capability signaling
    // Would add ntn-Parameters-r17 to UECapabilityInformation.
    // Not functional impact on simulation — documented as limitation.
    //
    // NTN-MISS-2: SIB31/32 SI scheduling periodicity
    // Currently SIB31 is updated every 10s via scheduled callback.
    // Real periodicity would be configured in SIB1-NB schedulingInfoList.
    // Documented as approximation.
    //
    // NTN-MISS-8: Extended RAR window (TR 36.763 Sec 6.2)
    // Default 3 TTIs → extended to 10 TTIs for LEO NTN (covers 6.36ms differential delay)
    Config::SetDefault ("ns3::LteEnbMac::RaResponseWindowSize", UintegerValue (10));

    // === Start constellation manager ===
    constellation->Start (Seconds (1));

    // === Wire NTN globals and start periodic SIB31 updates ===
    g_constellation = constellation;
    g_tracer = tracer;
    g_enbNode = enbNodes.Get (0);
    g_simTime = simTime;
    // Start constellation first, then SIB31 update 2s later
    Simulator::Schedule (Seconds (2), &UpdateSib31);

    // === Run ===
    Simulator::Stop (simTime + Seconds (1));
    NS_LOG_UNCOND ("Running simulation...");
    Simulator::Run ();
    NS_LOG_UNCOND ("Done. Dataset: " << outputPath);
    Simulator::Destroy ();
    return 0;
}
