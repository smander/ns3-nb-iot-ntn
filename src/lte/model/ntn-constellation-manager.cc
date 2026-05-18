#include "ntn-constellation-manager.h"
#include "leo-satellite-mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NtnConstellationManager");
NS_OBJECT_ENSURE_REGISTERED (NtnConstellationManager);

TypeId
NtnConstellationManager::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::NtnConstellationManager")
        .SetParent<Object> ()
        .SetGroupName ("Lte")
        .AddConstructor<NtnConstellationManager> ();
    return tid;
}

NtnConstellationManager::NtnConstellationManager ()
    : m_servingSatellite (0),
      m_previousServing (0),
      m_groundPosition (Vector (0, 0, EARTH_RADIUS_KM * 1000.0)), // meters
      m_minElevation (10.0),
      m_connected (false),
      m_handoverOccurred (false),
      m_servingElevation (0.0),
      m_interval (Seconds (1))
{
}

NtnConstellationManager::~NtnConstellationManager ()
{
}

void
NtnConstellationManager::AddSatellite (Ptr<Node> satNode)
{
    m_satellites.push_back (satNode);
    NS_LOG_INFO ("NtnConstellationManager: added satellite, total=" << m_satellites.size ());
}

void
NtnConstellationManager::SetGroundPosition (Vector position)
{
    m_groundPosition = position;
}

void
NtnConstellationManager::SetMinElevation (double degrees)
{
    m_minElevation = degrees;
}

void
NtnConstellationManager::Start (Time interval)
{
    m_interval = interval;
    Evaluate ();
}

bool
NtnConstellationManager::IsConnected () const
{
    return m_connected;
}

Ptr<Node>
NtnConstellationManager::GetServingSatellite () const
{
    return m_servingSatellite;
}

double
NtnConstellationManager::GetServingElevation () const
{
    return m_servingElevation;
}

bool
NtnConstellationManager::HandoverOccurred () const
{
    return m_handoverOccurred;
}

double
NtnConstellationManager::ComputeElevation (Ptr<Node> satNode) const
{
    Ptr<MobilityModel> satMob = satNode->GetObject<MobilityModel> ();
    if (!satMob)
        return -90.0;

    Vector satPos = satMob->GetPosition (); // km
    double rx = satPos.x - m_groundPosition.x;
    double ry = satPos.y - m_groundPosition.y;
    double rz = satPos.z - m_groundPosition.z;
    double range = std::sqrt (rx * rx + ry * ry + rz * rz);

    if (range < 1e-6)
        return 90.0;

    // Local up vector (radial from Earth center)
    double gmag = std::sqrt (m_groundPosition.x * m_groundPosition.x +
                             m_groundPosition.y * m_groundPosition.y +
                             m_groundPosition.z * m_groundPosition.z);
    if (gmag < 1e-6)
        return 0.0;

    double ux = m_groundPosition.x / gmag;
    double uy = m_groundPosition.y / gmag;
    double uz = m_groundPosition.z / gmag;

    double cosZenith = (rx * ux + ry * uy + rz * uz) / range;
    return 90.0 - std::acos (std::max (-1.0, std::min (1.0, cosZenith))) * 180.0 / M_PI;
}

void
NtnConstellationManager::Evaluate ()
{
    m_previousServing = m_servingSatellite;
    m_handoverOccurred = false;

    // Find satellite with highest elevation
    Ptr<Node> bestSat = 0;
    double bestElev = -90.0;

    for (auto &sat : m_satellites)
    {
        double elev = ComputeElevation (sat);
        if (elev > bestElev)
        {
            bestElev = elev;
            bestSat = sat;
        }
    }

    if (bestElev >= m_minElevation)
    {
        m_connected = true;
        m_servingSatellite = bestSat;
        m_servingElevation = bestElev;

        if (m_previousServing != m_servingSatellite && m_previousServing != 0)
        {
            m_handoverOccurred = true;
            NS_LOG_INFO ("NtnConstellationManager: HANDOVER at t="
                         << Simulator::Now ().GetSeconds ()
                         << "s, new serving elev=" << bestElev << " deg");
        }
    }
    else
    {
        m_connected = false;
        m_servingSatellite = 0;
        m_servingElevation = 0.0;
        NS_LOG_INFO ("NtnConstellationManager: COVERAGE GAP at t="
                     << Simulator::Now ().GetSeconds () << "s");
    }

    // Schedule next evaluation
    m_evaluateEvent = Simulator::Schedule (m_interval,
                                           &NtnConstellationManager::Evaluate, this);
}

