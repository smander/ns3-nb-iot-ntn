#ifndef NTN_HELPER_H
#define NTN_HELPER_H

#include "ns3/node-container.h"
#include "ns3/ipv4-address.h"
#include <string>

namespace ns3 {

/**
 * \brief Helper for setting up NB-IoT NTN simulation scenarios.
 *
 * Simplifies satellite node configuration, UE placement,
 * application installation, and IDS tracing setup.
 */
class NtnHelper
{
public:
    NtnHelper ();
    ~NtnHelper ();

    /**
     * Configure a satellite node with LEO mobility model.
     * \param node Satellite node
     * \param altitudeKm Orbit altitude in km (default 600)
     * \param inclinationDeg Orbit inclination in degrees (default 86.4)
     * \param raanDeg Right ascension of ascending node (default 0)
     */
    void InstallSatellite (Ptr<Node> node, double altitudeKm = 600.0,
                           double inclinationDeg = 86.4, double raanDeg = 0.0);

    /**
     * Configure UE nodes with fixed ground positions.
     * \param nodes UE nodes
     * \param radiusKm Random placement within this radius from origin
     */
    void InstallUes (NodeContainer nodes, double radiusKm = 50.0);

    /**
     * Install benign telemetry application on nodes.
     * \param nodes Target UE nodes
     * \param serverAddr IPv6 address of telemetry server
     * \param intervalSec Mean telemetry interval in seconds
     * \param payloadSize Payload size in bytes
     */
    void InstallTelemetryApp (NodeContainer nodes, Ipv4Address serverAddr,
                              double intervalSec = 300.0, uint32_t payloadSize = 50);

    /**
     * Install attack application on a node.
     * \param node Target UE node
     * \param attackType 1=Flood, 2=C2, 3=RACH, 4=Spoofing, 5=Exfil
     * \param serverAddr Target address
     */
    void InstallAttackApp (Ptr<Node> node, uint8_t attackType, Ipv4Address serverAddr);

    /**
     * Enable IDS feature tracing on all nodes.
     * \param nodes All UE nodes to trace
     * \param outputPath CSV output file path
     */
    void EnableIdsTracing (NodeContainer nodes, std::string outputPath);
};

} // namespace ns3

#endif
