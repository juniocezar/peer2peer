/*******************************************************************************
* Redes de Computadores - Um Sistema Peer-to-peer de Armazenamento Chave-valor *
* Junio Cezar Ribeiro da Silva - 2012075597                                    *
* Joao Paulo Martins Castanheira                                               *
* Servent                                                                      *
* Compilar: make                                                               *
* Executar: ./servent <PORTA> <ARQUIVO-BASE-DADOS> <IP1:PORTA1>...<IPN:PORTAN> *
*******************************************************************************/
// Escrito em C com Classes haha

#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>  // pegar interfaces ativas e IPs associados
#include <map>
#include <set>
#include <utility>
#include <fstream>

#include "utilitario.h"

#define IN_BYTES_SRV 54 // recebemos ou mandanmos no maximo 54 bytes a outro servant
#define IN_BYTES_CLI 42 // recebemos no maximo 42 bytes de um cliente
#define OUT_BYTES_RES 204 // enviamos 204 bytes a um cliente

using std::string;

// Verificar se parametros passados estao corretos e extrair dados deles
bool validParameters (int argc, char** argv, struct sockaddr_in *neighbours, char* filename);

// 
void pack_query(uint16_t code, uint16_t ttl, uint32_t ip, uint16_t port, uint32_t seq, char* key, 
  char *byte_array, int byte_array_len);


