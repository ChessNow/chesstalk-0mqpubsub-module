#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <values.h>

#include <errno.h>

#include <zmq.h>

#include "zhelpers.h"

#include "zmsg.c"

#define ZMQ_CONNECTED 0x1

struct workpack {

  void *context;
  void *socket;

  unsigned long state;

};

struct workpack w;

char *server_name;

unsigned short port;

int send_move(char *move_number, char *move_string) {

  int retval;

  char response_buffer[2048];

  int len;

  char send_string[40];

  assert(move_string!=NULL);

  if (! (w.state & ZMQ_CONNECTED)) {

    char transport_string[80];
    int rc;

    sprintf(transport_string, "tcp://%s:%d", server_name, port);

    rc = zmq_connect(w.socket, transport_string);

    if (rc!=0) {

      printf("%s: Trouble with zmq_connect.\n", __FUNCTION__);
      return -1;

    }

    w.state |= ZMQ_CONNECTED;

  }

  sprintf(send_string, "%s %s", move_number, move_string);
  printf("%s: Sending '%s'\n", __FUNCTION__, send_string);
  retval = s_send(w.socket, send_string);

  if (retval != 0) {
    printf("%s: Failure with s_send.\n", __FUNCTION__);
    return -1;
  }

  return 0;

}

int shutdown_workpack() {

  int close_retval, term_retval;
  
  close_retval = zmq_close(w.socket);
  if (close_retval!=0) {
    printf("%s: Trouble with call to zmq_close.\n", __FUNCTION__);
  }

  term_retval = zmq_term(w.context);
  if (term_retval!=0) {
    printf("%s: Trouble with call to zmq_term.\n", __FUNCTION__);
  }

  return (!close_retval && !term_retval) ? 0 : -1;

}

int main(int argc, char *argv[]) {

  char *move_number = argc>1 ? argv[1] : "1.";

  char *move_string = argc>2 ? argv[2] : "e4";

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

  {

    char string_white[40], string_black[40];

    long int move = strtol(move_number, NULL, 10);
    if (move<1 || move>LONG_MAX) {
      printf("%s: Move number out of range.\n", __FUNCTION__);
      return -1;
    }

    sprintf(string_white, "%ld.", move);
    sprintf(string_black, "%ld...", move);

    if (strncmp(move_number, string_white, strlen(string_white)) && strncmp(move_number, string_black, strlen(string_black))) {
      printf("%s: Got move=%ld and expected either a white move as %s or a black response as %s; rejecting %s\n", __FUNCTION__, move, string_white, string_black, move_number);
      return -1;
    }

  }

  w.context = zmq_init(1);

  if (w.context==NULL) {
    printf("%s: Trouble with call to zmq_init.\n", __FUNCTION__);
    return -1;
  }

  w.socket = zmq_socket(w.context, ZMQ_PUB);

  if (w.socket==NULL) {
    printf("%s: Trouble with call to zmq_socket.\n", __FUNCTION__);
    zmq_term(w.context);
    return -1;
  }

  w.state = 0;

  retval = send_move(move_number, move_string);
  if (retval==-1) {
    printf("%s: Trouble with generate_identifier function.\n", __FUNCTION__);
    return -1;
  }

  printf("%s: Sleeping for a second.\n", __FUNCTION__);

  sleep(1);

  retval = shutdown_workpack();
  if (retval==-1) {
    printf("%s: Trouble with shutdown function.\n", __FUNCTION__);
    return -1;
  }

  return 0;

}
