/* internal functions for shmwrap.c */
#ifndef _SHMWRAP_STATE_H_
#define _SHMWRAP_STATE_H_

#include <errno.h>
#include <stdio.h>

#define SHM_FATAL_PERROR(file,line) {			\
    perror((file));					\
    printf("(errno %d at line %d)\n",errno,(line));	\
    exit(1);						\
  }

typedef enum {
  SHMWRAP_AWAITING_CONNECTION=66,
  SHMWRAP_CONNECTED
} shmwrap_state_mode_t;

typedef struct {
  shmwrap_state_mode_t mode;
  int server_sockfd;
  int client_sockfd;
} shmwrap_state_t;

typedef enum {
  SHMWRAP_CMD_NOCOMMAND,
  SHMWRAP_CMD_HELLO,
  SHMWRAP_CMD_REQUEST_INFO
} shmwrap_command_type_t;

typedef struct {
  shmwrap_command_type_t type;
  void* payload;
} shmwrap_command_t;

shmwrap_state_t *create_state(void);
void destoy_state(shmwrap_state_t *state);

void handle_network(shmwrap_state_t *state,shmwrap_command_t* incoming_command);
void send_buf(shmwrap_state_t *state, const char* buf,int buflen);

#endif
