/*******************************************************************************
* Redes de Computadores - Um Sistema Peer-to-peer de Armazenamento Chave-valor *
* Junio Cezar Ribeiro da Silva - 2012075597                                    *
* Joao Paulo Martins Castanheira                                               *
* Servent                                                                      *
* Compilar: make                                                               *
* Executar: ./servent <PORTA> <ARQUIVO-BASE-DADOS> <IP1:PORTA1>...<IPN:PORTAN> *
*******************************************************************************/
// Escrito em C com Classes haha

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
#include <sstream>
#include "utilitario.h"


#define IN_BYTES_SRV 55 // recebemos ou mandanmos no maximo 55 bytes a outro servant
#define IN_BYTES_CLI 43 // recebemos no maximo 43 bytes de um cliente
#define OUT_BYTES_RES 206 // enviamos 206 bytes a um cliente
#define KEY_SIZE 41

using std::string;

// Verificar se parametros passados estao corretos e extrair dados deles
bool validParameters (int argc, char** argv, struct sockaddr_in *neighbours, char** filename);

// Empacotar mensagem do tipo query 
void pack_query(uint16_t code, uint16_t ttl, uint32_t ip, uint16_t port, uint32_t seq, char* key,
  uint8_t *byte_array, int byte_array_len);

// Desempacotar mensagem do tipo query 
void unpack_query (char *byte_array, uint16_t* ttl, uint32_t* ipClient, uint16_t* portClient,
  uint32_t* inSeqNum, char* key);

// Empacotar mensagem do tipo respnso 
void pack_response (uint16_t code, string key, string value, char *byte_array, int byte_array_len);

string tabTrim (string const& str);
string alltrim (string const& str);
void getMessageType (int *msgId, char * buf);
void getKey (char * key, char * buf);


