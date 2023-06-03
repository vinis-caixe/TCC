#ifndef TCCCHANNELMODEL_H_
#define TCCCHANNELMODEL_H_

#include "stack/phy/ChannelModel/NRChannelModel_3GPP38_901.h"

class TccChannelModel : public NRChannelModel_3GPP38_901{

public:
    virtual void initialize(int stage);

    virtual std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo* lteInfo);

    double ganhoCluster(Coord ueCoord, Coord enbCoord);

};

#endif /* TCCCHANNELMODEL_H_ */
