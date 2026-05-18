#include "ids-feature-tracer.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/mobility-model.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/lte-ue-phy.h"
#include "ns3/lte-ue-mac.h"
#include "ns3/lte-ue-rrc.h"
#include "ns3/lte-enb-net-device.h"
#include "ns3/config.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IdsFeatureTracer");
NS_OBJECT_ENSURE_REGISTERED (IdsFeatureTracer);

static const double C_M_S = 299792458.0;
static const double TS_SEC = 1.0 / (15000.0 * 2048.0);

TypeId IdsFeatureTracer::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::IdsFeatureTracer")
        .SetParent<Object> ()
        .SetGroupName ("Lte")
        .AddConstructor<IdsFeatureTracer> ();
    return tid;
}

IdsFeatureTracer::IdsFeatureTracer ()
    : m_windowSec (10.0), m_ntnTaCommon (0), m_ntnKOffset (20), m_lastRachNodeId (0)
{}

IdsFeatureTracer::~IdsFeatureTracer ()
{
    // Flush remaining windows on destruction
    for (auto &pair : m_ueStates)
    {
        if (pair.second.pktCount > 0)
            FlushWindow (pair.first);
    }
    if (m_outFile.is_open ()) m_outFile.close ();
}

void IdsFeatureTracer::SetEnbNode (Ptr<Node> enbNode) { m_enbNode = enbNode; }

void IdsFeatureTracer::SetLabel (uint32_t nodeId, uint8_t label)
{
    if (m_ueStates.find (nodeId) != m_ueStates.end ())
        m_ueStates[nodeId].label = label;
}

void IdsFeatureTracer::SetNtnParams (int32_t taCommon, uint16_t kOffset)
{
    m_ntnTaCommon = taCommon;
    m_ntnKOffset = kOffset;
}

void
IdsFeatureTracer::Install (NodeContainer ueNodes, NetDeviceContainer ueDevs,
                           std::string outputPath, double windowSec)
{
    m_windowSec = windowSec;
    m_ueNodes = ueNodes;
    m_ueDevs = ueDevs;

    m_outFile.open (outputPath);
    m_outFile << "ip_payload_length,ip_next_header,ip_hop_limit,"
              << "udp_src_port,udp_dst_port,udp_length,"
              << "coap_type,coap_code,coap_tkl,direction,"
              << "nrsrp,nrsrq,ce_level,timing_advance,tx_power,"
              << "ta_common,k_offset,"
              << "window_pkt_count,window_byte_count,window_conn_attempts,"
              << "label" << std::endl;

    for (uint32_t i = 0; i < ueNodes.GetN (); i++)
    {
        Ptr<Node> node = ueNodes.Get (i);
        uint32_t nodeId = node->GetId ();

        // Init window state
        PerUeState st;
        std::memset (&st, 0, sizeof (st));
        st.windowStart = 0.0;
        st.minPayloadLen = 65535;
        st.rsrpDbm = -110.0;
        st.rsrqDb = -15.0;
        st.label = 0;
        m_ueStates[nodeId] = st;

        // Hook Ipv4 Tx/Rx
        Ptr<Ipv4L3Protocol> ipv4 = node->GetObject<Ipv4L3Protocol> ();
        if (ipv4)
        {
            ipv4->TraceConnectWithoutContext (
                "Tx", MakeCallback (&IdsFeatureTracer::Ipv4TxCallback, this));
            ipv4->TraceConnectWithoutContext (
                "Rx", MakeCallback (&IdsFeatureTracer::Ipv4RxCallback, this));
        }

        // Hook PHY measurements
        Ptr<LteUeNetDevice> ueDev = DynamicCast<LteUeNetDevice> (ueDevs.Get (i));
        if (ueDev)
        {
            Ptr<LteUePhy> uePhy = ueDev->GetPhy ();
            if (uePhy)
                uePhy->TraceConnectWithoutContext (
                    "ReportUeMeasurements",
                    MakeCallback (&IdsFeatureTracer::MeasurementCallback, this));

            Ptr<LteUeMac> ueMac = ueDev->GetMac ();
            if (ueMac)
                ueMac->TraceConnectWithoutContext (
                    "RachAttempt",
                    MakeCallback (&IdsFeatureTracer::RachCallback, this));
        }
    }

    // Schedule periodic window flush (ensures rows are written even for quiet UEs)
    ScheduleWindowFlush ();

    NS_LOG_INFO ("IdsFeatureTracer: window-level, " << ueNodes.GetN ()
                 << " UEs, window=" << windowSec << "s");
}

void
IdsFeatureTracer::ScheduleWindowFlush ()
{
    Simulator::Schedule (Seconds (m_windowSec), &IdsFeatureTracer::FlushAllWindows, this);
}

void
IdsFeatureTracer::FlushAllWindows ()
{
    // Write one row per UE for the expired window, then reset
    // All pointer access in FlushWindow is already null-guarded
    double now = Simulator::Now ().GetSeconds ();
    for (auto &pair : m_ueStates)
    {
        if (now - pair.second.windowStart >= m_windowSec)
        {
            FlushWindow (pair.first);
            ResetWindow (pair.first);
        }
    }
    ScheduleWindowFlush ();
}

