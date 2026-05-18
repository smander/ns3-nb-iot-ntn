#include "ntn-propagation-delay-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NtnPropagationDelayModel");
NS_OBJECT_ENSURE_REGISTERED (NtnPropagationDelayModel);

TypeId NtnPropagationDelayModel::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::NtnPropagationDelayModel")
        .SetParent<PropagationDelayModel> ()
        .SetGroupName ("Propagation")
        .AddConstructor<NtnPropagationDelayModel> ();
    return tid;
}

NtnPropagationDelayModel::NtnPropagationDelayModel () {}
NtnPropagationDelayModel::~NtnPropagationDelayModel () {}

Time NtnPropagationDelayModel::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    Vector posA = a->GetPosition ();
    Vector posB = b->GetPosition ();
    double dx = posA.x - posB.x, dy = posA.y - posB.y, dz = posA.z - posB.z;
    double distKm = std::sqrt (dx*dx + dy*dy + dz*dz);
    return Seconds (distKm / SPEED_OF_LIGHT_KM_S);
}

int64_t NtnPropagationDelayModel::DoAssignStreams (int64_t stream) { return 0; }

} // namespace ns3
