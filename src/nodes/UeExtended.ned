//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package simu5g.nodes;
import simu5g.nodes.NR.NRUe;

module UeExtended extends NRUe{
	parameters:
	    
	    // Routing table nao eh criado dinamicamente, por isso deve ser feito este procedimento
	    string numero @mutable = default("0");
	    // Mudar string de acordo com a rede que estiver usando
	    *.routingTableModule = "UrbanMacro5G.ue" + numero + ".ipv4.routingTable";
	    
	    int clusterID @mutable = default(-1);
	    int pontoTipo @mutable = default(-1);
	    
	    mobility.typename = default("LinearMobility");
	    
	    mobility.initialX = uniform(0m, 1000m);
		mobility.initialY = uniform(0m, 1000m);
	    mobility.initialZ = 0m;
		mobility.initialMovementHeading = uniform(0deg, 360deg);
		mobility.initialElevation = 0deg;
		mobility.speed = 0.833mps;
	    
	    masterId = 1;
	    macNodeId = 1;
	    macCellId = 1;
	    nrMasterId = 1;
	    nrMacNodeId = 1;
	    nrMacCellId = 1;
	    
	    numApps = 1;
	    
	    app[0].typename = "VoIPReceiver";
	    app[0].localPort = 3000;
}