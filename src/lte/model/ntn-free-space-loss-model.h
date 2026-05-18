#ifndef NTN_FREE_SPACE_LOSS_MODEL_H
#define NTN_FREE_SPACE_LOSS_MODEL_H

#include "ns3/propagation-loss-model.h"

namespace ns3 {

class NtnFreeSpaceLossModel : public PropagationLossModel
{
public:
    static TypeId GetTypeId (void);
    NtnFreeSpaceLossModel ();
    virtual ~NtnFreeSpaceLossModel ();
private:
    virtual double DoCalcRxPower (double txPowerDbm, Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
    virtual int64_t DoAssignStreams (int64_t stream);
    double m_frequency;
    double m_atmosphericLoss;
};

} // namespace ns3
#endif
