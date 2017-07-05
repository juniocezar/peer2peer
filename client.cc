/******************************************************************************
* Redes de Computadores - Um Sistema Peer-to-peer de Armazenamento Chave-valor*
* Junio Cezar Ribeiro da Silva                                                *
* Joao Paulo Martins Castanheira                                              *
*                                                                             *
* Programa cliente                                                            *
* Compilar: make                                                              *
* Executar: ./client <IP:PORTA>                                               *
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
#define RESPONSE_LEN 204
#define VALUE_LEN 160

using std::string;

// Verificar se parametros passados estao corretos e extrair dados deles
bool validParameters (int argc, char** argv, char** ip, char** port);


void pack(uint16_t code, char *data, char *byte_array, int byte_array_len);

uint16_t unpack(char *byte_array, char *data, int data_len);


int main (int argc, char** argv) {

  char *ipToConnect, *portToConnect;
  struct sockaddr_in inServent, outServent;
  int sockfd;

  // checando se os parametros passados sao validos  
  if (not validParameters(argc, argv, &ipToConnect, &portToConnect))
    return 1;
  
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

  /* zerar estruturas dos servents
   * inServent é utilizado para todas recepcoes de pacotes UDP
   * outServent é utilizado para enviar pacotes UDP ao servent ao qual sou
   * associado */
  memset((char *) &inServent, 0, sizeof(inServent));
  memset((char *) &outServent, 0, sizeof(outServent));

  // inicializando servent associado com dados da CLI
  outServent.sin_family = AF_INET;
  outServent.sin_port = htons(atoi(portToConnect));
  outServent.sin_addr.s_addr = inet_addr(ipToConnect);

  int recv_len; 
  char key[KEY_LEN];
  char byte_array[KEY_LEN + 2], response[RESPONSE_LEN], *msg;
  msg = (char *) malloc(VALUE_LEN * sizeof(char));

  while (true) {    
    printf("\nInsira uma chave para consulta, max %d caracteres: ", KEY_LEN);
    scanf("%s", key);
    
    pack(CLIREQ, key, byte_array, KEY_LEN + 2);
    unsigned int slen = sizeof(outServent);
    
    // Enviando pacote ao servent associado    
    for (int i=1; i<=2; i++) { // Tentar se comunicar duas vezes com o servant. Somente duas vezes.      
      printf("mandando para: %s:%u\n", inet_ntoa(outServent.sin_addr), ntohs(outServent.sin_port) );
      if (sendto(sockfd, byte_array, sizeof(byte_array), 0, (struct sockaddr*) &outServent, slen) == -1) {
        perr("sendto()"); // ToDo: perr -> pwar
      }
      recv_len = recvfrom(sockfd, response, RESPONSE_LEN, 0, (struct sockaddr *) &inServent, &slen);
      if (recv_len != -1) {
        break;
      }
      if (i==1){
          printf("Não foi obtida uma resposta, tantando enviar mensagem novamente...\n");          
      } else if (i == 2) {
        printf("Numero de tentativas de reenvio extrapolado, cancelando operacao.\n\n");
      }
    }
    
    /* No caso em que recebemos uma resposta de um servent, poderemos receber mais
     * mensagens de N servents diferentes*/    
    if (recv_len != -1) {
      while (true) {
        // imprimir detalhes da host que nos enviou dados
        uint16_t code = unpack(response, msg, RESPONSE_LEN);
        printf("Resposta recebida com ID = %u, enviada por %s:%d\n", code, inet_ntoa(inServent.sin_addr), ntohs(inServent.sin_port));
        printf("Dados: %s\n", msg);

        memset((char *) &inServent, 0, sizeof(inServent));
        memset(msg, '\0', sizeof(msg));

        recv_len = recvfrom(sockfd, response, RESPONSE_LEN, 0, (struct sockaddr *) &inServent, &slen);

        /* se nao recebemos quaisquer respostas em 4 segundos, encerramos a espera e 
         * e damos controle ao usuario via cli para novas solicitacoes */
        if (recv_len < 0)
          break;
      }   
    }
  }

  free(msg);
  close(sockfd);

  return 0;

}


bool validParameters (int argc, char** argv, char** ip, char** port) {
  
  if (argc < 2) {
    fprintf(stderr, "Modo de executar: \
        \n./client <ip:porta>\n");
    return false;
  }

  *ip   = strtok(argv[1], ":");
  *port = strtok(NULL, ":");

  return true;

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
