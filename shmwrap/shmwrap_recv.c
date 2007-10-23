#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "shmwrap.h"

double floattime() {
  struct timeval t;
  if (gettimeofday(&t, (struct timezone *)NULL) == 0)
    return (double)t.tv_sec + t.tv_usec*0.000001;
  else
    return 0.0;
}

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main() {
    shmwrap_msg_ready_t curmsg;
    double now,tdiff;
    int sock, length, fromlen, n;
    struct sockaddr_in server;
    struct sockaddr_in from;
    key_t key;
    int shmid;
    char *data; // shared memory pointer
    //    size_t offset, stride;
    unsigned char *pixels;
    uint64_t last_framenumber;

    last_framenumber=0;

    // Get key
    if ((key = ftok(shmwrap_ftok_path, shmwrap_shm_name)) == -1) {
      perror("ftok");
      exit(1);
    }

    shmid = shmget(key, shm_size, 0644);
    if (shmid==-1) {
      perror("shmget"); exit(1);
    }

    data = shmat(shmid, (void *)0, SHM_RDONLY);
    if (data == (char *)(-1)) {
      perror("shmat");
      exit(1);
    }

    sock=socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) error("Opening socket");

    length = sizeof(server);
    bzero(&server,length);
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_port=htons(shmwrap_frame_ready_port);
    if (bind(sock,(struct sockaddr *)&server,length)<0) 
      error("binding");

    fromlen = sizeof(struct sockaddr_in);

    /*
    if ((key = ftok(shmwrap_ftok_path, shmwrap_msg_queue_name)) == -1) {
        perror("ftok");
        exit(1);
    }

    if ((msqid = msgget(key, 0644)) == -1) { // connect to the queue
        perror("msgget");
        exit(1);
    }
    */

    while (1) {
      n = recvfrom(sock,&curmsg,sizeof(curmsg),0,(struct sockaddr *)&from,&fromlen);
      if (n < 0) error("recvfrom");
      if (n!=sizeof(curmsg)) {
	printf("n!=sizeof(curmsg)\n");
	exit(2);
      }

      now = floattime();
      tdiff = now-curmsg.timestamp;
      pixels = data + curmsg.start_offset;
      printf("%d: latency %f msec\n",curmsg.framenumber,tdiff*1e3);
      if ((curmsg.framenumber-last_framenumber)!=1) {
	printf("                                MISSING!");
      }
      last_framenumber=curmsg.framenumber;
      printf(" %02x %02x %02x %02x %02x\n",pixels[0],pixels[1],pixels[2],pixels[3],pixels[4]);
    }
}
