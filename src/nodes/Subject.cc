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

    DBSCAN();

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

void Subject::DBSCAN(){
    int i, j,k, clusterID = 0;

    for(i = 0; i < uesVector.size(); i++){
        if(uesVector[i]->par("clusterID").intValue() != -1){
            continue;
        }

        std::vector<cModule *> vizinhosCluster = vizinhos(uesVector[i]);
        if(vizinhosCluster.size() < par("minUEs").intValue()){
            uesVector[i]->par("possuiCluster") = false;
            continue;
        }

        clusterID++;
        uesVector[i]->par("clusterID") = clusterID;

        for(j = 0; j < vizinhosCluster.size(); j++){
            if(vizinhosCluster[j]->par("possuiCluster").boolValue() == false){
                vizinhosCluster[j]->par("possuiCluster") = true;
                vizinhosCluster[j]->par("clusterID") = clusterID;
            }
            if(vizinhosCluster[j]->par("clusterID").intValue() == -1){
                continue;
            }
            vizinhosCluster[j]->par("possuiCluster") = true;
            vizinhosCluster[j]->par("clusterID") = clusterID;

            std::vector<cModule *> vizinhosCluster2 = vizinhos(vizinhosCluster[j]);
            if(vizinhosCluster2.size() >= par("minUEs").intValue()){

                for(k = 0; k < vizinhosCluster2.size(); k++){
                    auto it = find_if(vizinhosCluster.begin(), vizinhosCluster.end(), [&vizinhosCluster2, k](cModule *obj){return obj->par("numero").stringValue() == vizinhosCluster2[k]->par("numero").stringValue();});

                    if(it == vizinhosCluster.end()){
                        vizinhosCluster.push_back(vizinhosCluster2[k]);
                    }
                }
            }
        }
    }
}

std::vector<cModule *> Subject::vizinhos(cModule *it){
    std::vector<cModule *> vizinhosCluster;
    int i;

    for(i = 0; i < uesVector.size(); i++){
        if(calculoCorrelacao(it, uesVector[i]) <= epsilon){
            vizinhosCluster.push_back(uesVector[i]);
        }
    }

    return vizinhosCluster;
}

int Subject::calculoCorrelacao(cModule *it, cModule *iter){
    return (rand() % 8);
}
