#include "ne.h"
#include "router.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include "stdio.h"
#include <time.h>
#include <sys/time.h>

#define SOCKET_ERROR -2
#define BIND_ERROR -3
#define DEAD 123

struct neighbor_timeout {
  unsigned int neighbor;
  unsigned int cost;
  time_t last_update;
};

struct neighbor_timeout neighbor_list[MAX_ROUTERS];
unsigned int num_neighbors;
time_t last_time_update_sent;
time_t last_time_table_updated;
time_t init_time;
int converged = 0;
int ID;
int udp_socket;

FILE* file;


int udp_listen(int port);
struct sockaddr_in get_network(const char* server, int porti, int* found);
void initialize(int fd, struct sockaddr_in network_address, int id);


void handle_update();
void check_update(struct sockaddr_in network);
void check_neigbors();
void check_convergence();


int main(int argc, char** argv) {
  char* hostname;
  int ne_port;
  int router_port;
  int found; // Whether or not the host was found
  struct sockaddr_in network_address;
  fd_set rfds;
  struct timeval wait_time;
  char log_file_name[100];

  // Get argumetns from the argument string
  if (argc != 5) {
    printf("Usage: %s <router_id> <host_name> <ne_port> <router_port>\n", argv[0]);
    return -1;
  }
  ID = atoi(argv[1]);
  hostname = argv[2];
  ne_port = atoi(argv[3]);
  router_port = atoi(argv[4]);

  // Open the log file
  sprintf(log_file_name, "router%d.log",ID);
  file = fopen(log_file_name, "w");
  #ifdef DEBUG
  #if DEBUG
  file = stdout;
  #endif
  #endif

  // Initialize the network connection
  udp_socket = udp_listen(router_port);
  if (udp_socket < 0) {
    printf("Unable to bind to udp port!\n");
    return -2;
  }
  network_address = get_network(hostname, ne_port, &found);
  if (!found) {
    printf("Host not found!\n");
    return -2;
  }

  // Request neighbors from the network
  initialize(udp_socket, network_address, ID);


  // Print the routing table to stdout for now to show that it has been properly initialized
  fprintf(file, "\nRouting Table:\n");
  PrintRoutes(file, ID);
  fflush(file);

  last_time_table_updated = time(0);
  last_time_update_sent = time(0);
  init_time = time(0);
  while(1) {
    wait_time.tv_sec = UPDATE_INTERVAL;
    wait_time.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(udp_socket, &rfds);
    select(udp_socket+1, &rfds, NULL, NULL, &wait_time);

    if (FD_ISSET(udp_socket, &rfds))
      handle_update();

    check_update(network_address);
    check_neigbors();
    check_convergence();

  }

  return 0;

}

// Return the file descriptor of the socket of the given UDP port
int udp_listen(int port){
  int listenfd;
  struct sockaddr_in serveraddr = {0};
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  
  // Open a socket in the OS
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0))  < 0)
    return SOCKET_ERROR;

  // Bind to port
  if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))<0)
    return BIND_ERROR;

  return listenfd;
}

// Lookup the hostname and convert to an ip address
struct sockaddr_in get_network(const char* server, int port, int* found){
  struct sockaddr_in serveraddr = {0};
  struct hostent* hostdata;

  hostdata = gethostbyname(server);
  if (hostdata == NULL){
    *found=0;
    return serveraddr;
  }
  *found=1;
  serveraddr.sin_family = AF_INET;
  bcopy((char*)hostdata->h_addr, &serveraddr.sin_addr.s_addr,
    hostdata->h_length);
  serveraddr.sin_port = htons(port);

  return serveraddr;
}


// Send an init request and wait for the init response
void initialize(int fd, struct sockaddr_in network, int id){
  struct pkt_INIT_REQUEST request;
  struct pkt_INIT_RESPONSE response;

  request.router_id = htonl(id);
  socklen_t addr_len=sizeof(struct sockaddr_in);
  int len;

  sendto(fd,(char*)&request,sizeof(request),0,(struct sockaddr*)&network,addr_len);
  len = recvfrom(fd, (char*)&response, sizeof(response),0, (struct sockaddr*)&network,&addr_len);
  ntoh_pkt_INIT_RESPONSE(&response);
  InitRoutingTbl(&response, id);

  num_neighbors =  response.no_nbr;
  int i;
  time_t current_time = time(0);
  for (i=0; i < num_neighbors; i++) {
    neighbor_list[i].neighbor = response.nbrcost[i].nbr;
    neighbor_list[i].cost =	response.nbrcost[i].cost;
    neighbor_list[i].last_update = current_time;
  }

}

void handle_update() {
  int i;
  struct sockaddr_in network;
  struct pkt_RT_UPDATE update_struct; 
  socklen_t addr_len=sizeof(struct sockaddr_in);
  recvfrom(udp_socket, (char*)&update_struct, sizeof(update_struct),0, (struct sockaddr*)&network,&addr_len);
  ntoh_pkt_RT_UPDATE(&update_struct);

  // Update table

  // Update neighbor timeouts
  for (i=0; i<num_neighbors; i++) {
    if(neighbor_list[i].neighbor == update_struct.sender_id) {
      neighbor_list[i].last_update = time(0);
	if (UpdateRoutes(&update_struct, neighbor_list[i].cost, ID)) {
	  converged = 0;
	  last_time_table_updated = time(0);

	  fprintf(file, "\nRouting Table:\n");
	  PrintRoutes(file, ID);
	  fflush(file);

	}
      break;
    }
  }
  
  
  
}

void check_update(struct sockaddr_in network) {
  struct pkt_RT_UPDATE update_struct;
  socklen_t addr_len=sizeof(struct sockaddr_in);
  int i;

  if (time(0) - last_time_update_sent >= UPDATE_INTERVAL) {
    last_time_update_sent = time(0);
    for (i=0; i<num_neighbors; i++) {
      ConvertTabletoPkt(&update_struct, ID);// Need to set sender_id and send to all neighbors
      update_struct.dest_id = neighbor_list[i].neighbor;
      update_struct.sender_id = ID;
      hton_pkt_RT_UPDATE(&update_struct);
      sendto(udp_socket, (char*)(&update_struct), sizeof(update_struct), 0, (struct sockaddr*)(&network), addr_len);
    }
  }
}
void check_neigbors() {
  int i;
  time_t current_time = time(0);
  for (i=0; i < num_neighbors; i++) {
    if (current_time - neighbor_list[i].last_update >= FAILURE_DETECTION 
      && neighbor_list[i].last_update != DEAD) {
      UninstallRoutesOnNbrDeath(neighbor_list[i].neighbor);
      neighbor_list[i].last_update = DEAD;
      // neighbor_list[i].cost = INFINITY;
      fprintf(file, "\nRouting Table:\n");
      PrintRoutes(file, ID);
      fflush(file);
      last_time_table_updated = time(0);
      converged=0;
    }
  }
}
void check_convergence() {
  if (time(0) - last_time_table_updated >= CONVERGE_TIMEOUT && !converged) {
    converged = 1;
    fprintf(file, "%ld:Converged\n", (long)(time(0) - init_time));
    fflush(file);
  }
}
