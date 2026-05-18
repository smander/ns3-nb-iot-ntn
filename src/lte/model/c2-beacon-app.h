#ifndef C2_BEACON_APP_H
#define C2_BEACON_APP_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

class C2BeaconApp : public Application
{
public:
    static TypeId GetTypeId (void);
    C2BeaconApp ();
    virtual ~C2BeaconApp ();
    void SetRemote (Ipv4Address addr, uint16_t port);
private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);
    void SendBeacon (void);
    Ptr<Socket> m_socket;
    Ipv4Address m_peerAddress;
    uint16_t m_peerPort;
    double m_beaconInterval;
    uint32_t m_payloadSize;
    EventId m_sendEvent;
    uint16_t m_coapMsgId;
};

} // namespace ns3
#endif
