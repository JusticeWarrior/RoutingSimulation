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
	for (j = 0; j < RecvdUpdatePacket->no_routes; j++) {
		if (RecvdUpdatePacket->route[j].dest_id == myID)
		  break;
		new = 1;
		for (i = 1; i < NumRoutes; i++) {
			if (routingTable[i].dest_id == RecvdUpdatePacket->route[j].dest_id) {
				new = 0;
				if (RecvdUpdatePacket->route[j].next_hop != myID) {
					for (k = 1; k < NumRoutes; k++) {
						if (routingTable[k].dest_id == RecvdUpdatePacket->sender_id) {
							if ((RecvdUpdatePacket->route[j].cost + routingTable[k].cost < routingTable[i].cost) || routingTable[i].next_hop == RecvdUpdatePacket->sender_id) {
								if (routingTable[i].next_hop != RecvdUpdatePacket->sender_id || routingTable[i].cost != RecvdUpdatePacket->route[j].cost + routingTable[k].cost)
								  changed=1;
								routingTable[i].next_hop = RecvdUpdatePacket->sender_id;
								routingTable[i].cost = RecvdUpdatePacket->route[j].cost + routingTable[k].cost;
								break;
							}
						}
					}
					break;
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
