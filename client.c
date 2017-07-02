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
     
  if (sendto(sockfd, "teste", 5, 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1) {
    perr("sendto()"); // ToDo: perr -> pwar
  }

  char key[40];
  printf("Insira uma chave para consulta: ");
  scanf("%s\n", key);

  char buf[1000];
  unsigned int slen = sizeof(si_other);
  int recv_len = recvfrom(sockfd, buf, 1000, 0, (struct sockaddr *) &si_other, &slen);

         
    // imprimir detalhes da host que nos enviou dados
    printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
    printf("Data: %s\n" , buf);
    

  return 0;
}