int main (int argc, char** argv) {

  char* data_base_name;
  struct sockaddr_in *neighbours;
  std::map <string, string> dictionary;
  std::set<string> requestsReceived;
  bool hasFile = true;

  // checando se os parametros passados sao validos
  if(argc > 3)
    neighbours = (struct sockaddr_in*) malloc((argc - 2) * sizeof(struct sockaddr_in));

  if(not validParameters(argc, argv, neighbours, &data_base_name))
    return 1;

  int port = atoi(argv[1]);
  int numNiggas = argc - 3;
  int seqNum = 0;


  // Abrindo base de dados para criacao do dicionario
  std::ifstream data_base(data_base_name);

  if (data_base.fail()) {
    pwar("Erro ao abrir o arquivo de entrada.\nConsiderando dicionario vazio");
    hasFile = false;
  }

  // Iterando linha por linha para criacao do dicionario
  string line;
  if(hasFile)
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
    string value = alltrim(stripped_line.substr(key_end, stripped_line.size()));
    dictionary[key] = value;

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
    printf("(%s:%u) ", inet_ntoa(neighbours[i].sin_addr), ntohs(neighbours[i].sin_port));

    if (i % 3 == 0)
      printf("\n");
  }
  printf("\n");


  char buf[IN_BYTES_SRV], keyReceived[KEY_SIZE];
  int recv_len;
  unsigned int slen = sizeof(si_other);
  int msgId = 0;

  while (true) {
    printf("===========================================================\n");
    printf("Aguardando solicitacao...\n");
    fflush(stdout);

    /* Bloquear enquanto espera alguma entrada de dados
     * Note que podemos receber mensagens de clientes com 43 bytes ou de outros
     * servents com 204 bytes, por isso o buffer sempre espera pelo maior deles*/
    if ((recv_len = recvfrom(sockfd, buf, IN_BYTES_SRV, 0, (struct sockaddr *) &si_other, &slen)) == -1) {
      pwar("recvfrom(): Problema durante ultima chamada da funcao");
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
        printf("Enviando QUERY aos meus vizinhos\n");
        uint16_t TTL = 3;
        uint8_t byte_array[IN_BYTES_SRV];
        pack_query(QUERY, TTL, si_other.sin_addr.s_addr, si_other.sin_port, seqNum, keyReceived, byte_array, IN_BYTES_SRV);

        // convertendo inteiros para strings
        std::ostringstream sPort, sSeqNum;
        sPort << ntohs(si_other.sin_port);
        sSeqNum << seqNum;


        string requestIdentifier = inet_ntoa(si_other.sin_addr) + sPort.str() + sSeqNum.str() + keyReceived;
        requestsReceived.insert(requestIdentifier);
        seqNum++;

        // Enviando query a cada um dos meus vizinhos
        for (int i = 0; i < numNiggas; i++) {
          if (sendto(sockfd, byte_array, IN_BYTES_SRV, 0, (struct sockaddr*) &(neighbours[i]), slen) == -1) {
            pwar("sendto(): Problema durante ultima chamada da funcao");
          }
        }
      }

      // Procurando chave no dicionario local
      if (hasFile) {
        std::map<string, string>::iterator it;
        it = dictionary.find(keyReceived);
        if (it != dictionary.end()) {
          printf("Chave encontrada no banco de dados com valor:\n==>%s\n", it->second.c_str());
          char *byte_array = (char*) malloc(OUT_BYTES_RES * sizeof(char));  
          pack_response(RESPONSE, it->first, it->second, byte_array, OUT_BYTES_RES);
          
          if (sendto(sockfd, byte_array, OUT_BYTES_RES, 0, (struct sockaddr*) &si_other, slen) == -1) {
            pwar("sendto(): Problema durante ultima chamada da funcao");
          } else
            printf("RESPONSE enviada ao cliente\n");

          free(byte_array);
        } else {
          printf("Chave solicitada nao encontrada na base de dados.\n");
        }
      } else {
        printf("Chave solicitada nao encontrada na base de dados.\n");
      }


    } else if (msgId == QUERY) { // mensagem vem de servent
      printf("Recebi QUERY de um vizinho\n");
      uint16_t ttl, portClient;
      uint32_t ipClient, inSeqNum;
      unpack_query(buf, &ttl, &ipClient, &portClient, &inSeqNum, keyReceived);
      ttl = ntohs(ttl);

      // Verificar se ja recebi essa mesma requisicao
      // Convertendo inteiros para strings
      std::ostringstream sPort, sSeqNum;
      sPort << ntohs(portClient);
      sSeqNum << ntohl(inSeqNum);

      struct in_addr clientIP;
      clientIP.s_addr = ipClient;

      printf("==> Cliente original: %s:%d, SeqNum: %u, Chave: %s\n", inet_ntoa(clientIP), ntohs(portClient), ntohl(inSeqNum), keyReceived);

      // Criando assinatura da mensagem IP + PORTA + SEQNUM + CHAVE
      string requestIdentifier = inet_ntoa(clientIP) + sPort.str() + sSeqNum.str() + keyReceived;
      printf("Identificacao do pacote: %s\n", requestIdentifier.c_str());

       // caso em que ja recebi a requisicao anteriormente
      if (requestsReceived.count(requestIdentifier)) {
        printf("Recebi solicitacao repetida, ignorando operacao.\n");
        continue;
      } else {
        requestsReceived.insert(requestIdentifier);
        ttl--;
        printf("Retransmitindo QUERY para meus vizinhos. Novo TTL: %u\n", ttl);
        if (ttl > 0) { // retrasmite aos meus vizinhos
          ttl = htons(ttl);
          memcpy(&(buf[2]), &ttl, sizeof(ttl)); // atualizando ttl na mensagem
          slen = sizeof(neighbours[i]);
          for (int i = 0; i < numNiggas; i++) {
            // Evitando reenvio ao vizinho que mandou a mensagem QUERY a mim
            if (si_other.sin_addr.s_addr == neighbours[i].sin_addr.s_addr and 
              si_other.sin_port == neighbours[i].sin_port)
              continue;

            if (sendto(sockfd, buf, IN_BYTES_SRV, 0, (struct sockaddr*) &(neighbours[i]), slen) == -1) {
              pwar("sendto(): Problema durante ultima chamada da funcao");
            }
          }
          if(numNiggas == 0)
            printf("==> Nao ha vizinhos para retransmitir\n");
        } else {
          printf("TTL final da mensagem igual a zero, nao retransmitirei ...\n");
        }
      }

      // Procurando chave no dicionario local
      if (hasFile) {
        std::map<string, string>::iterator it;
        it = dictionary.find(keyReceived);
        if (it != dictionary.end()){
          printf("Chave encontrada no banco de dados com valor:\n==>%s\n", it->second.c_str());
          char *byte_array = (char*) malloc(OUT_BYTES_RES * sizeof(char));
          struct sockaddr_in client;
          client.sin_family = AF_INET;
          client.sin_port = portClient;
          client.sin_addr.s_addr = ipClient;
          pack_response(RESPONSE, it->first, it->second, byte_array, OUT_BYTES_RES);

          if (sendto(sockfd, byte_array, OUT_BYTES_RES, 0, (struct sockaddr*) &client, slen) == -1) {
            pwar("sendto(): Problema durante ultima chamada da funcao");
          } else {
            printf("RESPONSE enviada ao cliente original\n");
          }

          free(byte_array);
        } else {
          printf("Chave solicitada nao encontrada na base de dados.\n");
        }
      } else {
        printf("Chave solicitada nao encontrada na base de dados.\n");
      }

    } else {
      pwar("Mensagem com tipo nao identificado.");
    }

    printf("===========================================================\n\n");

  }

  close(sockfd);
  free(neighbours);
  return 0;

}




