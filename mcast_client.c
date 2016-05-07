#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>


struct sockaddr_in localSock;
struct ip_mreq group;
int sd;
int datalen;
char databuf[5000];



int main(int argc, char **argv){

  if(argc < 2){
    printf("Usage: ./client <port>\n");
    exit(1);
  }

  int port = atoi(argv[1]);

  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sd < 0){
    perror("Opening datagram socket error");
    exit(1);
  }
  else{
    printf("Opening datagram socket....OK\n");
  }

  int reuse = 1;
  if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0){
    perror("Setting SO_REUSEADDR error");
    close(sd);
    exit(1);
  }
  else{
    printf("Setting SO_REUSEADDR...OK.\n");
  }

  memset((char *) &localSock, 0, sizeof(localSock));
  localSock.sin_family = AF_INET;
  localSock.sin_port = htons(port);
  localSock.sin_addr.s_addr = INADDR_ANY;
  if(bind(sd, (struct sockaddr*) &localSock, sizeof(localSock))){
    perror("Binding datagram socket error");
    close(sd);
    exit(1);
  }
  else{
    printf("Binding datagram socket...OK.\n");
  }

  group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
  group.imr_interface.s_addr = INADDR_ANY;
  if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0){
    perror("Adding multicast group error");
    close(sd);
    exit(1);
  }
  else{
    printf("Adding multicast group...OK.\n");
  }

  datalen = 5000;


  /*runs forever, used to receive messages */
  for(;;){
    int b;
    if((b = recvfrom(sd,databuf,datalen,0,NULL,0)) < 0){
      printf("error\n");
      perror("Reading datagram message error");
      close(sd);
      exit(1);
    }
    else{
      printf("Received Message: %s\n",databuf);
    }
  }


  return 0;

}
