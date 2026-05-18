#ifndef IDS_FEATURE_TRACER_H
#define IDS_FEATURE_TRACER_H

#include "ns3/object.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4.h"
#include "ns3/udp-header.h"
#include <fstream>
#include <map>

namespace ns3 {

/**
 * \brief Window-level IDS feature extractor for NB-IoT NTN.
 *
 * Outputs ONE row per UE per window (default 10 seconds).
 * Aggregates per-packet features within each window.
 * This naturally balances the dataset by UE count, not packet count.
 */
class IdsFeatureTracer : public Object
{
public:
    static TypeId GetTypeId (void);
    IdsFeatureTracer ();
    virtual ~IdsFeatureTracer ();

    void Install (NodeContainer ueNodes, NetDeviceContainer ueDevs,
                  std::string outputPath, double windowSec = 10.0);
    void SetEnbNode (Ptr<Node> enbNode);
    void SetLabel (uint32_t nodeId, uint8_t label);
    void SetNtnParams (int32_t taCommon, uint16_t kOffset);

private:
    struct PerUeState {
        // Window counters
        uint32_t pktCount;
        uint32_t byteCount;
        uint32_t connAttempts;
        double windowStart;

        // Aggregated packet features (accumulated during window)
        uint16_t maxPayloadLen;
        uint16_t minPayloadLen;
        uint8_t dominantProtocol;    // most seen ip_next_header
        uint8_t minHopLimit;
        uint16_t lastSrcPort;
        uint16_t lastDstPort;
        uint16_t uniqueDstPorts;
        uint8_t dominantCoapType;
        uint8_t dominantCoapCode;
        uint8_t dominantCoapTkl;
        uint32_t ulCount;           // uplink packet count
        uint32_t dlCount;           // downlink packet count

        // Protocol counters for "dominant" computation
        uint32_t udpCount;
        uint32_t icmpCount;

        // CoAP type counters
        uint32_t coapConCount;
        uint32_t coapNonCount;

        // Radio (cached from PHY)
        double rsrpDbm;
        double rsrqDb;

        // Label
        uint8_t label;

        // Dst port tracking (simple hash set)
        uint16_t seenPorts[32];
        uint8_t seenPortCount;
    };

    void Ipv4TxCallback (Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t iface);
    void Ipv4RxCallback (Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t iface);
    void ProcessPacket (Ptr<const Packet> packet, uint32_t nodeId, uint8_t direction);
    void MeasurementCallback (uint16_t rnti, uint16_t cellId, double rsrp, double rsrq, bool isServingCell, uint8_t ccId);
    void RachCallback (uint8_t ceLevel);
    void FlushWindow (uint32_t nodeId);
    void ResetWindow (uint32_t nodeId);
    void ScheduleWindowFlush ();
    void FlushAllWindows ();

    std::ofstream m_outFile;
    double m_windowSec;
    std::map<uint32_t, PerUeState> m_ueStates;

    NodeContainer m_ueNodes;
    NetDeviceContainer m_ueDevs;
    Ptr<Node> m_enbNode;

    int32_t m_ntnTaCommon;
    uint16_t m_ntnKOffset;

    std::map<uint16_t, uint32_t> m_rntiToNodeId;
    uint32_t m_lastRachNodeId;
};

} // namespace ns3
#endif
