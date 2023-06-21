#include "stack/phy/ChannelModel/TccChannelModel.h"

#include "stack/phy/layer/LtePhyUe.h"
#include "nodes/Subject.h"

Define_Module(TccChannelModel);

void TccChannelModel::initialize(int stage){
    NRChannelModel_3GPP38_901::initialize(stage);
}

std::vector<double> TccChannelModel::getSINR(LteAirFrame *frame, UserControlInfo* lteInfo){

   if (useRsrqFromLog_)
   {
       int time = -1, rsrq = oldRsrq_;
       double currentTime = simTime().dbl();
       if (currentTime > oldTime_+1)
       {
           std::ifstream file;

           // open the rsrq file
           file.clear();
           file.open("rsrqFile.dat");

           file >> time;
           file >> rsrq;

           file.close();

           oldTime_ = simTime().dbl();
           oldRsrq_ = rsrq;

           std::cout << "LteRealisticChannelModel::getSINR - time["<<time<<"] rsrq["<<rsrq<<"]" << endl;
       }

       double sinr = rsrqScale_ * (rsrq + rsrqShift_);
       std::vector<double> snrVector;
       snrVector.resize(numBands_, sinr);

       return snrVector;
   }

   //get tx power
   double recvPower = lteInfo->getTxPower(); // dBm

   //Get the Resource Blocks used to transmit this packet
   RbMap rbmap = lteInfo->getGrantedBlocks();

   //get move object associated to the packet
   //this object is refereed to eNodeB if direction is DL or UE if direction is UL
   Coord coord = lteInfo->getCoord();

   // position of eNb and UE
   Coord ueCoord;
   Coord enbCoord;

   double antennaGainTx = 0.0;
   double antennaGainRx = 0.0;
   double noiseFigure = 0.0;
   double speed = 0.0;

   // true if we are computing a CQI for the DL direction
   bool cqiDl = false;

   MacNodeId ueId = 0;
   MacNodeId eNbId = 0;

   Direction dir = (Direction) lteInfo->getDirection();

   EV << "------------ GET SINR ----------------" << endl;
   //===================== PARAMETERS SETUP ============================
   /*
    * if direction is DL and this is not a feedback packet,
    * this function has been called by LteRealisticChannelModel::error() in the UE
    *
    *         DownLink error computation
    */
   if (dir == DL && (lteInfo->getFrameType() != FEEDBACKPKT))
   {

       //set noise Figure
       noiseFigure = ueNoiseFigure_; //dB
       //set antenna gain Figure
       antennaGainTx = antennaGainEnB_; //dB
       antennaGainRx = antennaGainUe_;  //dB

       // get MacId for Ue and eNb
       ueId = lteInfo->getDestId();
       eNbId = lteInfo->getSourceId();

       // get position of Ue and eNb
       ueCoord = phy_->getCoord();
       enbCoord = lteInfo->getCoord();

       speed = computeSpeed(ueId, phy_->getCoord());
   }
   /*
    * If direction is UL OR
    * if the packet is a feedback packet
    * it means that this function is called by the feedback computation module
    *
    * located in the eNodeB that compute the feedback received by the UE
    * Hence the UE macNodeId can be taken by the sourceId of the lteInfo
    * and the speed of the UE is contained by the Move object associated to the lteinfo
    */
   else // UL/DL CQI & UL error computation
   {

       // get MacId for Ue and eNb
       ueId = lteInfo->getSourceId();
       eNbId = lteInfo->getDestId();

       if (dir == DL)
       {
           //set noise Figure
           noiseFigure = ueNoiseFigure_; //dB
           //set antenna gain Figure
           antennaGainTx = antennaGainEnB_; //dB
           antennaGainRx = antennaGainUe_;  //dB

           // use the jakes map in the UE side
           cqiDl = true;
       }
       else // if( dir == UL )
       {

           // TODO check if antennaGainEnB should be added in UL direction too
           antennaGainTx = antennaGainUe_;
           antennaGainRx = antennaGainEnB_;
           noiseFigure = bsNoiseFigure_;

           // use the jakes map in the eNb side
           cqiDl = false;
       }
       speed = computeSpeed(ueId, coord);

       // get position of Ue and eNb
       ueCoord = coord;
       enbCoord = phy_->getCoord();
   }

   CellInfo* eNbCell = getCellInfo(eNbId);
   const char* eNbTypeString = eNbCell ? (eNbCell->getEnbType() == MACRO_ENB ? "MACRO" : "MICRO") : "NULL";

   EV << "TccChannelModel::getSINR - srcId=" << lteInfo->getSourceId()
                      << " - destId=" << lteInfo->getDestId()
                      << " - DIR=" << (( dir==DL )?"DL" : "UL")
                      << " - frameType=" << ((lteInfo->getFrameType()==FEEDBACKPKT)?"feedback":"other")
                      << endl
                      << eNbTypeString << " - txPwr " << lteInfo->getTxPower()
                      << " - ueCoord[" << ueCoord << "] - enbCoord[" << enbCoord << "] - ueId[" << ueId << "] - enbId[" << eNbId << "]" <<
                      endl;
   //=================== END PARAMETERS SETUP =======================

   //=============== PATH LOSS + SHADOWING + FADING =================
   EV << "\t using parameters - noiseFigure=" << noiseFigure << " - antennaGainTx=" << antennaGainTx << " - antennaGainRx=" << antennaGainRx <<
           " - txPwr=" << lteInfo->getTxPower() << " - for ueId=" << ueId << endl;

   // attenuation for the desired signal
   double attenuation;
   if ((lteInfo->getFrameType() == FEEDBACKPKT))
       attenuation = getAttenuation(ueId, UL, coord, cqiDl); // dB
   else
       attenuation = getAttenuation(ueId, dir, coord, cqiDl); // dB

   //compute attenuation (PATHLOSS + SHADOWING)
   recvPower -= attenuation; // (dBm-dB)=dBm

   //add antenna gain
   recvPower += antennaGainTx; // (dBm+dB)=dBm
   recvPower += antennaGainRx; // (dBm+dB)=dBm

   //sub cable loss
   recvPower -= cableLoss_; // (dBm-dB)=dBm


   //=============== Ganho de agrupamento ================
   if(dir == DL){
       recvPower += ganhoCluster(ueCoord, enbCoord);
   }


   //=============== ANGOLAR ATTENUATION =================
   if (dir == DL)
   {
       //get tx angle
       omnetpp::cModule* eNbModule = getSimulation()->getModule(binder_->getOmnetId(eNbId));
       LtePhyBase* ltePhy = eNbModule ?
          check_and_cast<LtePhyBase*>(eNbModule->getSubmodule("cellularNic")->getSubmodule("phy")) :
          nullptr;

       if (ltePhy && ltePhy->getTxDirection() == ANISOTROPIC)
       {
           // get tx angle
           double txAngle = ltePhy->getTxAngle();

           // compute the angle between uePosition and reference axis, considering the eNb as center
           double ueAngle = computeAngle(enbCoord, ueCoord);

           // compute the reception angle between ue and eNb
           double recvAngle = fabs(txAngle - ueAngle);

           if (recvAngle > 180)
               recvAngle = 360 - recvAngle;

           double verticalAngle = computeVerticalAngle(enbCoord, ueCoord);

           // compute attenuation due to sectorial tx
           double angolarAtt = computeAngolarAttenuation(recvAngle,verticalAngle);

           recvPower -= angolarAtt;
       }
       // else, antenna is omni-directional
   }
   //=============== END ANGOLAR ATTENUATION =================


   std::vector<double> snrVector;
   snrVector.resize(numBands_, 0.0);

   // compute and add interference due to fading
   // Apply fading for each band
   // if the phy layer is localized we can assume that for each logical band we have different fading attenuation
   // if the phy layer is distributed the number of logical band should be set to 1
   double fadingAttenuation = 0;

   // for each logical band
   // FIXME compute fading only for used RBs
   for (unsigned int i = 0; i < numBands_; i++)
   {
       fadingAttenuation = 0;
       //if fading is enabled
       if (fading_)
       {
           //Appling fading
           if (fadingType_ == RAYLEIGH)
               fadingAttenuation = rayleighFading(ueId, i);

           else if (fadingType_ == JAKES)
               fadingAttenuation = jakesFading(ueId, speed, i, cqiDl);
       }
       // add fading contribution to the received pwr
       double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

       //if txmode is multi user the tx power is dived by the number of paired user
       // in db divede by 2 means -3db
       if (lteInfo->getTxMode() == MULTI_USER)
       {
           finalRecvPower -= 3;
       }

       EV << " TccChannelModel::getSINR node " << ueId
          << ((lteInfo->getFrameType() == FEEDBACKPKT) ?
           " FEEDBACK PACKET " : " NORMAL PACKET ")
          << " band " << i << " recvPower " << recvPower
          << " direction " << dirToA(dir) << " antenna gain tx "
          << antennaGainTx << " antenna gain rx " << antennaGainRx
          << " noise figure " << noiseFigure
          << " cable loss   " << cableLoss_
          << " attenuation (pathloss + shadowing) " << attenuation
          << " speed " << speed << " thermal noise " << thermalNoise_
          << " fading attenuation " << fadingAttenuation << endl;

       snrVector[i] = finalRecvPower;
   }
   //============ END PATH LOSS + SHADOWING + FADING ===============

   /*
    * The SINR will be calculated as follows
    *
    *              Pwr
    * SINR = ---------
    *           N  +  I
    *
    * Ndb = thermalNoise_ + noiseFigure (measured in decibel)
    * I = extCellInterference + multiCellInterference
    */

   //============ MULTI CELL INTERFERENCE COMPUTATION =================
   //vector containing the sum of multicell interference for each band
   std::vector<double> multiCellInterference; // Linear value (mW)
   // prepare data structure
   multiCellInterference.resize(numBands_, 0);
   if (enableDownlinkInterference_ && dir == DL && lteInfo->getFrameType() != HANDOVERPKT)
   {
       computeDownlinkInterference(eNbId, ueId, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, &multiCellInterference);
   }
   else if (enableUplinkInterference_ && dir == UL)
   {
       computeUplinkInterference(eNbId, ueId, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, &multiCellInterference);
   }

   //============ BACKGROUND CELLS INTERFERENCE COMPUTATION =================
   //vector containing the sum of bg-cell interference for each band
   std::vector<double> bgCellInterference; // Linear value (mW)
   // prepare data structure
   bgCellInterference.resize(numBands_, 0);
   if (enableBackgroundCellInterference_)
   {
       computeBackgroundCellInterference(ueId, enbCoord, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, dir, &bgCellInterference); // dBm
   }

   //============ EXTCELL INTERFERENCE COMPUTATION =================
   // TODO this might be obsolete as it is replaced by background cell interference
   //vector containing the sum of ext-cell interference for each band
   std::vector<double> extCellInterference; // Linear value (mW)
   // prepare data structure
   extCellInterference.resize(numBands_, 0);
   if (enableExtCellInterference_ && dir == DL)
   {
       computeExtCellInterference(eNbId, ueId, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), &extCellInterference); // dBm
   }

   //===================== SINR COMPUTATION ========================
   // compute and linearize total noise
   double totN = dBmToLinear(thermalNoise_ + noiseFigure);

   // denominator expressed in dBm as (N+extCell+bgCell+multiCell)
   double den;
   EV << "LteRealisticChannelModel::getSINR - distance from my eNb=" << enbCoord.distance(ueCoord) << " - DIR=" << (( dir==DL )?"DL" : "UL") << endl;


   double sumSnr = 0.0;
   int usedRBs = 0;
   // add interference for each band
   for (unsigned int i = 0; i < numBands_; i++)
   {
       // if we are decoding a data transmission and this RB has not been used, skip it
       // TODO fix for multi-antenna case
       if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
           continue;

       //               (      mW              +          mW            +  mW  +        mW            )
       den = linearToDBm(bgCellInterference[i] + extCellInterference[i] + totN + multiCellInterference[i]);

       EV << "\t bgCell[" << bgCellInterference[i] << "] - ext[" << extCellInterference[i] << "] - multi[" << multiCellInterference[i] << "] - recvPwr["
          << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i]-den << "]\n";

       // compute final SINR
       snrVector[i] -= den;

       sumSnr += snrVector[i];
       ++usedRBs;
   }

   // emit SINR statistic
   if (collectSinrStatistics_ && (lteInfo->getFrameType() == FEEDBACKPKT) && usedRBs > 0)
   {
       // we are on the BS, so we need to retrieve the channel model of the sender
       // XXX I know, there might be a faster way...
       LteChannelModel* ueChannelModel = check_and_cast<LtePhyUe*>(getPhyByMacNodeId(ueId))->getChannelModel(lteInfo->getCarrierFrequency());

       if (dir == DL) // we are on the UE
           ueChannelModel->emit(measuredSinrDl_, sumSnr / usedRBs);
       else
           ueChannelModel->emit(measuredSinrUl_, sumSnr / usedRBs);
   }

   //if sender is an eNodeB
   if (dir == DL)
       //store the position of user
       updatePositionHistory(ueId, phy_->getCoord());
   //sender is an UE
   else
       updatePositionHistory(ueId, coord);
   return snrVector;
}

