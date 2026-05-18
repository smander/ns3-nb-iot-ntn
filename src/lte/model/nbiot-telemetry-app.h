#ifndef NBIOT_TELEMETRY_APP_H
#define NBIOT_TELEMETRY_APP_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class NbIotTelemetryApp : public Application
{
public:
    static TypeId GetTypeId (void);
    NbIotTelemetryApp ();
    virtual ~NbIotTelemetryApp ();
    void SetRemote (Ipv4Address addr, uint16_t port);
private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);
    void SendTelemetry (void);
    void ScheduleNext (void);

    Ptr<Socket> m_socket;
    Ipv4Address m_peerAddress;
    uint16_t m_peerPort;
    double m_interval;
    uint32_t m_payloadSize;
    EventId m_sendEvent;
    uint32_t m_sent;
    uint16_t m_coapMsgId;
    uint8_t m_coapTkl;  // token length, random 1-4 per instance
    Ptr<UniformRandomVariable> m_intervalJitter;
};

} // namespace ns3
#endif
