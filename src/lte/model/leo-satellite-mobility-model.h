#ifndef LEO_SATELLITE_MOBILITY_MODEL_H
#define LEO_SATELLITE_MOBILITY_MODEL_H

#include "ns3/mobility-model.h"
#include "ns3/nstime.h"

namespace ns3 {

/**
 * \brief LEO satellite circular orbit mobility model.
 *
 * Computes ECEF position and velocity for a satellite in circular orbit
 * at configurable altitude and inclination.
 *
 * Reference: 3GPP TR 38.811 for orbit parameters.
 */
class LeoSatelliteMobilityModel : public MobilityModel
{
public:
    static TypeId GetTypeId (void);
    LeoSatelliteMobilityModel ();
    virtual ~LeoSatelliteMobilityModel ();

    double GetElevation (Vector uePosition) const;
    bool IsVisible (Vector uePosition, double minElevationDeg = 10.0) const;

private:
    virtual Vector DoGetPosition (void) const;
    virtual void DoSetPosition (const Vector &position);
    virtual Vector DoGetVelocity (void) const;

    double m_altitude;       //!< Orbit altitude in km (default 600)
    double m_inclination;    //!< Orbit inclination in degrees (default 86.4)
    double m_raan;           //!< Right ascension of ascending node in degrees
    double m_epoch;          //!< Simulation time offset in seconds

    static constexpr double EARTH_RADIUS_KM = 6371.0;
    static constexpr double EARTH_MU = 3.986004418e14; // m^3/s^2
};

} // namespace ns3

#endif
