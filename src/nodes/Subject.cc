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

    if(!std::strcmp(msg->getName(), "DBSCAN")){
        DBSCAN();
    }
    // Mensagem OUT remove usuarios
    if(!std::strcmp(msg->getName(), "OUT")){
        removerUes();
    }

    delete msg;

}

void Subject::iniciarContador(){

    int i;
    simtime_t tempoMetade = finalSim / 2.0;
    simtime_t tempoPeriodo = tempoMetade / 10.0; // Periodo para adicionar/remover usuarios
    simtime_t tempoDBSCAN = tempoPeriodo / 2.0;

    for(i = 0; i < 10; i++){
        scheduleAt((inicioSim), new cMessage("IN"));
        inicioSim += tempoPeriodo;
    }

    for(i = 0; i < 19; i++){
        scheduleAt((tempoDBSCAN), new cMessage("DBSCAN"));
        tempoDBSCAN += tempoPeriodo;
    }

    for(i = 0; i < 10; i++){
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

        adicionarUeDBSCAN(module);

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

void Subject::DBSCAN(){
    int i, j, k, clusterID = -1;
    clusters.clear();

    for(i = 0; i < uesVector.size(); i++){
        uesVector[i]->par("clusterID") = -1;
        uesVector[i]->par("pontoTipo") = NAOIDENTIFICADO;
    }

    for(i = 0; i < uesVector.size(); i++){
        // Verifica se UE ja foi identificado em um cluster
        if(uesVector[i]->par("pontoTipo").intValue() != NAOIDENTIFICADO){
            continue;
        }

        // Verifica se UE eh possivel ruido
        std::vector<cModule *> vizinhosCluster = vizinhos(uesVector[i]);
        if(vizinhosCluster.size() < par("minUEs").intValue()){
            uesVector[i]->par("pontoTipo") = RUIDO;
            continue;
        }

        // Ponto eh centro de cluster
        clusterID++;
        uesVector[i]->par("clusterID") = clusterID;
        uesVector[i]->par("pontoTipo") = CENTRO;

        clusters.push_back({clusterID, {uesVector[i]}});

        // Verifica se vizinhos sao pontos de borda ou centro
        for(j = 0; j < vizinhosCluster.size(); j++){
            if(vizinhosCluster[j]->par("pontoTipo").intValue() == RUIDO){
                vizinhosCluster[j]->par("clusterID") = clusterID;
                uesVector[j]->par("pontoTipo") = BORDA;
            }
            if(vizinhosCluster[j]->par("pontoTipo").intValue() != NAOIDENTIFICADO){
                continue;
            }
            vizinhosCluster[j]->par("clusterID") = clusterID;

            auto it = find_if(clusters.begin(), clusters.end(), [clusterID](cluster a){return a.clusterID == clusterID;});

            it->ues.push_back(uesVector[j]);

            // Realiza mesmo processo para vizinho dos vizinhos
            std::vector<cModule *> vizinhosCluster2 = vizinhos(vizinhosCluster[j]);
            if(vizinhosCluster2.size() >= par("minUEs").intValue()){
                vizinhosCluster[j]->par("pontoTipo") = CENTRO;

                for(k = 0; k < vizinhosCluster2.size(); k++){
                    auto it = find_if(vizinhosCluster.begin(), vizinhosCluster.end(), [&vizinhosCluster2, k](cModule *obj){return obj->par("numero").stringValue() == vizinhosCluster2[k]->par("numero").stringValue();});

                    if(it == vizinhosCluster.end()){
                        vizinhosCluster.push_back(vizinhosCluster2[k]);
                    }
                }
            }
            else{
                vizinhosCluster[j]->par("pontoTipo") = BORDA;
            }

            // Caso o numero maximo de UEs ultrapasse, novo processo comeca para novo cluster
            auto iter = find_if(clusters.begin(), clusters.end(), [clusterID](cluster a){return a.clusterID == clusterID;});
            if(iter->ues.size() >= par("maxUEs").intValue()){
                break;
            }
        }
    }
}

std::vector<cModule *> Subject::vizinhos(cModule *it){
    int clusterID;
    std::vector<cModule *> vizinhosCluster;

    // Encontra quais sao os vizinhos do UE de acordo com a feature space
    for(int i = 0; i < uesVector.size(); i++){
        clusterID = uesVector[i]->par("clusterID").intValue();
        auto iter = find_if(clusters.begin(), clusters.end(), [clusterID](cluster a){return a.clusterID == clusterID;});

        // UEs que estao em clusters que possui quantidade maxima de UEs sao desconsiderados como vizinhos
        if(iter != clusters.end() && iter->ues.size() >= par("maxUEs").intValue()){
            continue;
        }

        if(it->par("numero").stringValue() != uesVector[i]->par("numero").stringValue() && calculoCorrelacao(it, uesVector[i]) <= epsilon){
            vizinhosCluster.push_back(uesVector[i]);
        }
    }

    return vizinhosCluster;
}

int Subject::calculoCorrelacao(cModule *it, cModule *iter){
    // TODO: implementar feature space correta - noise path loss NOMA

    int a = std::stoi(it->par("numero").stringValue());
    int b = std::stoi(iter->par("numero").stringValue());
    a = a % 2;
    b = b % 2;
    int c = a - b;

    if(c == 0){
        return 0;
    }

    return 3;
}

void Subject::adicionarUeDBSCAN(cModule *ue){
    int i, j, k, n, clusterID;

    std::vector<cModule *> vizinhosCluster = vizinhos(ue);
    std::vector<cModule *> vizinhosCluster2;

    if(vizinhosCluster.size() == 0){
        ue->par("pontoTipo") = RUIDO;
        return;
    }

    std::vector<int> possuiCentro; // UEs vizinhos que sao pontos de centro
    std::vector<int> possuiBorda; // UEs vizinhos que sao pontos de borda

    for(i = 0; i < vizinhosCluster.size(); i++){
        if(vizinhosCluster[i]->par("pontoTipo").intValue() == CENTRO){
            possuiCentro.push_back(vizinhosCluster[i]->par("clusterID").intValue());
        }
        if(vizinhosCluster[i]->par("pontoTipo").intValue() == BORDA){
            possuiBorda.push_back(vizinhosCluster[i]->par("clusterID").intValue());
        }
    }

    if(vizinhosCluster.size() < par("minUEs").intValue()){

        if(possuiCentro.size() == 0){
            ue->par("pontoTipo") = RUIDO;
            return;
        }

        auto it = find_if(clusters.begin(), clusters.end(), [possuiCentro](cluster a){return a.clusterID == possuiCentro[0];});

        ue->par("pontoTipo") = BORDA;
        ue->par("clusterID") = it->clusterID;
        it->ues.push_back(ue);

        for(i = 0; i < it->ues.size(); i++){
            if(it->ues[i]->par("pontoTipo").intValue() == BORDA){
                vizinhosCluster2 = vizinhos(it->ues[i]);
                if(vizinhosCluster2.size() >= par("minUEs").intValue()){
                    it->ues[i]->par("pontoTipo") = CENTRO;
                    for(j = 0; j < vizinhosCluster2.size(); j++){
                        if(vizinhosCluster2[j]->par("pontoTipo").intValue() == RUIDO && it->ues.size() < par("maxUEs").intValue()){
                            vizinhosCluster2[j]->par("pontoTipo") = BORDA;
                            vizinhosCluster2[j]->par("clusterID") = it->clusterID;
                            it->ues.push_back(vizinhosCluster2[j]);
                        }
                    }
                }
            }
        }

        return;
    }
    else{
        if((vizinhosCluster.size() - possuiCentro.size() - possuiBorda.size()) >= par("minUEs").intValue()){
            if(clusters.size() != 0){
                n = clusters.size();
                clusterID = clusters[n - 1].clusterID;
            }
            else{
                clusterID = -1;
            }

            clusterID++;
            ue->par("clusterID") = clusterID;
            ue->par("pontoTipo") = CENTRO;
            clusters.push_back({clusterID, {ue}});

            n = clusters.size();

            for(i = 0; i < vizinhosCluster.size(); i++){

                if(vizinhosCluster[i]->par("pontoTipo").intValue() == RUIDO){
                    vizinhosCluster[i]->par("clusterID") = clusterID;
                    vizinhosCluster[i]->par("pontoTipo") = BORDA;

                    clusters[n - 1].ues.push_back(vizinhosCluster[i]);

                    vizinhosCluster2 = vizinhos(vizinhosCluster[i]);
                    if(vizinhosCluster2.size() >= par("minUEs").intValue()){
                        vizinhosCluster[i]->par("pontoTipo") = CENTRO;
                        for(k = 0; k < vizinhosCluster2.size(); k++){
                            if(vizinhosCluster2[k]->par("pontoTipo").intValue() == RUIDO && clusters[n - 1].ues.size() < par("maxUEs").intValue()){
                                auto it = find_if(vizinhosCluster.begin(), vizinhosCluster.end(), [&vizinhosCluster2, k](cModule *obj){return obj->par("numero").stringValue() == vizinhosCluster2[k]->par("numero").stringValue();});

                                if(it == vizinhosCluster.end()){
                                    vizinhosCluster2[k]->par("pontoTipo") = BORDA;
                                    vizinhosCluster2[k]->par("clusterID") = clusterID;
                                    clusters[n - 1].ues.push_back(vizinhosCluster2[k]);
                                }

                            }
                        }
                    }

                    if(clusters[n - 1].ues.size() >= par("maxUEs").intValue()){
                        return;
                    }
                }
            }

            return;
        }

        int maior = -1, contador = 0, maiorContador = 0;
        std::sort(possuiCentro.begin(), possuiCentro.end());

        for(i = 0; i < possuiCentro.size(); i++){
            if(i == 0){
                maior = possuiCentro[i];
                contador++;
                maiorContador = contador;
            }
            else{
                if(possuiCentro[i] == possuiCentro[i-1]){
                    contador++;
                }
                else{
                    contador = 1;
                }

                if(contador > maiorContador){
                    maior = possuiCentro[i];
                    maiorContador = contador;
                }
            }
        }

        if(maiorContador >= par("minUEs").intValue()){
            auto it = find_if(clusters.begin(), clusters.end(), [maior](cluster a){return a.clusterID == maior;});
            ue->par("clusterID") = maior;
            ue->par("pontoTipo") = CENTRO;
            it->ues.push_back(ue);

            if(it->ues.size() >= par("maxUEs").intValue()){
                return;
            }

            for(i = 0; i < vizinhosCluster.size(); i++){
                if(vizinhosCluster[i]->par("pontoTipo").intValue() == RUIDO && it->ues.size() < par("maxUEs").intValue()){
                    vizinhosCluster[i]->par("pontoTipo") = BORDA;
                    vizinhosCluster[i]->par("clusterID") = maior;
                    it->ues.push_back(vizinhosCluster[i]);
                }
            }

            return;
        }

        maior = -1;
        contador = 0;
        maiorContador = 0;
        std::sort(possuiBorda.begin(), possuiBorda.end());

        for(i = 0; i < possuiBorda.size(); i++){
            if(i == 0){
                maior = possuiBorda[i];
                contador++;
                maiorContador = contador;
            }
            else{
                if(possuiBorda[i] == possuiBorda[i-1]){
                    contador++;
                }
                else{
                    contador = 1;
                }

                if(contador > maiorContador){
                    maior = possuiBorda[i];
                    maiorContador = contador;
                }
            }
        }

        if(maiorContador >= par("minUEs").intValue()){
            auto it = find_if(clusters.begin(), clusters.end(), [maior](cluster a){return a.clusterID == maior;});
            ue->par("clusterID") = maior;
            ue->par("pontoTipo") = CENTRO;
            it->ues.push_back(ue);

            if(it->ues.size() >= par("maxUEs").intValue()){
                return;
            }

            for(i = 0; i < vizinhosCluster.size(); i++){
                if(vizinhosCluster[i]->par("pontoTipo").intValue() == RUIDO && it->ues.size() < par("maxUEs").intValue()){
                    vizinhosCluster[i]->par("pontoTipo") = BORDA;
                    vizinhosCluster[i]->par("clusterID") = maior;
                    it->ues.push_back(vizinhosCluster[i]);
                }
            }

            return;
        }

        if(possuiCentro.size() != 0){
            ue->par("clusterID") = possuiCentro[0];
            ue->par("pontoTipo") = BORDA;
            auto it = find_if(clusters.begin(), clusters.end(), [possuiCentro](cluster a){return a.clusterID == possuiCentro[0];});
            it->ues.push_back(ue);
            return;
        }

        ue->par("pontoTipo") = RUIDO;
    }
}
