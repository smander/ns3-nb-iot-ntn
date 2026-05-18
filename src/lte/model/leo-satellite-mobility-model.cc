#include "leo-satellite-mobility-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LeoSatelliteMobilityModel");
NS_OBJECT_ENSURE_REGISTERED (LeoSatelliteMobilityModel);

TypeId
LeoSatelliteMobilityModel::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::LeoSatelliteMobilityModel")
        .SetParent<MobilityModel> ()
        .SetGroupName ("Mobility")
        .AddConstructor<LeoSatelliteMobilityModel> ()
        .AddAttribute ("Altitude", "Orbit altitude in km",
                       DoubleValue (600.0),
                       MakeDoubleAccessor (&LeoSatelliteMobilityModel::m_altitude),
                       MakeDoubleChecker<double> (100.0, 40000.0))
        .AddAttribute ("Inclination", "Orbit inclination in degrees",
                       DoubleValue (86.4),
                       MakeDoubleAccessor (&LeoSatelliteMobilityModel::m_inclination),
                       MakeDoubleChecker<double> (0.0, 180.0))
        .AddAttribute ("RAAN", "Right ascension of ascending node in degrees",
                       DoubleValue (0.0),
                       MakeDoubleAccessor (&LeoSatelliteMobilityModel::m_raan),
                       MakeDoubleChecker<double> (0.0, 360.0))
        .AddAttribute ("Epoch", "Time offset for orbit phase in seconds",
                       DoubleValue (0.0),
                       MakeDoubleAccessor (&LeoSatelliteMobilityModel::m_epoch),
                       MakeDoubleChecker<double> ());
    return tid;
}

LeoSatelliteMobilityModel::LeoSatelliteMobilityModel ()
    : m_altitude (600.0), m_inclination (86.4), m_raan (0.0), m_epoch (0.0)
{
}

LeoSatelliteMobilityModel::~LeoSatelliteMobilityModel () {}

Vector
LeoSatelliteMobilityModel::DoGetPosition (void) const
{
    double t = Simulator::Now ().GetSeconds () + m_epoch;
    double r = (EARTH_RADIUS_KM + m_altitude) * 1000.0; // meters
    double orbitalPeriod = 2.0 * M_PI * std::sqrt (std::pow (r, 3) / EARTH_MU);
    double meanAnomaly = 2.0 * M_PI * t / orbitalPeriod;

    double incRad = m_inclination * M_PI / 180.0;
    double raanRad = m_raan * M_PI / 180.0;

    double xOrb = r * std::cos (meanAnomaly);
    double yOrb = r * std::sin (meanAnomaly);

    // Rotate to ECEF (simplified: ignoring Earth rotation)
    // All positions in METERS (ns-3 convention)
    double x = std::cos (raanRad) * xOrb - std::sin (raanRad) * std::cos (incRad) * yOrb;
    double y = std::sin (raanRad) * xOrb + std::cos (raanRad) * std::cos (incRad) * yOrb;
    double z = std::sin (incRad) * yOrb;

    return Vector (x, y, z); // meters
}

void
LeoSatelliteMobilityModel::DoSetPosition (const Vector &position)
{
    NS_LOG_WARN ("SetPosition called on LeoSatelliteMobilityModel; ignored");
}

Vector
LeoSatelliteMobilityModel::DoGetVelocity (void) const
{
    double r = (EARTH_RADIUS_KM + m_altitude) * 1000.0;
    double v = std::sqrt (EARTH_MU / r);
    double t = Simulator::Now ().GetSeconds () + m_epoch;
    double orbitalPeriod = 2.0 * M_PI * std::sqrt (std::pow (r, 3) / EARTH_MU);
    double meanAnomaly = 2.0 * M_PI * t / orbitalPeriod;

    double incRad = m_inclination * M_PI / 180.0;
    double raanRad = m_raan * M_PI / 180.0;

    double vxOrb = -v * std::sin (meanAnomaly);
    double vyOrb = v * std::cos (meanAnomaly);

    // All velocities in METERS/SECOND (ns-3 convention)
    double vx = std::cos (raanRad) * vxOrb - std::sin (raanRad) * std::cos (incRad) * vyOrb;
    double vy = std::sin (raanRad) * vxOrb + std::cos (raanRad) * std::cos (incRad) * vyOrb;
    double vz = std::sin (incRad) * vyOrb;

    return Vector (vx, vy, vz); // m/s
}

double
LeoSatelliteMobilityModel::GetElevation (Vector uePosition) const
{
    Vector satPos = DoGetPosition ();
    double rx = satPos.x - uePosition.x;
    double ry = satPos.y - uePosition.y;
    double rz = satPos.z - uePosition.z;
    double range = std::sqrt (rx * rx + ry * ry + rz * rz);
    if (range < 1e-6) return 90.0;

    double ueMag = std::sqrt (uePosition.x * uePosition.x +
                              uePosition.y * uePosition.y +
                              uePosition.z * uePosition.z);
    if (ueMag < 1e-6) return 0.0;

    double ux = uePosition.x / ueMag;
    double uy = uePosition.y / ueMag;
    double uz = uePosition.z / ueMag;

    double cosZenith = (rx * ux + ry * uy + rz * uz) / range;
    return 90.0 - std::acos (std::max (-1.0, std::min (1.0, cosZenith))) * 180.0 / M_PI;
}

bool
LeoSatelliteMobilityModel::IsVisible (Vector uePosition, double minElevationDeg) const
{
    return GetElevation (uePosition) >= minElevationDeg;
}

} // namespace ns3
