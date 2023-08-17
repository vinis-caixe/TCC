# Trabalho de Graduação (UnB) - Agrupamento DBSCAN de Usuários em Sistemas Millimiter-Wave NOMA
O trabalho utilizou o _framework_ OMNeT++ e o simulador Simu5G para realizar simulações de um sistema em que usuários entram, saem e se movimentam constantemente. Os usuários são agrupados de acordo com uma versão modificada do algoritmo _Density-based spatial clustering of applications with noise_ (DBSCAN) para realizar o esquema _Non-orthogonal multiple access_ (NOMA).

Para poder realizar as simulações é necessário instalar o [OMNeT++](https://omnetpp.org/), [INET](https://inet.omnetpp.org/) e o simulador [Simu5G](http://simu5g.org/install.html). O caminho dos arquivos na pasta /src deste repositório é equivalente ao caminho da pasta /src do Simu5G, enquanto os da pasta /simulations é necessário criar uma pasta no Simu5G no caminho /simulations/NR e colocar os arquivos deste repositório nesta nova pasta.

**Atenção:** A simulação padrão faz um estudo de parâmetro entre dois _channel models_, repetindo a simulação 150 vezes por parâmetro. Para fazer com que a simulação não seja repetida é necessário modificar o arquivo /simulations/NR/_pastaCriada_/omnetpp.ini, mudando o "repeat = 150" por "repeat = 1".
- Aluno: Vinícius Caixeta de Souza
- Matrícula: 180132199