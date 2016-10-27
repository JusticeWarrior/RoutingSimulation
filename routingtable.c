#include "ne.h"
#include "router.h"

struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;

void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID) { }

int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID) { return 0; }

void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID) { }

void PrintRoutes (FILE* Logfile, int myID) { }

void UninstallRoutesOnNbrDeath(int DeadNbr) { }
