#include "exfiltration-app.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ExfiltrationApp");
NS_OBJECT_ENSURE_REGISTERED (ExfiltrationApp);

TypeId ExfiltrationApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::ExfiltrationApp")
        .SetParent<Application> ()
        .SetGroupName ("Applications")
        .AddConstructor<ExfiltrationApp> ()
        .AddAttribute ("SendInterval", "Seconds between exfil packets",
                       DoubleValue (10.0),
                       MakeDoubleAccessor (&ExfiltrationApp::m_sendInterval),
                       MakeDoubleChecker<double> (0.1))
        .AddAttribute ("PayloadSize", "Exfil payload bytes",
                       UintegerValue (800),
                       MakeUintegerAccessor (&ExfiltrationApp::m_payloadSize),
                       MakeUintegerChecker<uint32_t> (100, 1500))
        .AddAttribute ("DestPort", "Exfil destination port",
                       UintegerValue (8443),
                       MakeUintegerAccessor (&ExfiltrationApp::m_peerPort),
                       MakeUintegerChecker<uint16_t> ());
    return tid;
}

ExfiltrationApp::ExfiltrationApp ()
    : m_socket (0), m_peerPort (8443), m_sendInterval (10.0), m_payloadSize (800), m_coapMsgId (0)
{}

ExfiltrationApp::~ExfiltrationApp () { m_socket = 0; }
void ExfiltrationApp::SetRemote (Ipv4Address addr, uint16_t port) { m_peerAddress = addr; m_peerPort = port; }

void ExfiltrationApp::SendExfil (void)
{
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
        m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }

    // CoAP: Ver=1, Type=NON(1), TKL=4 (large token), Code=POST(0x02), incrementing MsgID
    uint8_t coapType = 1;  // NON
    uint8_t coapTkl  = 4;  // large token — exfil indicator
    uint8_t coapCode = 0x02; // POST
    m_coapMsgId++;

    uint8_t hdr[4];
    hdr[0] = (0x01 << 6) | (coapType << 4) | (coapTkl & 0x0F);
    hdr[1] = coapCode;
    hdr[2] = (m_coapMsgId >> 8) & 0xFF;
    hdr[3] = m_coapMsgId & 0xFF;

    // Large 800-byte payload (minus 4-byte header = 796 bytes of exfil data)
    uint32_t payloadSize = (m_payloadSize > 4) ? (m_payloadSize - 4) : 0;
    uint32_t totalSize   = 4 + payloadSize;
    uint8_t *buf = new uint8_t[totalSize];
    std::memcpy (buf, hdr, 4);
    std::memset (buf + 4, 0x00, payloadSize);
    Ptr<Packet> p = Create<Packet> (buf, totalSize);
    delete[] buf;

    m_socket->Send (p);
    m_sendEvent = Simulator::Schedule (Seconds (m_sendInterval), &ExfiltrationApp::SendExfil, this);
}

void ExfiltrationApp::StartApplication (void) { SendExfil (); }
void ExfiltrationApp::StopApplication (void) { Simulator::Cancel (m_sendEvent); if (m_socket) m_socket->Close (); }

} // namespace ns3
