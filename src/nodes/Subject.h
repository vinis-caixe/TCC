#ifndef SUBJECT_H_
#define SUBJECT_H_

#include <omnetpp.h>
#include <vector>
#include <string>
#include <random>

using namespace omnetpp;

class Subject : public cSimpleModule{
protected:
    simtime_t inicioSim = simTime(); // Tempo inicial
    simtime_t finalSim = 10.0;       // Tempod final, mudar de acordo com experimento

    unsigned int quantUe = 0;

    cModuleType *moduleType = cModuleType::get("simu5g.nodes.UeExtended");

    std::vector<cModule *> uesVector;

    cPoisson *testePoisson = new cPoisson(getRNG(0), 6.0); // Determina numero aleatorio de usuarios que irao entrar/sair

public:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void iniciarContador();
    void adicionarUes();
    void removerUes();
};

#endif //SUBJECT_H_