NbIotRrcSap::SystemInformationBlockType31Nb
NtnConstellationManager::ComputeSib31 () const
{
    NbIotRrcSap::SystemInformationBlockType31Nb sib31;

    if (!m_servingSatellite)
        return sib31;

    Ptr<MobilityModel> satMob = m_servingSatellite->GetObject<MobilityModel> ();
    if (!satMob)
        return sib31;

    Vector pos = satMob->GetPosition ();  // meters (after BUG-1 fix)
    Vector vel = satMob->GetVelocity ();  // m/s (after BUG-1 fix)

    // State vector in ECEF — already in meters and m/s
    sib31.stateVector.positionX = (int32_t)(pos.x);
    sib31.stateVector.positionY = (int32_t)(pos.y);
    sib31.stateVector.positionZ = (int32_t)(pos.z);
    sib31.stateVector.velocityX = (int16_t)(vel.x);
    sib31.stateVector.velocityY = (int16_t)(vel.y);
    sib31.stateVector.velocityZ = (int16_t)(vel.z);
    sib31.useStateVector = true;

    // ta_common = 2 * distance(satellite, cell_center) / c / Ts
    // 3GPP ASN.1: ta-Common-r17 is INTEGER(-256..256) in units of 16*Ts
    // Raw TA in Ts units, then quantize to 16*Ts steps
    double dx = pos.x - m_groundPosition.x;
    double dy = pos.y - m_groundPosition.y;
    double dz = pos.z - m_groundPosition.z;
    double distM = std::sqrt (dx * dx + dy * dy + dz * dz);
    double delaySec = distM / SPEED_OF_LIGHT;
    double rawTaTs = 2.0 * delaySec / TS_SEC;
    // Quantize to 16*Ts steps per ASN.1 (BUG-8 fix)
    sib31.taCommon = (int32_t)(rawTaTs); // store raw Ts for IDS feature accuracy
    // Note: for over-the-air encoding, would be: (int32_t)(rawTaTs / 16.0) clamped to [-256,256]

    // k_offset = ceil(2 * distance / c / slot_duration)
    sib31.kOffset = (uint16_t)std::min (1023.0, std::max (2.0,
        std::ceil (2.0 * delaySec / SUBFRAME_DURATION_SEC)));

    // ta_common_drift from radial velocity (all in meters and m/s)
    if (distM > 1.0)
    {
        double radialVelMS = (vel.x * dx + vel.y * dy + vel.z * dz) / distM;
        sib31.taCommonDrift = (int16_t)(2.0 * radialVelMS / SPEED_OF_LIGHT / TS_SEC);
    }

    // Epoch
    sib31.epochTimeSfn = (uint16_t)((uint64_t)(Simulator::Now ().GetMilliSeconds () / 10) % 1024);
    sib31.epochTimeSubframe = (uint8_t)((uint64_t)(Simulator::Now ().GetMilliSeconds ()) % 10);

    sib31.ulSyncValidityDuration = 60; // 60 seconds for LEO
    sib31.ntnPolarizationDL = 0; // RHCP
    sib31.ntnPolarizationUL = 0; // RHCP

    return sib31;
}

NbIotRrcSap::SystemInformationBlockType32Nb
NtnConstellationManager::ComputeSib32 () const
{
    NbIotRrcSap::SystemInformationBlockType32Nb sib32;

    // Collect up to 4 non-serving satellites sorted by elevation
    struct SatElev {
        Ptr<Node> node;
        double elevation;
    };
    std::vector<SatElev> others;

    for (auto &sat : m_satellites)
    {
        if (sat == m_servingSatellite)
            continue;
        double elev = ComputeElevation (sat);
        others.push_back ({sat, elev});
    }

    // Sort by elevation descending (highest = most likely next serving)
    std::sort (others.begin (), others.end (),
               [](const SatElev &a, const SatElev &b) { return a.elevation > b.elevation; });

    uint8_t count = std::min ((uint8_t)4, (uint8_t)others.size ());
    sib32.numSatellites = count;

    for (uint8_t i = 0; i < count; i++)
    {
        Ptr<MobilityModel> mob = others[i].node->GetObject<MobilityModel> ();
        if (!mob) continue;

        Vector p = mob->GetPosition ();
        Vector v = mob->GetVelocity ();
        sib32.satellites[i].ephemeris.positionX = (int32_t)(p.x);
        sib32.satellites[i].ephemeris.positionY = (int32_t)(p.y);
        sib32.satellites[i].ephemeris.positionZ = (int32_t)(p.z);
        sib32.satellites[i].ephemeris.velocityX = (int16_t)(v.x);
        sib32.satellites[i].ephemeris.velocityY = (int16_t)(v.y);
        sib32.satellites[i].ephemeris.velocityZ = (int16_t)(v.z);
        sib32.satellites[i].cellId = others[i].node->GetId ();
        sib32.satellites[i].valid = true;
    }

    return sib32;
}

} // namespace ns3
