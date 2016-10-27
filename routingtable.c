#include "ne.h"
#include "router.h"

struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;

void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID) {
	ntoh_pkt_INIT_RESPONSE(InitResponse);

	routingTable[i].dest_id = myID;
	routingTable[i].next_hop = myID;
	routingTable[i].cost = 0;

	int i;
	for (i = 1; i <= InitResponse->no_nbr; i++) {
		routingTable[i].dest_id = InitResponse->nbrcost[i - 1].nbr;
		routingTable[i].next_hop = InitResponse->nbrcost[i - 1].nbr;
		routingtable[i].cost = InitResponse->nbrcost[i - 1].cost;
	}
}

int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID) { return 0; }

void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID) { }

void PrintRoutes (FILE* Logfile, int myID) { }

void UninstallRoutesOnNbrDeath(int DeadNbr) { }
