#include "ne.h"
#include "router.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include "stdio.h"

#define SOCKET_ERROR -2
#define BIND_ERROR -3


int udp_listen(int port);
struct sockaddr_in get_network(const char* server, int porti, int* found);
void initialize(int fd, struct sockaddr_in network_address, int id);

int main(int argc, char** argv) {
  int ID;
  char* hostname;
  int ne_port;
  int router_port;
  int udp_socket;
  int found; // Whether or not the host was found
  struct sockaddr_in network_address;

  // Get argumetns from the argument string
  if (argc != 5) {
    printf("Usage: %s <router_id> <host_name> <ne_port> <router_port>\n", argv[0]);
    return -1;
  }
  ID = atoi(argv[1]);
  hostname = argv[2];
  ne_port = atoi(argv[3]);
  router_port = atoi(argv[4]);

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
  PrintRoutes(stdout, ID);

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
}
