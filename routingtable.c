#include "ne.h"
#include "router.h"

struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;

void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID) { }

int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID) { return 0; }

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
