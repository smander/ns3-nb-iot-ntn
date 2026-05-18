#include "rach-flood-app.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RachFloodApp");
NS_OBJECT_ENSURE_REGISTERED (RachFloodApp);

TypeId RachFloodApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::RachFloodApp")
        .SetParent<Application> ()
        .SetGroupName ("Applications")
        .AddConstructor<RachFloodApp> ()
        .AddAttribute ("BurstInterval", "Seconds between bursts",
                       DoubleValue (5.0),
                       MakeDoubleAccessor (&RachFloodApp::m_burstInterval),
                       MakeDoubleChecker<double> (0.1))
        .AddAttribute ("AttemptsPerBurst", "Connection attempts per burst",
                       UintegerValue (15),
                       MakeUintegerAccessor (&RachFloodApp::m_attemptsPerBurst),
                       MakeUintegerChecker<uint32_t> (1, 100));
    return tid;
}

RachFloodApp::RachFloodApp ()
    : m_socket (0), m_peerPort (5683), m_burstInterval (5.0), m_attemptsPerBurst (15), m_coapMsgId (0)
{}

RachFloodApp::~RachFloodApp () { m_socket = 0; }
void RachFloodApp::SetRemote (Ipv4Address addr, uint16_t port) { m_peerAddress = addr; m_peerPort = port; }

void RachFloodApp::SendBurst (void)
{
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
        m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }

    // CoAP: Ver=1, Type=CON(0), TKL=2, Code=POST(0x02)
    // Rapid burst: each packet in the burst gets an incremented MsgID
    uint8_t coapType = 0;  // CON
    uint8_t coapTkl  = 2;
    uint8_t coapCode = 0x02; // POST
    uint32_t payloadSize = 36; // 40 total - 4 header = 36 payload bytes

    for (uint32_t i = 0; i < m_attemptsPerBurst; i++)
    {
        m_coapMsgId++;

        uint8_t hdr[4];
        hdr[0] = (0x01 << 6) | (coapType << 4) | (coapTkl & 0x0F);
        hdr[1] = coapCode;
        hdr[2] = (m_coapMsgId >> 8) & 0xFF;
        hdr[3] = m_coapMsgId & 0xFF;

        uint32_t totalSize = 4 + payloadSize;
        uint8_t *buf = new uint8_t[totalSize];
        std::memcpy (buf, hdr, 4);
        std::memset (buf + 4, 0x00, payloadSize);
        Ptr<Packet> p = Create<Packet> (buf, totalSize);
        delete[] buf;

        m_socket->Send (p);
    }

    m_sendEvent = Simulator::Schedule (Seconds (m_burstInterval), &RachFloodApp::SendBurst, this);
}

void RachFloodApp::StartApplication (void) { SendBurst (); }
void RachFloodApp::StopApplication (void) { Simulator::Cancel (m_sendEvent); if (m_socket) m_socket->Close (); }

} // namespace ns3
