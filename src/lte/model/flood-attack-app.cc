#include "flood-attack-app.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include <cstring>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FloodAttackApp");
NS_OBJECT_ENSURE_REGISTERED (FloodAttackApp);

TypeId FloodAttackApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::FloodAttackApp")
        .SetParent<Application> ()
        .SetGroupName ("Applications")
        .AddConstructor<FloodAttackApp> ()
        .AddAttribute ("PacketRate", "Packets per second",
                       DoubleValue (50.0),
                       MakeDoubleAccessor (&FloodAttackApp::m_packetRate),
                       MakeDoubleChecker<double> (0.1))
        .AddAttribute ("PacketSize", "Flood packet size (bytes)",
                       UintegerValue (40),
                       MakeUintegerAccessor (&FloodAttackApp::m_packetSize),
                       MakeUintegerChecker<uint32_t> (1, 1500));
    return tid;
}

FloodAttackApp::FloodAttackApp ()
    : m_socket (0), m_packetRate (50.0), m_packetSize (40), m_coapMsgId (0)
{
    m_portRng = CreateObject<UniformRandomVariable> ();
    m_portRng->SetAttribute ("Min", DoubleValue (1.0));
    m_portRng->SetAttribute ("Max", DoubleValue (65535.0));

    m_coapRng = CreateObject<UniformRandomVariable> ();
    m_coapRng->SetAttribute ("Min", DoubleValue (0.0));
    m_coapRng->SetAttribute ("Max", DoubleValue (1.0));
}

FloodAttackApp::~FloodAttackApp () { m_socket = 0; }
void FloodAttackApp::SetRemote (Ipv4Address addr) { m_peerAddress = addr; }

void FloodAttackApp::SendFlood (void)
{
    uint16_t dstPort = static_cast<uint16_t> (m_portRng->GetInteger ());
    if (!m_socket)
        m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));

    Ptr<Packet> p;

    // 50% chance: prepend CoAP NON header; 50% raw UDP flood
    if (m_coapRng->GetValue () >= 0.5)
    {
        // CoAP: Ver=1, Type=NON(1), TKL=0, Code=random 0-4, incrementing MsgID
        uint8_t coapType = 1;  // NON
        uint8_t coapTkl  = 0;
        uint8_t coapCode = static_cast<uint8_t> (m_portRng->GetInteger () % 5); // 0-4
        m_coapMsgId++;

        uint8_t hdr[4];
        hdr[0] = (0x01 << 6) | (coapType << 4) | (coapTkl & 0x0F);
        hdr[1] = coapCode;
        hdr[2] = (m_coapMsgId >> 8) & 0xFF;
        hdr[3] = m_coapMsgId & 0xFF;

        uint32_t payloadSize = (m_packetSize > 4) ? (m_packetSize - 4) : 0;
        uint32_t totalSize   = 4 + payloadSize;
        uint8_t *buf = new uint8_t[totalSize];
        std::memcpy (buf, hdr, 4);
        std::memset (buf + 4, 0x00, payloadSize);
        p = Create<Packet> (buf, totalSize);
        delete[] buf;
    }
    else
    {
        // Raw UDP flood — no CoAP header
        p = Create<Packet> (m_packetSize);
    }

    m_socket->SendTo (p, 0, InetSocketAddress (m_peerAddress, dstPort));
    m_sendEvent = Simulator::Schedule (Seconds (1.0 / m_packetRate), &FloodAttackApp::SendFlood, this);
}

void FloodAttackApp::StartApplication (void) { SendFlood (); }
void FloodAttackApp::StopApplication (void) { Simulator::Cancel (m_sendEvent); if (m_socket) m_socket->Close (); }

} // namespace ns3
