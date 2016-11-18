#include "ne.h"
#include "router.h"

struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;

void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID) {
	routingTable[0].dest_id = myID;
	routingTable[0].next_hop = myID;
	routingTable[0].cost = 0;

	int i;
	for (i = 1; i <= InitResponse->no_nbr; i++) {
		routingTable[i].dest_id = InitResponse->nbrcost[i - 1].nbr;
		routingTable[i].next_hop = InitResponse->nbrcost[i - 1].nbr;
		routingTable[i].cost = InitResponse->nbrcost[i - 1].cost;
	}

	NumRoutes = InitResponse->no_nbr + 1;
}

int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID) {
	int i, j, k, new;
	int changed = 0;
	// Iterate through all routes to update in the packet
	for (j = 0; j < RecvdUpdatePacket->no_routes; j++) {
		if (RecvdUpdatePacket->route[j].dest_id == myID)
		  continue;
		new = 1;

		// Iterate through all entries in the table
		for (i = 1; i < NumRoutes; i++) {
			// If the given route corresponds to this entry
			if (routingTable[i].dest_id == RecvdUpdatePacket->route[j].dest_id) {
				new = 0;
				// Split horizon rule, no loopback
				if (RecvdUpdatePacket->route[j].next_hop != myID) {
					// Get info on the neigbor
					for (k = 1; k < NumRoutes; k++) {
						int next_hop = RecvdUpdatePacket->sender_id;
						int next_cost = next_hop == RecvdUpdatePacket->route[j].dest_id ?
						  costToNbr : RecvdUpdatePacket->route[j].cost + routingTable[k].cost;
						next_cost = next_cost > INFINITY ? INFINITY : next_cost;
						if (routingTable[k].dest_id == RecvdUpdatePacket->sender_id) {
							if ((next_cost < routingTable[i].cost) || routingTable[i].next_hop == RecvdUpdatePacket->sender_id) {
								/*
								int next_hop = RecvdUpdatePacket->sender_id;
								int next_cost = next_hop == RecvdUpdatePacket->route[j].dest_id ?
								  costToNbr : RecvdUpdatePacket->route[j].cost + routingTable[k].cost;
								next_cost = next_cost > INFINITY ? INFINITY : next_cost;
								*/
								/*
								if (routingTable[i].next_hop != RecvdUpdatePacket->sender_id || routingTable[i].cost != RecvdUpdatePacket->route[j].cost + routingTable[k].cost)
								  changed=1;
								routingTable[i].next_hop = RecvdUpdatePacket->sender_id;
								routingTable[i].cost = RecvdUpdatePacket->route[j].cost + routingTable[k].cost;
								*/
								if (routingTable[i].next_hop != next_hop || routingTable[i].cost != next_cost)
								      changed = 1;
								routingTable[i].next_hop = next_hop;
								routingTable[i].cost = next_cost;
								break;
							}
						}
					}
					break;
				}
				// If loop back happens for some reason, stop it
				else if (routingTable[i].next_hop = RecvdUpdatePacket->sender_id) {
				  routingTable[i].cost = INFINITY;
				  changed=1;
				}
			}
		}
		if (new) {
			for (k = 1; k < NumRoutes; k++) {
				if (routingTable[k].dest_id == RecvdUpdatePacket->sender_id) {
					routingTable[NumRoutes].dest_id = RecvdUpdatePacket->route[j].dest_id;
					routingTable[NumRoutes].next_hop = RecvdUpdatePacket->sender_id;
					routingTable[NumRoutes].cost = RecvdUpdatePacket->route[j].cost + routingTable[k].cost;
					NumRoutes++;
					changed = 1;
					break;
				}
			}
		}
	}
	
	return changed;
}

void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID) {
  int i;
  UpdatePacketToSend->no_routes = NumRoutes;
  for(i=0; i<NumRoutes; i++) {
    UpdatePacketToSend->route[i] = routingTable[i];
  }
  
}

void PrintRoutes (FILE* Logfile, int myID) {
  int i;
  int cost;
  for (i=0; i < NumRoutes; i++ ) {
    cost = routingTable[i].cost;
    cost = cost < INFINITY ? cost : INFINITY;
    fprintf(
      Logfile,
      "R%d -> R%d: R%d, %d\n",
      myID,
      routingTable[i].dest_id,
      routingTable[i].next_hop,
      cost
    );
  }
  fflush(Logfile);
}

void UninstallRoutesOnNbrDeath(int DeadNbr) { 
  // Loop through all of the routes installed in the routing table and invalidate all that have dead
  // DeadNbr as their next hop
  int i;
  for (i=0; i<NumRoutes; i++) {
    if (routingTable[i].next_hop == DeadNbr) {
      routingTable[i].cost = INFINITY;
    }
  }
}