bool validParameters (int argc, char** argv, struct sockaddr_in *neighbours, char** filename) {

  if (argc < 3) {
    fprintf(stderr, "Modo de executar: \
        \n./servant <porta> <base-de-dados> <ip:porta> ... <ip_n:porta_n>\n");
    return false;
  }

  *filename = argv[2];

  if(argc == 3)
    return true;

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

    printf("argc %d - i %d\n", argc, i);
    neighbours[i-3].sin_family = AF_INET;
    neighbours[i-3].sin_port = htons(atoi(port));
    neighbours[i-3].sin_addr.s_addr = inet_addr(ip);

    i++;
  }

  return true;

}

void pack_response(uint16_t code, string key, string value, char *byte_array, int byte_array_len) {



  string keyValue = string(key + '\t' + value + '\0');
  const char* keyValueChar = keyValue.c_str();

  code = htons(code);
  memset(byte_array, 0, sizeof(*byte_array) * byte_array_len);

  memcpy(&(byte_array[0]), &code, sizeof(code)); // RESPONSE
  memcpy(&(byte_array[2]), keyValueChar, strlen(keyValueChar) + 1); // +1 para incluir \0

}


void unpack_query(char *byte_array, uint16_t* ttl, uint32_t* ipClient, uint16_t* portClient,
  uint32_t* inSeqNum, char* key) {

  int keySize = strlen(&(byte_array[14]));
  memcpy(ttl, &(byte_array[2]), sizeof(*ttl)); // TTL
  memcpy(ipClient, &(byte_array[4]), sizeof(*ipClient)); // IP
  memcpy(portClient, &(byte_array[8]), sizeof(*portClient)); // PORTA
  memcpy(inSeqNum, &(byte_array[10]), sizeof(*inSeqNum)); // SEQ
  memcpy(key, &(byte_array[14]), keySize); //
  key[keySize] = '\0';

}


void pack_query(uint16_t code, uint16_t ttl, uint32_t ip, uint16_t port, uint32_t seq, char* key,
  uint8_t *byte_array, int byte_array_len) {

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

std::string tabTrim(std::string const& str){
    if(str.empty())
        return str;

    std::size_t firstScan = str.find_first_not_of('\t');
    std::size_t first     = firstScan == std::string::npos ? str.length() : firstScan;
    std::size_t last      = str.find_last_not_of('\t');

    return str.substr(first, last-first+1);
}

std::string alltrim(std::string const& str){
    if(str.empty())
        return str;

    std::size_t firstScan = str.find_first_not_of(' ');
    std::size_t first     = firstScan == std::string::npos ? str.length() : firstScan;
    std::size_t last      = str.find_last_not_of(' ');

    return tabTrim(str.substr(first, last-first+1));
}

void getMessageType (int *msgId, char * buf){

    uint16_t code_net;
    memcpy(&code_net, &(buf[0]), sizeof(code_net));
    *msgId = ntohs(code_net);

}

void getKey (char * key, char * buf){

    strcpy(key, &buf[2]);

}