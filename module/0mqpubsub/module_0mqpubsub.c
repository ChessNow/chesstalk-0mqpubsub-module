#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <errno.h>

#include "chesstalk_module.h"

#include <zmq.h>

#include "zhelpers.h"

#include "zmsg.c"

signed long zeitgeist_identifier;

#define ZMQ_CONNECTED 0x1

struct workpack {

  void *context;
  void *pub_socket;

  unsigned long state;

};

struct workpack w;

char *server_name;

unsigned short port;

int move_submission(int move_number, char *move_string, int white_move) {

  int retval;

  char send_string[40];

  int len;

  assert(move_string!=NULL);

  if (! (w.state & ZMQ_CONNECTED)) {

    char transport_string[80];
    int rc;

    sprintf(transport_string, "tcp://%s:%d", server_name, port);

    rc = zmq_connect(w.pub_socket, transport_string);

    if (rc!=0) {

      printf("%s: Trouble with zmq_connect.\n", __FUNCTION__);
      return -1;

    }

    printf("%s: Connected to ZMQ transport %s for publishing.\n", __FUNCTION__, transport_string);

    w.state |= ZMQ_CONNECTED;

  }

  sprintf(send_string, "%d%s %s", move_number, white_move?".":"...", move_string);

  retval = s_send(w.pub_socket, send_string);
  if (retval!=0) {
    return -1;
  }

  printf("%s: [ZMQ] Published move %s\n", __FUNCTION__, send_string);

  return 0;

}

int module_shutdown() {

  int close_retval, term_retval;
  
  zeitgeist_identifier = -1;

  close_retval = zmq_close(w.pub_socket);
  if (close_retval!=0) {
    printf("%s: Trouble with call to zmq_close.\n", __FUNCTION__);
  }

  term_retval = zmq_term(w.context);
  if (term_retval!=0) {
    printf("%s: Trouble with call to zmq_term.\n", __FUNCTION__);
  }

  return (!close_retval && !term_retval) ? 0 : -1;

}

int module_entry(struct chesstalk_module *m) {

  char *env_PUB_ADDRESS = getenv("PUB_ADDRESS");

  char *env_PUB_PORT = getenv("PUB_PORT");

  int retval;

  if (env_PUB_ADDRESS == NULL || env_PUB_PORT==NULL) {
    printf("%s: Need both a PUB_ADDRESS and PUB_PORT specified.\n", __FUNCTION__);
    return -1;
  }

  server_name = env_PUB_ADDRESS;

  {
    long int conversion = strtol(env_PUB_PORT, NULL, 10);
    if (conversion<0 || conversion>65535) {
      printf("%s: ZEITGEIST_PORT out of range.\n", __FUNCTION__);
      return -1;
    }
    port = conversion;
  }

  if (m==NULL) {
    printf("%s: Need an allocated chesstalk_module structure.\n", __FUNCTION__);
    return -1;
  }

  w.context = zmq_init(1);

  if (w.context==NULL) {
    printf("%s: Trouble with call to zmq_init.\n", __FUNCTION__);
    return -1;
  }

  w.pub_socket = zmq_socket(w.context, ZMQ_PUB);

  if (w.pub_socket==NULL) {
    printf("%s: Trouble with call to zmq_socket.\n", __FUNCTION__);
    zmq_term(w.context);
    return -1;
  }

  w.state = 0;

  m->move_submission = move_submission;
  m->module_shutdown = module_shutdown;

  return 0;

}
