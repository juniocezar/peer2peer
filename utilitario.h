#include "utilitario.c"

void getMessageType(char * msgType, char * buf);
void getKey(char * key, char * buf);
void perr (const char *str);
void pack(uint16_t code, char *data, char *byte_array, int byte_array_len);
uint16_t unpack(char *byte_array, char *data, int data_len);
