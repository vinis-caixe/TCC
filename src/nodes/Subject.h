#ifndef SUBJECT_H_
#define SUBJECT_H_

#include <omnetpp.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <iostream>

#define CENTRO 1
#define BORDA 0
#define NAOIDENTIFICADO -1
#define RUIDO -2

using namespace omnetpp;

struct cluster{
    int clusterID;
    std::vector<cModule *> ues;
};

class Subject : public cSimpleModule{
protected:
    simtime_t inicioSim = simTime(); // Tempo inicial
    simtime_t finalSim = 10.0;       // Tempo final, mudar de acordo com experimento

    unsigned int quantUe = 0;

    cModuleType *moduleType = cModuleType::get("simu5g.nodes.UeExtended"); // Modulo que sera criado dinamicamente

    std::vector<cModule *> uesVector;

    cPoisson *testePoisson = new cPoisson(getRNG(0), 6.0); // Determina numero aleatorio de usuarios que irao entrar/sair

    int epsilon = 1;
    std::vector<cluster> clusters;

public:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void iniciarContador();
    void adicionarUes();
    void removerUes();

    void DBSCAN();
    std::vector<cModule *> vizinhos(cModule *it);
    int calculoCorrelacao(cModule *it, cModule *iter);

    void adicionarUeDBSCAN(cModule *ue);
    void removerUeDBSCAN(cModule *ue);
};

#endif //SUBJECT_H_
