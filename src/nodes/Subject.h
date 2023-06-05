#ifndef SUBJECT_H_
#define SUBJECT_H_

#include <omnetpp.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cmath>
#include <math.h>
#include "inet/common/geometry/common/Coord.h"
#include "inet/mobility/contract/IMobility.h"

#define CENTRO 1
#define BORDA 0
#define NAOIDENTIFICADO -1
#define RUIDO -2

#define PI 3.1415

using namespace omnetpp;

struct cluster{
    int clusterID;
    std::vector<cModule *> ues;
};

class Subject : public cSimpleModule{
protected:
    simtime_t inicioSim = simTime(); // Tempo inicial
    simtime_t finalSim = SimTime::parse(getEnvir()->getConfig()->getConfigValue("sim-time-limit")); // Tempo final de acordo com arquivo .ini

    unsigned int quantUe = 0;

    cModuleType *moduleType = cModuleType::get("simu5g.nodes.UeExtended"); // Modulo que sera criado dinamicamente

    std::vector<cModule *> uesVector;

    cPoisson *testePoisson = new cPoisson(getRNG(0), 6.0); // Determina numero aleatorio de usuarios que irao entrar/sair

    double epsilon = 15.0;
    std::vector<cluster> clusters;

public:
    ~Subject();
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void iniciarContador();
    void adicionarUes();
    void removerUes();

    void DBSCAN();
    std::vector<cModule *> vizinhos(cModule *it);
    double calculoCorrelacao(cModule *it, cModule *iter);

    void adicionarUeDBSCAN(cModule *ue);
    void removerUeDBSCAN(cModule *ue);

    std::vector<cluster> getClusters(){ return clusters; }
    std::vector<cModule *> getUes(){ return uesVector; }
};

#endif //SUBJECT_H_
