#include "utilitario.h"
#include <algorithm>
#include <cctype>
#include <locale>

void getMessageType(int *msgId, char * buf){

    uint16_t code_net;
    memcpy(&code_net, &(buf[0]), sizeof(code_net));
    *msgId = ntohs(code_net);

}

void getKey(char * key, char * buf){

    strcpy(key, &buf[2]);

}

void perr (const char *str) {

  fprintf(stderr, "***********************************************************\n");
  fprintf(stderr, "ERROR: %s\n", str);
  fprintf(stderr, "***********************************************************\n");
  exit(EXIT_FAILURE);

}
