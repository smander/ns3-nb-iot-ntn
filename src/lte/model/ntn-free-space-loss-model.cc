#include "ntn-free-space-loss-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NtnFreeSpaceLossModel");
NS_OBJECT_ENSURE_REGISTERED (NtnFreeSpaceLossModel);

TypeId NtnFreeSpaceLossModel::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::NtnFreeSpaceLossModel")
        .SetParent<PropagationLossModel> ()
        .SetGroupName ("Propagation")
        .AddConstructor<NtnFreeSpaceLossModel> ()
        .AddAttribute ("Frequency", "Carrier frequency in Hz",
                       DoubleValue (900e6),
                       MakeDoubleAccessor (&NtnFreeSpaceLossModel::m_frequency),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("AtmosphericLoss", "Atmospheric loss in dB at zenith",
                       DoubleValue (0.5),
                       MakeDoubleAccessor (&NtnFreeSpaceLossModel::m_atmosphericLoss),
                       MakeDoubleChecker<double> ());
    return tid;
}

NtnFreeSpaceLossModel::NtnFreeSpaceLossModel () : m_frequency (900e6), m_atmosphericLoss (0.5) {}
NtnFreeSpaceLossModel::~NtnFreeSpaceLossModel () {}

double NtnFreeSpaceLossModel::DoCalcRxPower (double txPowerDbm, Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    if (a == 0 || b == 0) return txPowerDbm;
    Vector posA, posB;
    try { posA = a->GetPosition (); posB = b->GetPosition (); }
    catch (...) { return txPowerDbm; }

    double dx = posA.x - posB.x, dy = posA.y - posB.y, dz = posA.z - posB.z;
    double distM = std::sqrt (dx*dx + dy*dy + dz*dz);
    if (distM < 1.0) distM = 1.0;

    double fsplDb = 20.0 * std::log10 (distM) + 20.0 * std::log10 (m_frequency) - 147.55;

    double groundDist = std::sqrt (dx*dx + dy*dy);
    double height = std::abs (dz);
    double elevRad = std::atan2 (height, groundDist);
    double sinElev = std::sin (elevRad);
    double atmoDb = (sinElev > 0.1) ? m_atmosphericLoss / sinElev : m_atmosphericLoss * 10.0;

    Vector velA (0,0,0), velB (0,0,0);
    try { velA = a->GetVelocity (); velB = b->GetVelocity (); }
    catch (...) { /* no velocity available — skip Doppler */ }
    double rvx = velA.x - velB.x, rvy = velA.y - velB.y, rvz = velA.z - velB.z;
    // Radial velocity = dot(relative_vel, range_unit) in km/s -> m/s
    double range = distM;
    double radialVelMS = 0.0;
    if (range > 1.0)
        radialVelMS = (rvx * dx + rvy * dy + rvz * dz) / range; // all in m/s and m

    // Doppler shift
    double cMS = 299792458.0;
    double dopplerHz = std::abs (radialVelMS) * m_frequency / cMS;

    // Assume UE pre-compensates ~90% of Doppler (from SIB31 ephemeris)
    double residualDopplerHz = dopplerHz * 0.1; // 10% residual

    // SINR penalty from residual Doppler (TR 38.821)
    // penalty = -10*log10(1 - (f_residual/subcarrier_spacing)^2)
    double subcarrierHz = 15000.0; // NB-IoT 15 kHz
    double ratio = residualDopplerHz / subcarrierHz;
    double dopplerPenaltyDb = 0.0;
    if (ratio < 0.99) // avoid log of negative
        dopplerPenaltyDb = -10.0 * std::log10 (1.0 - ratio * ratio);

    // NTN-MISS-23: Shadow fading (TR 38.811 Sec 6.6.2)
    // Simplified: log-normal shadow fading with std dev dependent on elevation
    // Low elevation → more fading, high elevation → less
    // For satellite links, typical std dev 2-4 dB (suburban/rural)
    double shadowFadingDb = 0.0; // placeholder — proper implementation needs RandomVariableStream
    // In a full implementation: shadowFadingDb = N(0, sigma) where sigma = 3.0 / sin(elevation)

    // NTN-MISS-22: Clutter loss (TR 38.811 Sec 6.6.4)
    // Additional loss from ground clutter at low elevation
    double clutterLossDb = 0.0;
    if (elevRad < 0.35) // below ~20 degrees
        clutterLossDb = 3.0; // simplified 3 dB clutter at low elevation

    // NTN-MISS-20: Scintillation (TR 38.811 Sec 6.6.6)
    // Ionospheric scintillation at L-band: typically < 1 dB for mid-latitudes
    double scintillationDb = 0.0; // placeholder

    // NTN-MISS-21: Rain attenuation (ITU-R P.618)
    // At 900 MHz (L-band), rain attenuation is negligible (< 0.1 dB)
    double rainDb = 0.0;

    return txPowerDbm - fsplDb - atmoDb - dopplerPenaltyDb - shadowFadingDb - clutterLossDb - scintillationDb - rainDb;
}

int64_t NtnFreeSpaceLossModel::DoAssignStreams (int64_t stream) { return 0; }

} // namespace ns3
