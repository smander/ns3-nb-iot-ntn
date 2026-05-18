#include "c2-beacon-app.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("C2BeaconApp");
NS_OBJECT_ENSURE_REGISTERED (C2BeaconApp);

TypeId C2BeaconApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::C2BeaconApp")
        .SetParent<Application> ()
        .SetGroupName ("Applications")
        .AddConstructor<C2BeaconApp> ()
        .AddAttribute ("BeaconInterval", "Seconds between beacons",
                       DoubleValue (30.0),
                       MakeDoubleAccessor (&C2BeaconApp::m_beaconInterval),
                       MakeDoubleChecker<double> (0.1))
        .AddAttribute ("PayloadSize", "Beacon payload bytes",
                       UintegerValue (20),
                       MakeUintegerAccessor (&C2BeaconApp::m_payloadSize),
                       MakeUintegerChecker<uint32_t> (1, 200))
        .AddAttribute ("DestPort", "C2 server port",
                       UintegerValue (4444),
                       MakeUintegerAccessor (&C2BeaconApp::m_peerPort),
                       MakeUintegerChecker<uint16_t> ());
    return tid;
}

C2BeaconApp::C2BeaconApp ()
    : m_socket (0), m_peerPort (4444), m_beaconInterval (30.0), m_payloadSize (20), m_coapMsgId (0)
{}

C2BeaconApp::~C2BeaconApp () { m_socket = 0; }
void C2BeaconApp::SetRemote (Ipv4Address addr, uint16_t port) { m_peerAddress = addr; m_peerPort = port; }

void C2BeaconApp::SendBeacon (void)
{
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
        m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }

    // CoAP: Ver=1, Type=CON(0), TKL=1, Code=GET(0x01), fixed MsgID pattern (increments per beacon)
    uint8_t coapType = 0;  // CON
    uint8_t coapTkl  = 1;
    uint8_t coapCode = 0x01; // GET
    // MsgID uses a fixed base pattern: upper byte 0xBE, lower byte increments
    m_coapMsgId = static_cast<uint16_t> (0xBE00 | (m_coapMsgId & 0x00FF) + 1);

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
    m_sendEvent = Simulator::Schedule (Seconds (m_beaconInterval), &C2BeaconApp::SendBeacon, this);
}

void C2BeaconApp::StartApplication (void) { SendBeacon (); }
void C2BeaconApp::StopApplication (void) { Simulator::Cancel (m_sendEvent); if (m_socket) m_socket->Close (); }

} // namespace ns3
