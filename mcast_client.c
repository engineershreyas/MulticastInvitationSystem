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
#include <time.h>


struct sockaddr_in localSock;
struct ip_mreq group;
int sd;
int datalen;
char databuf[5000];


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

void strip(char *str, char strip)
{
    char *p, *q;
    for (q = p = str; *p; p++)
        if (*p != strip)
            *q++ = *p;
    *q = '\0';
}

char* reply(char attending, char* email, char* curtime){
    char *msg;
    strip(email, '\n');
    strip(curtime, '\n');
    if(attending == 'y'){
      sprintf(msg, "%s\t\t%s will be attending!\n",curtime,email);
      return msg;
    }
    sprintf(msg, "%s\t\t%s will not be attending.\n",curtime,email);
    return msg;
}

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

  int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in groupSock;

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

  if(mcast_join(sockfd,(struct sockaddr *)&groupSock, sizeof(groupSock),"test",1,1) < 0){
    perror("Setting local interface error");
    exit(1);
  }
  else{
    printf("Setting local interface....OK\n");
  }

  datalen = 5000;
  char attending;
  char email[5000];
  char replytime[5000];
  time_t rawtime;
  struct tm * curtime;
  int timelen = 150;
  /*runs forever, used to receive messages */
  LOOP:for(;;){
    int b;
    if((b = recvfrom(sd,databuf,datalen,0,NULL,0)) < 0){
      printf("error\n");
      perror("Reading datagram message error");
      close(sd);
      exit(1);
    }
    else{
      printf("%s", databuf);
      /*while(attending != 'y' && attending != 'n'){
        printf("Will you attend? (y/n):\n");
        fgets(attending,1,stdin);
        if(attending != 'y' || attending != 'n') printf("Unrecognized input.\n");
      }*/
      printf("What is your email address?\n");
      fgets(email,datalen,stdin);
      time (&rawtime);
      curtime = localtime (&rawtime);
      if(sendto(sockfd, reply(attending, email, asctime(curtime)), datalen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0) perror("Sending message error");
      else printf("Sent RSVP");
      goto LOOP;
    }
  }


  return 0;

}
