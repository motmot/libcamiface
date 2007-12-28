#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#include "_shmwrap_state.h"
#include "cam_iface_shmwrap.h"

shmwrap_state_t *create_state(void) {
  shmwrap_state_t *state;
  struct sockaddr_in serv_addr;

  state = (shmwrap_state_t *)malloc(sizeof(shmwrap_state_t));
  if (state==NULL) return state;

  bzero((char *)state, sizeof(shmwrap_state_t));
  state->mode = SHMWRAP_AWAITING_CONNECTION;

  // ---------------------------------

  state->server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (state->server_sockfd < 0)
    SHM_FATAL_PERROR(__FILE__,__LINE__);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(shmwrap_control_port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(state->server_sockfd, (struct sockaddr *) &serv_addr,
	   sizeof(serv_addr)) < 0)
    SHM_FATAL_PERROR(__FILE__,__LINE__);

  listen(state->server_sockfd,5);

  // make socket non-blocking
  if (fcntl(state->server_sockfd, F_SETFL, O_NONBLOCK) < 0) SHM_FATAL_PERROR(__FILE__,__LINE__);

  return state;
}

void destoy_state(shmwrap_state_t *state) {
  if (state!=NULL) {
    free(state);
  }
}
void cleanup_handle_network(shmwrap_state_t *state,shmwrap_command_t* incoming_command) {
  if (incoming_command->payload != NULL) {
    free(incoming_command->payload);
  }
}

void handle_network(shmwrap_state_t *state,shmwrap_command_t* incoming_command) {
  int newsockfd, clilen, n;
  struct sockaddr_in cli_addr;
  char buffer[256];
  int incoming_sockfd;
  int err;

  int device, prop, is_auto, trig_value;
  long value;

    incoming_command->type = SHMWRAP_CMD_NOCOMMAND;
    incoming_command->payload = NULL;

    // check for connection attempts on state->server_sockfd
    clilen = sizeof(cli_addr);
    incoming_sockfd = accept(state->server_sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (incoming_sockfd == -1) {
      // case 1: no new connection attempts
      switch (errno) {
      case EAGAIN: // same as EWOULDBLOCK
	break;
      default:
	SHM_FATAL_PERROR(__FILE__,__LINE__);
      }
    } else {
      // case 2: conncetion attempt made on state->server_sockfd
      if (state->mode == SHMWRAP_AWAITING_CONNECTION) {
	state->client_sockfd = incoming_sockfd;

	// make socket non-blocking
	if (fcntl(state->client_sockfd, F_SETFL, O_NONBLOCK) < 0)
	  SHM_FATAL_PERROR(__FILE__,__LINE__);

	/*

	// make socket send data immediately (like flush)
	// see http://www.ibm.com/developerworks/linux/library/l-hisock.html?ca=dgr-lnxw01BoostSocket
	int flag=1;
	err = setsockopt( state->client_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );

	*/

	state->mode = SHMWRAP_CONNECTED;
	printf("connect\n");
      } else {
	printf("ERROR: connection attempt, but I'm already connected!\n");
      }
    }

    if (state->client_sockfd) {
      // control stream exists, check for data

      // check for new data on control stream

      bzero(buffer,256);
      //n = read(state->client_sockfd,buffer,255);
      int flags=0;
      n = recv(state->client_sockfd,buffer,255,flags);
      if (n<0) {
	// case 1: no new connection attempts
	switch (errno) {
	case EAGAIN:
	  break;
	case EBADF: SHM_FATAL_PERROR(__FILE__,__LINE__);
	case EFAULT: SHM_FATAL_PERROR(__FILE__,__LINE__);
	case EINTR: SHM_FATAL_PERROR(__FILE__,__LINE__);
	case EINVAL: SHM_FATAL_PERROR(__FILE__,__LINE__);
	case EIO: SHM_FATAL_PERROR(__FILE__,__LINE__);
	case EISDIR: SHM_FATAL_PERROR(__FILE__,__LINE__);
	case ECONNRESET: SHM_FATAL_PERROR(__FILE__,__LINE__);
	default:
	  SHM_FATAL_PERROR(__FILE__,__LINE__);
	}
      } else if (n==0) {
	// case 2: empty buffer - assuming disconnect
	printf("disconnect\n");
	close(state->client_sockfd);
	state->client_sockfd=0;
	state->mode = SHMWRAP_AWAITING_CONNECTION;
      } else {
	// case 3: new data
	printf("Here is the message (len %d): %s\n",n,buffer);

	if (!strcmp(buffer,"hello\r\n")) {
	  incoming_command->type = SHMWRAP_CMD_HELLO;
	}
	else if (!strcmp(buffer,"info\r\n")) {
	  incoming_command->type = SHMWRAP_CMD_REQUEST_INFO;
	}
	else if (sscanf(buffer,"set_prop(%d,%d,%ld,%d)\r\n",&device,&prop,&value,&is_auto)==4) {
	  incoming_command->type = SHMWRAP_CMD_SET_PROP;
	  incoming_command->payload = malloc(sizeof(camera_property_set_info_t));
	  camera_property_set_info_t* set_prop;
	  set_prop = (camera_property_set_info_t*)incoming_command->payload;
	  set_prop->device_number = device;
	  set_prop->property_number = prop;
	  set_prop->Value = value;
	  set_prop->Auto = is_auto;
	  printf("got set_prop() command\n");
	}
	else if (sscanf(buffer,"set_trig(%d,%d)\r\n",&device,&trig_value)==2) {
	  incoming_command->type = SHMWRAP_CMD_SET_TRIG;
	  incoming_command->payload = malloc(sizeof(camera_trigger_set_trig_t));
	  camera_trigger_set_trig_t* set_trig;
	  set_trig = (camera_trigger_set_trig_t*)incoming_command->payload;
	  set_trig->device_number = device;
	  set_trig->mode = trig_value;
	}
	else {
	  printf("unhandled message\n");
	  exit(1);
	}
	printf("processed message OK\n");
      }
    }

}

void send_buf(shmwrap_state_t *state, const char* buf,int buflen) {
  if (state->mode != SHMWRAP_CONNECTED) {
    printf("error: not connected");
    return;
  }
  int flags=0;
  send(state->client_sockfd,buf,buflen,flags);
  printf("wrote %d characters\n",buflen);
}