void
IdsFeatureTracer::RachCallback (uint8_t ceLevel)
{
    for (auto &pair : m_ueStates)
        pair.second.connAttempts++;
}

void
IdsFeatureTracer::Ipv4TxCallback (Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t iface)
{
    Ptr<Node> node = ipv4->GetObject<Node> ();
    if (!node) return;
    uint32_t nodeId = node->GetId ();
    if (m_ueStates.find (nodeId) == m_ueStates.end ()) return;
    ProcessPacket (packet, nodeId, 0);
}

void
IdsFeatureTracer::Ipv4RxCallback (Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t iface)
{
    Ptr<Node> node = ipv4->GetObject<Node> ();
    if (!node) return;
    uint32_t nodeId = node->GetId ();
    if (m_ueStates.find (nodeId) == m_ueStates.end ()) return;
    ProcessPacket (packet, nodeId, 1);
}

void
IdsFeatureTracer::ProcessPacket (Ptr<const Packet> packet, uint32_t nodeId, uint8_t direction)
{
    PerUeState &ue = m_ueStates[nodeId];

    // Lazy window flush: if window expired, write row and reset
    double now = Simulator::Now ().GetSeconds ();
    if (ue.pktCount > 0 && (now - ue.windowStart >= m_windowSec))
    {
        FlushWindow (nodeId);
        ResetWindow (nodeId);
    }

    // Parse IPv4 header
    Ipv4Header ipv4Hdr;
    Ptr<Packet> copy = packet->Copy ();
    uint32_t removed = copy->RemoveHeader (ipv4Hdr);
    if (removed == 0) return;

    uint16_t payloadLen = ipv4Hdr.GetPayloadSize ();
    uint8_t protocol = ipv4Hdr.GetProtocol ();
    uint8_t hopLimit = ipv4Hdr.GetTtl ();

    // Accumulate into window
    ue.pktCount++;
    ue.byteCount += payloadLen;
    if (payloadLen > ue.maxPayloadLen) ue.maxPayloadLen = payloadLen;
    if (payloadLen < ue.minPayloadLen) ue.minPayloadLen = payloadLen;
    if (hopLimit < ue.minHopLimit || ue.minHopLimit == 0) ue.minHopLimit = hopLimit;

    if (direction == 0) ue.ulCount++;
    else ue.dlCount++;

    if (protocol == 17) ue.udpCount++;
    else if (protocol == 1) ue.icmpCount++;

    // Parse UDP + CoAP if present
    if (protocol == 17)
    {
        UdpHeader udpHdr;
        copy->RemoveHeader (udpHdr);
        ue.lastSrcPort = udpHdr.GetSourcePort ();
        ue.lastDstPort = udpHdr.GetDestinationPort ();

        // Track unique dst ports (simple linear search, max 32)
        bool found = false;
        for (uint8_t p = 0; p < ue.seenPortCount && p < 32; p++)
        {
            if (ue.seenPorts[p] == ue.lastDstPort) { found = true; break; }
        }
        if (!found && ue.seenPortCount < 32)
        {
            ue.seenPorts[ue.seenPortCount++] = ue.lastDstPort;
        }
        ue.uniqueDstPorts = ue.seenPortCount;

        // Parse CoAP
        if (copy->GetSize () >= 4)
        {
            uint8_t coapBuf[4];
            copy->CopyData (coapBuf, 4);
            uint8_t coapType = (coapBuf[0] >> 4) & 0x03;
            ue.dominantCoapCode = coapBuf[1];
            ue.dominantCoapTkl = coapBuf[0] & 0x0F;
            if (coapType == 0) ue.coapConCount++;
            else ue.coapNonCount++;
        }
    }
}

