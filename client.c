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
#include <sys/time.h>

#include "utilitario.h"

#define KEY_LEN 40

using std::string;


// key --> up to KEY_LEN chars
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

  // Setando as configurações de timeout do recvfrom
  struct timeval tv;
  tv.tv_sec = 4; // Timeout de 4 segundos para o recvfrom
  tv.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perr("setsockopt()");
  }

  // zerar estrutura
  memset((char *) &si_other, 0, sizeof(si_other));

  // cliente dara bind na mesma porta do par
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(atoi(port));
  si_other.sin_addr.s_addr = inet_addr(ip);

  char key[KEY_LEN];
  printf("Insira uma chave para consulta, max %d caracteres: ", KEY_LEN);
  scanf("%s", key);

  char byte_array[KEY_LEN + 2], data[170], msg[160];
  pack(1, key, byte_array, KEY_LEN + 2);
  unsigned int slen = sizeof(si_other);
  int recv_len;

  // COMUNICAÇÃO COM O SERVANT
  for(int i=1; i<=2; i++){ // Tentar se comunicar duas vezes com o servant. Somente duas vezes.
    if (sendto(sockfd, byte_array, sizeof(byte_array), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1) {
      perr("sendto()"); // ToDo: perr -> pwar
    }
    recv_len = recvfrom(sockfd, data, 170, 0, (struct sockaddr *) &si_other, &slen);
    if(recv_len != -1){
      break;
    }
    if(i==1){
        printf("Não foi obtida uma resposta, tantando enviar mensagem novamente...\n");
    }
  }
  // COMUNICAÇÃO COM O SERVANT

  if(recv_len != -1){
    // imprimir detalhes da host que nos enviou dados
    uint16_t code = unpack(data, msg, 160);
    printf("Received packet ID = %u from %s:%d\n", code, inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
    printf("Data: %s\n", msg);
  }else{
    printf("Não foi possível conectar-se à rede, programa ser encerrado...\n");
  }



  return 0;
}
