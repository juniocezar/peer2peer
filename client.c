/*******************************************************************************
* Redes de Computadores - Um Sistema Peer-to-peer de Armazenamento Chave-valor *
* Junio Cezar Ribeiro da Silva - 2012075597                                    *
* cliente                                                                      *
* Compilar: make                                                               *
* Executar: ./client <IP:PORTA>                                                   *    
*******************************************************************************/
// Escrito em C com Classes haha

#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdint.h> // ToDo: Macro para versao da std, 98 ou 11
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <map>
#include <fstream>

using std::string;

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


// key --> up to 40 chars
// values -> up to 160 chars

int main (int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Modo de executar: \
    \n./client <ip:porta>\n");
    return 1;
  }

  char* ip   = strtok(argv[1], ":");
  char* port = strtok(NULL, ":");
    
  
  // Inicializar socket
  struct sockaddr_in si_other;
  int sockfd;

  //criar socket UDP
  if ((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    perr("socket");
  }

  // zerar estrutura
  memset((char *) &si_other, 0, sizeof(si_other));
     
  // cliente dara bind na mesma porta do par
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(atoi(port));
  si_other.sin_addr.s_addr = inet_addr(ip);

  char key[40];
  printf("Insira uma chave para consulta, max 40 caracteres: ");
  scanf("%s", key);

  char byte_array[42];
  pack(1, key, byte_array, 42);

  if (sendto(sockfd, byte_array, sizeof(byte_array), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1) {
    perr("sendto()"); // ToDo: perr -> pwar
  }

  char data[170];
  char msg[160];
  unsigned int slen = sizeof(si_other);
  int recv_len = recvfrom(sockfd, data, 170, 0, (struct sockaddr *) &si_other, &slen);
         
  // imprimir detalhes da host que nos enviou dados  
  uint16_t code = unpack(data, msg, 160);
  printf("Received packet ID = %u from %s:%d\n", code, inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
  printf("Data: %s\n", msg);
  
    

  return 0;
}
