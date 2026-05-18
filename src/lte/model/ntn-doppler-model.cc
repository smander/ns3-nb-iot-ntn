#include "ntn-doppler-model.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NtnDopplerModel");
NS_OBJECT_ENSURE_REGISTERED (NtnDopplerModel);

TypeId NtnDopplerModel::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::NtnDopplerModel")
        .SetParent<Object> ()
        .SetGroupName ("Propagation")
        .AddConstructor<NtnDopplerModel> ();
    return tid;
}

NtnDopplerModel::NtnDopplerModel () {}
NtnDopplerModel::~NtnDopplerModel () {}

double NtnDopplerModel::GetDopplerShift (Ptr<MobilityModel> satellite, Ptr<MobilityModel> ue, double carrierFreqHz) const
{
    Vector satPos = satellite->GetPosition (), uePos = ue->GetPosition ();
    Vector satVel = satellite->GetVelocity ();
    double rx = satPos.x - uePos.x, ry = satPos.y - uePos.y, rz = satPos.z - uePos.z;
    double range = std::sqrt (rx*rx + ry*ry + rz*rz);
    if (range < 1e-6) return 0.0;
    double radialVelMS = (satVel.x*rx + satVel.y*ry + satVel.z*rz) / range * 1000.0;
    return -radialVelMS * carrierFreqHz / SPEED_OF_LIGHT_M_S;
}

} // namespace ns3
