#ifndef FLOOD_ATTACK_APP_H
#define FLOOD_ATTACK_APP_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class FloodAttackApp : public Application
{
public:
    static TypeId GetTypeId (void);
    FloodAttackApp ();
    virtual ~FloodAttackApp ();
    void SetRemote (Ipv4Address addr);
private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);
    void SendFlood (void);
    Ptr<Socket> m_socket;
    Ipv4Address m_peerAddress;
    double m_packetRate;
    uint32_t m_packetSize;
    EventId m_sendEvent;
    Ptr<UniformRandomVariable> m_portRng;
    Ptr<UniformRandomVariable> m_coapRng;
    uint16_t m_coapMsgId;
};

} // namespace ns3
#endif
