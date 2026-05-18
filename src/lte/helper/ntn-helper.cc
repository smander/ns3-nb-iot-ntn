#include "ntn-helper.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/mobility-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"

// NTN module includes
#include "ns3/leo-satellite-mobility-model.h"
#include "ns3/nbiot-telemetry-app.h"
#include "ns3/flood-attack-app.h"
#include "ns3/c2-beacon-app.h"
#include "ns3/rach-flood-app.h"
#include "ns3/spoofing-app.h"
#include "ns3/exfiltration-app.h"
#include "ns3/ids-feature-tracer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NtnHelper");

NtnHelper::NtnHelper () {}
NtnHelper::~NtnHelper () {}

void
NtnHelper::InstallSatellite (Ptr<Node> node, double altitudeKm,
                              double inclinationDeg, double raanDeg)
{
    Ptr<LeoSatelliteMobilityModel> mobility = CreateObject<LeoSatelliteMobilityModel> ();
    mobility->SetAttribute ("Altitude", DoubleValue (altitudeKm));
    mobility->SetAttribute ("Inclination", DoubleValue (inclinationDeg));
    mobility->SetAttribute ("RAAN", DoubleValue (raanDeg));
    node->AggregateObject (mobility);
    NS_LOG_INFO ("Satellite installed: alt=" << altitudeKm << "km, inc=" << inclinationDeg << "deg");
}

void
NtnHelper::InstallUes (NodeContainer nodes, double radiusKm)
{
    Ptr<UniformRandomVariable> xRng = CreateObject<UniformRandomVariable> ();
    Ptr<UniformRandomVariable> yRng = CreateObject<UniformRandomVariable> ();
    xRng->SetAttribute ("Min", DoubleValue (-radiusKm));
    xRng->SetAttribute ("Max", DoubleValue (radiusKm));
    yRng->SetAttribute ("Min", DoubleValue (-radiusKm));
    yRng->SetAttribute ("Max", DoubleValue (radiusKm));

    for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
        double x = xRng->GetValue ();
        double y = yRng->GetValue ();
        // z = Earth radius (UE is on surface)
        double z = 6371.0; // EARTH_RADIUS_KM

        Ptr<ConstantPositionMobilityModel> mobility =
            CreateObject<ConstantPositionMobilityModel> ();
        mobility->SetPosition (Vector (x, y, z));
        nodes.Get (i)->AggregateObject (mobility);
    }
    NS_LOG_INFO ("Installed " << nodes.GetN () << " UEs within " << radiusKm << "km radius");
}

void
NtnHelper::InstallTelemetryApp (NodeContainer nodes, Ipv4Address serverAddr,
                                 double intervalSec, uint32_t payloadSize)
{
    for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
        Ptr<NbIotTelemetryApp> app = CreateObject<NbIotTelemetryApp> ();
        app->SetAttribute ("Interval", DoubleValue (intervalSec));
        app->SetAttribute ("PayloadSize", UintegerValue (payloadSize));
        app->SetRemote (serverAddr, 5683);
        nodes.Get (i)->AddApplication (app);
    }
}

void
NtnHelper::InstallAttackApp (Ptr<Node> node, uint8_t attackType, Ipv4Address serverAddr)
{
    switch (attackType)
    {
        case 1: { // Flood
            Ptr<FloodAttackApp> app = CreateObject<FloodAttackApp> ();
            app->SetRemote (serverAddr);
            node->AddApplication (app);
            break;
        }
        case 2: { // C2 Beacon
            Ptr<C2BeaconApp> app = CreateObject<C2BeaconApp> ();
            app->SetRemote (serverAddr, 4444);
            node->AddApplication (app);
            break;
        }
        case 3: { // RACH Flood
            Ptr<RachFloodApp> app = CreateObject<RachFloodApp> ();
            app->SetRemote (serverAddr, 5683);
            node->AddApplication (app);
            break;
        }
        case 4: { // Spoofing
            Ptr<SpoofingApp> app = CreateObject<SpoofingApp> ();
            app->SetRemote (serverAddr, 5683);
            node->AddApplication (app);
            break;
        }
        case 5: { // Exfiltration
            Ptr<ExfiltrationApp> app = CreateObject<ExfiltrationApp> ();
            app->SetRemote (serverAddr, 8443);
            node->AddApplication (app);
            break;
        }
        default:
            NS_LOG_WARN ("Unknown attack type: " << (int)attackType);
    }
}

void
NtnHelper::EnableIdsTracing (NodeContainer nodes, std::string outputPath)
{
    // Note: full usage requires NetDeviceContainer — use IdsFeatureTracer directly
    // This helper is a convenience stub
    NS_LOG_INFO ("IDS tracing: use IdsFeatureTracer::Install(nodes, devs, path) directly");
}

} // namespace ns3
