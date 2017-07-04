#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>  // pegar interfaces ativas e IPs associados
#include <map>
#include <fstream>

void getMessageType(char * msgType, char * buf){

    memset(msgType,0,sizeof(msgType));

    if(buf[1] == 1){
        strcpy(msgType,"CLIREQ");
    }else if(buf[1] == 2){
        strcpy(msgType,"QUERY");
    }else{
        strcpy(msgType,"RESPONSE");
    }
}

void getKey(char * key, char * buf){

    strcpy(key,&buf[2]);

}

void perr (const char *str) {
  fprintf(stderr, "***********************************************************\n");
  fprintf(stderr, "ERROR: %s\n", str);
  fprintf(stderr, "***********************************************************\n");
  exit(EXIT_FAILURE);
}

void pack(uint16_t code, char *data, char *byte_array, int byte_array_len) {
  uint16_t code_net = htons(code);
  memset(byte_array, 0, sizeof(*byte_array) * byte_array_len);
  memcpy(&(byte_array[0]), &code_net, sizeof(code_net)); // CODE, QUERY, RESPONSE,
  memcpy(&(byte_array[2]), data, strlen(data)); //
}

uint16_t unpack(char *byte_array, char *data, int data_len) {
  uint16_t code_net;
  memset(data, '\0', sizeof(*data) * data_len);
  memcpy(&code_net, &(byte_array[0]), sizeof(code_net)); // CODE, QUERY, RESPONSE,
  memcpy(data, &(byte_array[2]), sizeof(*data) * data_len); //
  return ntohs(code_net);
}
