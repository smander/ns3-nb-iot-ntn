#include "nbiot-telemetry-app.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NbIotTelemetryApp");
NS_OBJECT_ENSURE_REGISTERED (NbIotTelemetryApp);

TypeId NbIotTelemetryApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::NbIotTelemetryApp")
        .SetParent<Application> ()
        .SetGroupName ("Applications")
        .AddConstructor<NbIotTelemetryApp> ()
        .AddAttribute ("Interval", "Mean interval between sends (seconds)",
                       DoubleValue (300.0),
                       MakeDoubleAccessor (&NbIotTelemetryApp::m_interval),
                       MakeDoubleChecker<double> (1.0))
        .AddAttribute ("PayloadSize", "Sensor payload size (bytes)",
                       UintegerValue (50),
                       MakeUintegerAccessor (&NbIotTelemetryApp::m_payloadSize),
                       MakeUintegerChecker<uint32_t> (1, 1500));
    return tid;
}

NbIotTelemetryApp::NbIotTelemetryApp ()
    : m_socket (0), m_peerPort (5683), m_interval (300.0),
      m_payloadSize (50), m_sent (0), m_coapMsgId (0), m_coapTkl (2)
{
    m_intervalJitter = CreateObject<UniformRandomVariable> ();
    m_intervalJitter->SetAttribute ("Min", DoubleValue (0.8));
    m_intervalJitter->SetAttribute ("Max", DoubleValue (1.2));
    // Random TKL 1-4 per instance
    Ptr<UniformRandomVariable> tklRng = CreateObject<UniformRandomVariable> ();
    m_coapTkl = 1 + (tklRng->GetInteger () % 4);
}

NbIotTelemetryApp::~NbIotTelemetryApp () { m_socket = 0; }

void NbIotTelemetryApp::SetRemote (Ipv4Address addr, uint16_t port)
{ m_peerAddress = addr; m_peerPort = port; }

void NbIotTelemetryApp::SendTelemetry (void)
{
    // Build CoAP header (4 bytes) + token + sensor payload
    // Byte 0: Ver=1(01), Type=NON(01) or CON(00), TKL
    // Byte 1: Code=POST(0x02) or PUT(0x03)
    // Byte 2-3: Message ID (incrementing)
    uint8_t coapType = (m_sent % 5 == 0) ? 0 : 1; // 20% CON, 80% NON
    uint8_t coapCode = (m_sent % 20 == 0) ? 0x03 : 0x02; // 5% PUT, 95% POST

    uint8_t hdr[4];
    hdr[0] = (0x01 << 6) | (coapType << 4) | (m_coapTkl & 0x0F);
    hdr[1] = coapCode;
    hdr[2] = (m_coapMsgId >> 8) & 0xFF;
    hdr[3] = m_coapMsgId & 0xFF;

    // Create packet: CoAP header + token bytes + sensor payload
    uint32_t totalSize = 4 + m_coapTkl + m_payloadSize;
    uint8_t *buf = new uint8_t[totalSize];
    std::memcpy (buf, hdr, 4);
    std::memset (buf + 4, 0xAB, m_coapTkl); // token
    // Payload marker (0xFF) + sensor data
    if (m_payloadSize > 0)
    {
        buf[4 + m_coapTkl] = 0xFF; // payload marker
        for (uint32_t i = 1; i < m_payloadSize; i++)
            buf[4 + m_coapTkl + i] = (uint8_t)(i & 0xFF);
    }

    Ptr<Packet> p = Create<Packet> (buf, totalSize);
    delete[] buf;

    m_socket->Send (p);
    m_sent++;
    m_coapMsgId++;
    ScheduleNext ();
}

void NbIotTelemetryApp::ScheduleNext (void)
{
    double jitter = m_intervalJitter->GetValue ();
    m_sendEvent = Simulator::Schedule (Seconds (m_interval * jitter),
                                       &NbIotTelemetryApp::SendTelemetry, this);
}

void NbIotTelemetryApp::StartApplication (void)
{
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
        m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }
    ScheduleNext ();
}

void NbIotTelemetryApp::StopApplication (void)
{
    Simulator::Cancel (m_sendEvent);
    if (m_socket) m_socket->Close ();
}

} // namespace ns3
