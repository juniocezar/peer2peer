#include "utilitario.h"


void getMessageType(int &msgId, char * buf){

    uint16_t code_net;
    memcpy(&code_net, &(buf[0]), sizeof(code_net));
    msgId = ntoh(code_net);

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

/*bool validParameters (int argc, appType ap) {

  if (ap == CLIENT and argc < 2) {
    fprintf(stderr, "Modo de executar: \
        \n./client <ip:porta>\n");
    return false;
  } else if (ap == SERVENT and argc <= 3) {    
    fprintf(stderr, "Modo de executar: \
        \n./servant <porta> <base-de-dados> <ip:porta> ... <ip_n:porta_n>\n");
    return false;
  }
  return true; 

} */ 



/*void pwar (const char *format, ...) {
  va_list arg;

  va_start (arg, format);
  fprintf(stderr, "************************************************************\
          \nWARNING: ");
  vfprintf (stderr, format, arg);
  fprintf(stderr, "************************************************************\n");
  va_end (arg);
}
*/