int main (int argc, char** argv) {
  
  char* data_base_name;
  struct sockaddr_in *neighbours;
  std::map <string, string> dictionary;
  std::set<string> requestsReceived;

  // checando se os parametros passados sao validos  
  if(not validParameters(argc, argv, neighbours, data_base_name))
    return 1;

  int port = atoi(argv[1]);
  int numNiggas = argc - 3;
  int seqNum = 0;


  // Abrindo base de dados para criacao do dicionario
  std::ifstream data_base(data_base_name);

  if (data_base.fail()) {
    fprintf(stderr, "Erro ao abrir o arquivo: %s\nAbortando execucao ...\n", data_base_name);
    return 1;
  }

  // Iterando linha por linha para criacao do dicionario
  string line;
  while (std::getline(data_base, line)) {
    if(line.empty())
      continue;

    // removendo espacos em branco do comeco e fim da linha
    size_t line_begin = line.find_first_not_of(' ');
    size_t line_end   = line.find_last_not_of(' ');

    // linha que so tem espaco em branco, descartando ela
    if (line_begin == std::string::npos && line_end == std::string::npos)
      continue;
    // linha que tem espaco em branco so no comeco, redefinindo final da linha
    else if (line_begin != std::string::npos && line_end == std::string::npos)
      line_end = line.size();
    // linha que tem espaco em branco so no final, redefinindo inicio da linha
    else if (line_begin == std::string::npos && line_end != std::string::npos)
      line_begin = 0;

    string stripped_line = line.substr(line_begin, line_end + 1);

    // ignorando comentarios
    if (stripped_line.c_str()[0] == '#')
      continue;

    const size_t key_end_space = stripped_line.find(" ");
    const size_t key_end_tab = stripped_line.find("\t");
    const size_t key_end = key_end_tab < key_end_space ? key_end_tab : key_end_space;

    string key   = stripped_line.substr(0, key_end);
    string value = stripped_line.substr(key_end, stripped_line.size());

    dictionary[key] = value;
    //std::cout << "Chave encontrada:" << dictionary.find(key)->first << '\n';
    //std::cout << "Valor encontrado:" << dictionary.find(key)->second << '\n';

    //Nota: Podemos retirar os espacos em branco ou tabs do inicio dos valores
    // nao eh necessario segundo a documentacao, mas seria interessante
  }

  // Inicializar socket
  struct sockaddr_in si_me, si_other;
  int sockfd;

  //criar socket UDP
  if ((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    perr("socket");
  }

  // zerar estrutura
  memset((char *) &si_me, 0, sizeof(si_me));

  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(port);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  //bind socket para porta
  if (bind(sockfd , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
    perr("bind");
  }

  printf("===========================================================\n");
  printf("\tServant iniciado e escutando na porta %u\n", ntohs(si_me.sin_port));
  printf("\nAs seguintes interfaces e enderecos IPs podem ser\
          \nutilizados para se comunicar com o servant:\n\n");

  // listando interfaces ativas no computador e mostrando IPs associados
  struct ifaddrs *ifap, *ifa;
  struct sockaddr_in *sa;
  char *addr;

  getifaddrs (&ifap);
  int i = 1;
  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr->sa_family==AF_INET) {
      sa = (struct sockaddr_in *) ifa->ifa_addr;
      addr = inet_ntoa(sa->sin_addr);
      printf("%d - Interface: %s\tEndereco IP: %s\n", i, ifa->ifa_name, addr);
      i++;
    }
  }
  freeifaddrs(ifap);

  printf("\nDicionario local conta com %lu entradas.\n", dictionary.size());
  printf("Vizinhos: \n");

  for (int i = 0; i < numNiggas; i++) {
    printf("(%s,%u) ", inet_ntoa(neighbours[i].sin_addr), ntohs(neighbours[i].sin_port));
    if (i % 3 == 0)
      printf("\n");
  }
  printf("\n");


  char buf[IN_BYTES_SRV], keyReceived[40];
  int recv_len;
  unsigned int slen = sizeof(si_other);
  int msgId;

  while (true) {
    printf("Aguardando solicitacao...\n");
    printf("########################################################################\n");
    fflush(stdout);

    /* Bloquear enquanto espera alguma entrada de dados
     * Note que podemos receber mensagens de clientes com 42 bytes ou de outros
     * servents com 65 bytes, por isso o buffer sempre espera pelo maior deles*/
    if ((recv_len = recvfrom(sockfd, buf, IN_BYTES_SRV, 0, (struct sockaddr *) &si_other, &slen)) == -1) {
      perr("recvfrom()");
    }

    getMessageType(&msgId, buf);    

    if (msgId == CLIREQ) { // mensagem vem de cliente      
      getKey(keyReceived, buf);

      // imprimir detalhes da host que nos enviou dados
      printf("Recebi CLIREQ do cliente %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));      
      printf("Chave consultada: < %s >\n", keyReceived);

      /* Gerando pacote QUERY para enviar aos meus vizinhos, se tiver algum     
       * e adicionando requisicao ao set de queries ja recebidos (nao estou certo se precisa)*/
      if (numNiggas > 0) {
        uint16_t TTL = 3;
        uint8_t byte_array[IN_BYTES_SRV];
        pack_query(QUERY, TTL, si_other.sin_addr, si_other.sin_port, seqNum, keyReceived, byte_array, IN_BYTES_SRV);
        string requestIdentifier = string(inet_ntoa(si_other.sin_addr) + ntohs(si_other.sin_port) + seqNum + keyReceived);
        requestsReceived.insert(requestIdentifier);
        seqNum++;

        // Enviando query a cada um dos meus vizinhos
        for (int i = 0; i < numNiggas; i++) {
          if (sendto(sockfd, byte_array, IN_BYTES_SRV, 0, (struct sockaddr*) &(neighbours[i]), slen) == -1) {
            perr("sendto()");
          }
        }

        // Procurando chave no dicionario local                
        std::map<string, string>::iterator it;
        it = dictionary.find(keyReceived);
        if(it != dictionary.end()){ 
          std::pair<string, string> found = it;
          char byte_array[OUT_BYTES_RES];
          pack_response(RESPONSE, found->first, found->second, byte_array, OUT_BYTES_RES);
          if (sendto(sockfd, byte_array, OUT_BYTES_RES, 0, (struct sockaddr*) &si_other, slen) == -1) {
            perr("sendto()");
          }          
        } else {
          printf("Chave solicitada nao encontrada na base de dados.\n");
        }
      }

    } else if (msgId == QUERY) { // mensagem vem de servent
      uint16_t ttl, portClient;
      uint32_t ipClient, inSeqNum;
      unpack_query(buf, &ttl, &ipClient, &portClient, &inSeqNum, keyReceived);      

      // Verificar se ja recebi essa mesma requisicao      
      string requestIdentifier = string(inet_ntoa(ipClient) + ntohs(portClient) + inSeqNum + keyReceived);
      if (requestIdentifier.count(requestIdentifier)) { // caso em que ja recebi a requisicao anteriormente
        printf("Recebi solicitacao repetida, ignorando operacao.\n");
        continue;
      } else {
        ttl--;
        if (ttl > 0) { // retrasmite aos meus vizinhos
          memcpy(&(buf[2]), &ttl, sizeof(ttl)); // atualizando ttl na mensagem
          for (int i = 0; i < numNiggas; i++) {
            if (sendto(sockfd, buf, IN_BYTES_SRV, 0, (struct sockaddr*) &(neighbours[i]), slen) == -1) {
              perr("sendto()");
            }
          }
        } else {
          printf("TTL final da mensagem igual a zero, nao retransmitirei ...\n");
        }
      }

      // Implementar a busca no dicionario e enviar ao cliente da mensagem

    } else {
      perr("Mensagem com tipo nao identificado.");
    }
    

    
    
    printf("##############################################################################\n\n");

  }

  close(sockfd);
  free(neighbours);
  return 0;
}





bool validParameters (int argc, char** argv, struct sockaddr_in *neighbours, char* filename) {

  if (argc <= 3) {    
    fprintf(stderr, "Modo de executar: \
        \n./servant <porta> <base-de-dados> <ip:porta> ... <ip_n:porta_n>\n");
    return false;
  }

  filename = argv[2];
  neighbours = (struct sockaddr_in*) malloc((argc - 3) * sizeof(struct sockaddr_in));

  // iterando sobre argumentos para extrair vizinhos a se conectar
  // i = 3, pois iremos ignorar os 3 primeiros argumentos da cli
  int i = 3;
  while (i < argc) {
    char* ip   = strtok(argv[i], ":");
    char* port = strtok(NULL, ":");
    if (ip == NULL || port == NULL || !atoi(port)) {
      fprintf(stderr, "IP ou porta de algum vizinho inseridos incorretamente!\n");
      return 1;
    }

    neighbours[i-3].sin_family = AF_INET;
    neighbours[i-3].sin_port = htons(atoi(port));
    neighbours[i-3].sin_addr.s_addr = inet_addr(ip);
    
    i++;
  }

  return true;

}

void pack_response(uint16_t code, string key, string value, char *byte_array, int byte_array_len) {

  string keyValue = string(key + '\t' + value + '\0');  
  char* keyValueChar = keyValue.c_str();

  code = htons(code);
  memset(byte_array, 0, sizeof(*byte_array) * byte_array_len);

  memcpy(&(byte_array[0]), &code, sizeof(code)); // RESPONSE
  memcpy(&(byte_array[2]), keyValueChar, strlen(key+1)); // +1 para incluir \0

}


void unpack_query(char *byte_array, uint16_t* ttl, uint32_t* ipClient, uint16_t* portClient, 
  uint32_t* inSeqNum, char* key) {

  memcpy(ttl, &(byte_array[2]), sizeof(*ttl)); // TTL
  memcpy(ipClient, &(byte_array[4]), sizeof(*ipClient)); // IP
  memcpy(portClient, &(byte_array[8]), sizeof(*portClient)); // PORTA
  memcpy(inSeqNum, &(byte_array[10]), sizeof(*inSeqNum)); // SEQ
  memcpy(key, &(byte_array[14]), strlen(key)); //

}


void pack_query(uint16_t code, uint16_t ttl, uint32_t ip, uint16_t port, uint32_t seq, char* key, 
  char *byte_array, int byte_array_len) {

  code = htons(code);
  ttl = htons(ttl);
  seq = htonl(seq);
  memset(byte_array, 0, sizeof(*byte_array) * byte_array_len);

  memcpy(&(byte_array[0]), &code, sizeof(code)); // QUERY
  memcpy(&(byte_array[2]), &ttl, sizeof(ttl)); // TTL
  memcpy(&(byte_array[4]), &ip, sizeof(ip)); // IP
  memcpy(&(byte_array[8]), &port, sizeof(port)); // PORTA
  memcpy(&(byte_array[10]), &seq, sizeof(seq)); // SEQ
  memcpy(&(byte_array[14]), key, strlen(key)); //

}