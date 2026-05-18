#include "spoofing-app.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SpoofingApp");
NS_OBJECT_ENSURE_REGISTERED (SpoofingApp);

TypeId SpoofingApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SpoofingApp")
        .SetParent<Application> ()
        .SetGroupName ("Applications")
        .AddConstructor<SpoofingApp> ()
        .AddAttribute ("ReplayInterval", "Seconds between replays",
                       DoubleValue (60.0),
                       MakeDoubleAccessor (&SpoofingApp::m_replayInterval),
                       MakeDoubleChecker<double> (1.0))
        .AddAttribute ("PayloadSize", "Replayed payload size",
                       UintegerValue (50),
                       MakeUintegerAccessor (&SpoofingApp::m_payloadSize),
                       MakeUintegerChecker<uint32_t> (1, 500));
    return tid;
}

SpoofingApp::SpoofingApp ()
    : m_socket (0), m_peerPort (5683), m_replayInterval (60.0), m_payloadSize (50),
      m_coapMsgId (0x0001)  // fixed stale MsgID — replay indicator
{}

SpoofingApp::~SpoofingApp () { m_socket = 0; }
void SpoofingApp::SetRemote (Ipv4Address addr, uint16_t port) { m_peerAddress = addr; m_peerPort = port; }

void SpoofingApp::SendReplay (void)
{
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
        m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }

    // CoAP: Ver=1, Type=CON(0), TKL=0 (no token = replay), Code=POST(0x02)
    // FIXED MsgID=0x0001 — stale/replayed, never increments
    uint8_t coapType = 0;  // CON
    uint8_t coapTkl  = 0;  // no token — key replay indicator
    uint8_t coapCode = 0x02; // POST
    // m_coapMsgId stays fixed at 0x0001 (set in constructor, never updated)

    uint8_t hdr[4];
    hdr[0] = (0x01 << 6) | (coapType << 4) | (coapTkl & 0x0F);
    hdr[1] = coapCode;
    hdr[2] = (m_coapMsgId >> 8) & 0xFF;
    hdr[3] = m_coapMsgId & 0xFF;

    uint32_t payloadSize = (m_payloadSize > 4) ? (m_payloadSize - 4) : 0;
    uint32_t totalSize   = 4 + payloadSize;
    uint8_t *buf = new uint8_t[totalSize];
    std::memcpy (buf, hdr, 4);
    std::memset (buf + 4, 0x00, payloadSize);
    Ptr<Packet> p = Create<Packet> (buf, totalSize);
    delete[] buf;

    m_socket->Send (p);
    m_sendEvent = Simulator::Schedule (Seconds (m_replayInterval), &SpoofingApp::SendReplay, this);
}

void SpoofingApp::StartApplication (void) { SendReplay (); }
void SpoofingApp::StopApplication (void) { Simulator::Cancel (m_sendEvent); if (m_socket) m_socket->Close (); }

} // namespace ns3
