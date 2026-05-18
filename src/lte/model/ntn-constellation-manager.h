#ifndef NTN_CONSTELLATION_MANAGER_H
#define NTN_CONSTELLATION_MANAGER_H

#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/vector.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "nb-iot-rrc-sap.h"
#include <vector>

namespace ns3 {

class LeoSatelliteMobilityModel;

/**
 * \brief Manages a LEO satellite constellation for NB-IoT NTN.
 *
 * Tracks multiple satellites, determines serving satellite by highest
 * elevation, triggers handover when serving changes, detects coverage gaps.
 * Computes SIB31-NB parameters from satellite orbital state.
 *
 * References:
 *   - TS 36.331 Sec 6.7.4: SIB31-NB, SIB32-NB
 *   - TR 36.763 Sec 4: LEO constellation parameters
 *   - TR 36.763 Sec 6.3.1: k-Offset computation
 */
class NtnConstellationManager : public Object
{
public:
    static TypeId GetTypeId (void);
    NtnConstellationManager ();
    virtual ~NtnConstellationManager ();

    /**
     * Add a satellite node (must have LeoSatelliteMobilityModel).
     */
    void AddSatellite (Ptr<Node> satNode);

    /**
     * Set the ground reference position (cell center) for elevation computation.
     */
    void SetGroundPosition (Vector position);

    /**
     * Set minimum elevation angle for connectivity (default 10 degrees).
     */
    void SetMinElevation (double degrees);

    /**
     * Start periodic evaluation (call after all satellites added).
     * \param interval Evaluation interval (default 1 second)
     */
    void Start (Time interval = Seconds (1));

    /**
     * Is any satellite currently visible (above min elevation)?
     */
    bool IsConnected () const;

    /**
     * Get the currently serving satellite node (highest elevation).
     * Returns null if no satellite visible.
     */
    Ptr<Node> GetServingSatellite () const;

    /**
     * Get elevation angle of serving satellite in degrees.
     */
    double GetServingElevation () const;

    /**
     * Compute SIB31-NB from current serving satellite state.
     */
    NbIotRrcSap::SystemInformationBlockType31Nb ComputeSib31 () const;

    /**
     * Compute SIB32-NB with up to 4 next/neighbor satellites.
     */
    NbIotRrcSap::SystemInformationBlockType32Nb ComputeSib32 () const;

    /**
     * Did the serving satellite change since last evaluation?
     */
    bool HandoverOccurred () const;

private:
    void Evaluate ();
    double ComputeElevation (Ptr<Node> satNode) const;

    std::vector<Ptr<Node>> m_satellites;
    Ptr<Node> m_servingSatellite;
    Ptr<Node> m_previousServing;
    Vector m_groundPosition;
    double m_minElevation;
    bool m_connected;
    bool m_handoverOccurred;
    double m_servingElevation;
    EventId m_evaluateEvent;
    Time m_interval;

    static constexpr double EARTH_RADIUS_KM = 6371.0;
    static constexpr double SPEED_OF_LIGHT = 299792458.0;
    static constexpr double TS_SEC = 1.0 / (15000.0 * 2048.0);
    static constexpr double SUBFRAME_DURATION_SEC = 0.001; // 1ms NB-IoT subframe (k-Offset unit per 3GPP)
};

} // namespace ns3

#endif
