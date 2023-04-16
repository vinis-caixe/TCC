#include "Subject.h"

using namespace omnetpp;

Define_Module(Subject);

void Subject::initialize(){

    iniciarContador();

}

void Subject::handleMessage(cMessage *msg){

    // Mensagem IN adiciona usuarios
    if(!std::strcmp(msg->getName(), "IN")){
        adicionarUes();
    }
    // Mensagem OUT remove usuarios
    if(!std::strcmp(msg->getName(), "OUT")){
        removerUes();
    }

    delete msg;

}

void Subject::iniciarContador(){

    simtime_t tempoMetade = finalSim / 2.0;
    simtime_t tempoPeriodo = tempoMetade / 10.0; // Periodo para adicionar/remover usuarios

    for(int i = 0; i < 10; i++){
        scheduleAt((inicioSim), new cMessage("IN"));
        inicioSim += tempoPeriodo;
    }

    for(int i = 0; i < 10; i++){
        scheduleAt((tempoMetade), new cMessage("OUT"));
        tempoMetade += tempoPeriodo;
    }

}

void Subject::adicionarUes(){
    std::string ueString = "ue";

    // Quantidade aleatoria de usuarios que serao adicionados
    int ues = testePoisson->draw();

    while(ues > 0 && quantUe < 60){
        // Cria dinamicamente usuario
        cModule *module = moduleType->create((ueString + std::to_string(quantUe)).c_str(), this->getParentModule());
        // Determina numero do usuario para routing table
        module->par("numero") = std::to_string(quantUe);
        // Procedimentos necessarios para iniciar usuario
        module->finalizeParameters();
        module->buildInside();
        module->scheduleStart(simTime());
        module->callInitialize();

        uesVector.push_back(module);

        quantUe++;
        ues--;
    }

}

void Subject::removerUes(){
    // Quantidade aleatoria de usuarios que serao removidos
    int ues = testePoisson->draw();

    while(ues > 0){
        if(quantUe < 1){
            break;
        }
        else{
            // Determina usuario aleatorio no vetor
            int indexVector = rand() % uesVector.size();
            // Procedimentos para remover dinamicamente usuario
            uesVector[indexVector]->callFinish();
            uesVector[indexVector]->deleteModule();
            uesVector.erase(uesVector.begin() + indexVector);
        }

        quantUe--;
        ues--;
    }
}
