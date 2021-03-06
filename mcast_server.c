#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int mcast_join(int sockfd, const struct sockaddr *sa, socklen_t salen, const char *ifname, u_int ifindex, int loopback){


  switch(sa->sa_family){
    case AF_INET: {
      struct ip_mreq mreq;
      struct ifreq   ifreq;

      memcpy(&mreq.imr_multiaddr, &((struct sockaddr_in *) sa) ->sin_addr, sizeof(struct in_addr));

      if(ifindex > 0){
        if(if_indextoname(ifindex, ifreq.ifr_name) == NULL){
          errno = ENXIO;
          return(-1);
        }
        goto doioctl;
      }
      else if(ifname != NULL){
        strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
doioctl:
        if(ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0){
          return(-1);
        }

        memcpy(&mreq.imr_interface, &((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr, sizeof(struct in_addr));
      }
      else{
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
      }

      u_int yes = 1;
      if(loopback){
        return (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &yes, sizeof(yes)));
      }
      else{
          return (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &yes, sizeof(yes)));
      }
    }
  }



}



int main(int argc, char **argv){

  int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in groupSock;

  if(argc < 4){
    printf("Usage: ./server  <multicast name> <port> <loopback (0 or 1)>\n");
    exit(1);
  }

  char *name = argv[1];
  int port = atoi(argv[2]);
  int loopback = atoi(argv[3]);

  if(sockfd < 0){
    perror("Opening datagram socket error");
    exit(1);
  }
  else{
    printf("OPENING SOCKET\n");
  }

  groupSock.sin_family = AF_INET;
  groupSock.sin_addr.s_addr = inet_addr("226.1.1.1");
  groupSock.sin_port = htons(port);

  if(mcast_join(sockfd,(struct sockaddr *)&groupSock, sizeof(groupSock),name,1,loopback) < 0){
    perror("Setting local interface error");
    exit(1);
  }
  else{
    printf("Setting local interface....OK\n");
  }

  int datalen = 5000;
  char databuf[datalen];


  /*runs forever used to send messages*/
  for(;;){


    printf("Type in a message to send\n");
    if(fgets(databuf,sizeof(databuf),stdin)) {
      if(sendto(sockfd, databuf, datalen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0){
        perror("Sending message error");
      }
      else{
        printf("Sending message = %s\n",databuf);
      }
    }



  }


  return 0;







}