void
IdsFeatureTracer::FlushWindow (uint32_t nodeId)
{
    PerUeState &ue = m_ueStates[nodeId];

    // Skip empty windows (UE didn't send/receive anything)
    if (ue.pktCount == 0) return;

    // Compute aggregated features

    // Feature 1: max payload length in window
    uint16_t ipPayloadLength = ue.maxPayloadLen;
    // Feature 2: dominant protocol
    uint8_t ipNextHeader = (ue.udpCount >= ue.icmpCount) ? 17 : 1;
    // Feature 3: min hop limit (anomalous if low)
    uint8_t ipHopLimit = ue.minHopLimit;
    // Feature 4-5: last seen ports
    uint16_t udpSrcPort = ue.lastSrcPort;
    uint16_t udpDstPort = ue.lastDstPort;
    // Feature 6: total UDP bytes
    uint16_t udpLength = (uint16_t)std::min ((uint32_t)65535, ue.byteCount);
    // Feature 7: dominant CoAP type
    uint8_t coapType = (ue.coapConCount > ue.coapNonCount) ? 0 : 1;
    // Feature 8-9: last seen CoAP code and TKL
    uint8_t coapCode = ue.dominantCoapCode;
    uint8_t coapTkl = ue.dominantCoapTkl;
    // Feature 10: dominant direction
    uint8_t direction = (ue.ulCount >= ue.dlCount) ? 0 : 1;

    // Feature 11-12: RSRP/RSRQ from PHY
    uint8_t nrsrpIdx = (uint8_t)std::max (0.0, std::min (97.0, ue.rsrpDbm + 156.0));
    uint8_t nrsrqIdx = (uint8_t)std::max (0.0, std::min (34.0, ue.rsrqDb + 30.0));

    // Feature 13: CE level
    uint8_t ceLevel = 0;
    if (ue.rsrpDbm < -120.0) ceLevel = 2;
    else if (ue.rsrpDbm < -110.0) ceLevel = 1;

    // Feature 14: residual TA
    uint16_t ta = 0;
    for (uint32_t i = 0; i < m_ueNodes.GetN (); i++)
    {
        if (m_ueNodes.Get (i)->GetId () != nodeId) continue;
        Ptr<MobilityModel> ueMob = m_ueNodes.Get (i)->GetObject<MobilityModel> ();
        Ptr<MobilityModel> enbMob;
        if (m_enbNode) enbMob = m_enbNode->GetObject<MobilityModel> ();
        if (ueMob && enbMob)
        {
            double dist = ueMob->GetDistanceFrom (enbMob);
            double fullTaTs = 2.0 * dist / C_M_S / TS_SEC;
            double residualTa = fullTaTs - (double)m_ntnTaCommon;
            ta = (uint16_t)std::min (1282.0, std::max (0.0, std::abs (residualTa)));
        }
        break;
    }

    // Feature 15: tx power
    int8_t txPower = 14;
    for (uint32_t i = 0; i < m_ueDevs.GetN (); i++)
    {
        Ptr<LteUeNetDevice> ueDev = DynamicCast<LteUeNetDevice> (m_ueDevs.Get (i));
        if (!ueDev) continue;
        Ptr<Node> devNode = ueDev->GetNode ();
        if (!devNode || devNode->GetId () != nodeId) continue;
        Ptr<LteUePhy> uePhy = ueDev->GetPhy ();
        if (uePhy) txPower = (int8_t)uePhy->GetTxPower ();
        break;
    }

    // Write ONE row for this UE's window
    m_outFile << ipPayloadLength << ","
              << (int)ipNextHeader << ","
              << (int)ipHopLimit << ","
              << udpSrcPort << ","
              << udpDstPort << ","
              << udpLength << ","
              << (int)coapType << ","
              << (int)coapCode << ","
              << (int)coapTkl << ","
              << (int)direction << ","
              << (int)nrsrpIdx << ","
              << (int)nrsrqIdx << ","
              << (int)ceLevel << ","
              << ta << ","
              << (int)txPower << ","
              << m_ntnTaCommon << ","
              << m_ntnKOffset << ","
              << ue.pktCount << ","
              << ue.byteCount << ","
              << ue.connAttempts << ","
              << (int)ue.label
              << std::endl;
}

void
IdsFeatureTracer::ResetWindow (uint32_t nodeId)
{
    PerUeState &ue = m_ueStates[nodeId];
    uint8_t label = ue.label;
    double rsrp = ue.rsrpDbm;
    double rsrq = ue.rsrqDb;
    std::memset (&ue, 0, sizeof (ue));
    ue.windowStart = Simulator::Now ().GetSeconds ();
    ue.minPayloadLen = 65535;
    ue.label = label;
    ue.rsrpDbm = rsrp;
    ue.rsrqDb = rsrq;
}

void
IdsFeatureTracer::MeasurementCallback (uint16_t rnti, uint16_t cellId,
                                        double rsrp, double rsrq,
                                        bool isServingCell, uint8_t ccId)
{
    if (!isServingCell) return;

    if (m_rntiToNodeId.find (rnti) == m_rntiToNodeId.end ())
    {
        for (uint32_t i = 0; i < m_ueDevs.GetN (); i++)
        {
            Ptr<LteUeNetDevice> ueDev = DynamicCast<LteUeNetDevice> (m_ueDevs.Get (i));
            if (!ueDev) continue;
            Ptr<LteUeRrc> ueRrc = ueDev->GetRrc ();
            if (ueRrc && ueRrc->GetRnti () == rnti)
            {
                m_rntiToNodeId[rnti] = ueDev->GetNode ()->GetId ();
                break;
            }
        }
    }

    auto it = m_rntiToNodeId.find (rnti);
    if (it != m_rntiToNodeId.end ())
    {
        uint32_t nid = it->second;
        if (m_ueStates.find (nid) != m_ueStates.end ())
        {
            m_ueStates[nid].rsrpDbm = rsrp;
            m_ueStates[nid].rsrqDb = rsrq;
        }
    }
}

} // namespace ns3