double TccChannelModel::ganhoCluster(Coord ueCoord, Coord enbCoord){
    int i, clusterID;
    double direcaoUE;
    std::vector<double> angulos;

    cModule *mod = getSimulation()->getModuleByPath("subject");
    Subject *subject = check_and_cast<Subject*>(mod);
    std::vector<cluster> clusters = subject->getClusters();
    std::vector<cModule *> ues = subject->getUes();

    IMobility *mod2;
    Coord coordUE;

    // UE no vetor Subject possui diferenca de posicao da sua versao na simulacao de cerca de 0.1m
    for(i = 0; i < ues.size(); i++){
        mod2 = check_and_cast<IMobility*>(ues[i]->getSubmodule("mobility"));
        coordUE = mod2->getCurrentPosition();

        if((fabs(coordUE.x - ueCoord.x) <= 0.1) && (fabs(coordUE.y - ueCoord.y) <= 0.1)){
            break;
        }
    }

    clusterID = ues[i]->par("clusterID").intValue();

    auto it = find_if(clusters.begin(), clusters.end(), [clusterID](cluster a){return a.clusterID == clusterID;});

    // Se UE nao pertencer a nenhum grupo nao ocorre ganho
    if(it == clusters.end()){
        return 0.0;
    }

    // Obtem angulo de todos os UEs do grupo em relacao a BS
    for(i = 0; i < it->ues.size(); i++){
        mod2 = check_and_cast<IMobility*>(it->ues[i]->getSubmodule("mobility"));
        coordUE = mod2->getCurrentPosition();
        direcaoUE = atan2(((1000.0 - coordUE.y) - enbCoord.y), (coordUE.x - enbCoord.x)) * 180 / PI;
        if(direcaoUE < 0){
            direcaoUE += 360.0;
        }
        angulos.push_back(direcaoUE);
    }

    sort(angulos.begin(), angulos.end());

    double maiorAngulo = 0.0;

    // Obtem maior angulo entre angulos
    for(i = 1; i < angulos.size(); i++){
        if((angulos[i] - angulos[i - 1]) > maiorAngulo){
            maiorAngulo = (angulos[i] - angulos[i - 1]);
        }
    }
    if(360.0 - (angulos.back() - angulos[0]) > maiorAngulo){
        maiorAngulo = 360.0 - (angulos.back() - angulos[0]);
    }

    // Menor angulo que passa por todos os UEs do grupo
    double menorAngulo = 360.0 - maiorAngulo;

    // Caso angulo seja menor que 60, UEs possuem canais similares e portanto ocorre ganho
    if(menorAngulo <= 60.0){
        return (30.0 - (0.5 * menorAngulo));
    } // Caso contrario ocorre perda
    else{
        return ((-0.1 * menorAngulo) + 6.0);
    }
}
