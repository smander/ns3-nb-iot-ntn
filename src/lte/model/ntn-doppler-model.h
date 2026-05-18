#ifndef NTN_DOPPLER_MODEL_H
#define NTN_DOPPLER_MODEL_H

#include "ns3/object.h"
#include "ns3/mobility-model.h"

namespace ns3 {

class NtnDopplerModel : public Object
{
public:
    static TypeId GetTypeId (void);
    NtnDopplerModel ();
    virtual ~NtnDopplerModel ();
    double GetDopplerShift (Ptr<MobilityModel> satellite, Ptr<MobilityModel> ue, double carrierFreqHz) const;
private:
    static constexpr double SPEED_OF_LIGHT_M_S = 299792458.0;
};

} // namespace ns3
#endif
