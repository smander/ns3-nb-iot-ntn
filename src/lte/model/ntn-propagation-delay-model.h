#ifndef NTN_PROPAGATION_DELAY_MODEL_H
#define NTN_PROPAGATION_DELAY_MODEL_H

#include "ns3/propagation-delay-model.h"

namespace ns3 {

class NtnPropagationDelayModel : public PropagationDelayModel
{
public:
    static TypeId GetTypeId (void);
    NtnPropagationDelayModel ();
    virtual ~NtnPropagationDelayModel ();
    virtual Time GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
    virtual int64_t DoAssignStreams (int64_t stream);
private:
    static constexpr double SPEED_OF_LIGHT_KM_S = 299792.458;
};

} // namespace ns3
#endif
