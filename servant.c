/*******************************************************************************
* Redes de Computadores - Um Sistema Peer-to-peer de Armazenamento Chave-valor *
* Junio Cezar Ribeiro da Silva - 2012075597                                    *
* Servant                                                                      *
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
#include <fstream>

using std::string;

void perr (const char *str) {
  fprintf(stderr, "***********************************************************\n");
  fprintf(stderr, "ERROR: %s\n", str);
  fprintf(stderr, "***********************************************************\n");
  exit(EXIT_FAILURE);
}

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

typedef struct Neighbours {
  string ip;
  string port;
} Neighbours;

int main (int argc, char** argv) {
  if (argc <= 3) {
    fprintf(stderr, "Modo de executar: \
    \n./servant <porta> <base-de-dados> <ip:porta> ... <ip_n:porta_n>\n");
    return 1;
  }

  int port   = atoi(argv[1]);
  char* data_base_name = argv[2];
  Neighbours neighbours[argc - 3];
  std::map <string, string> dictionary;

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

    neighbours[i-3].ip   = string(ip);
    neighbours[i-3].port = string(port);
    i++;
  }


  // Abrindo base de dados para criacao do dicionario
  //FILE *data_base = fopen(data_base_name, "r");
  std::ifstream data_base(data_base_name);

  if (data_base == NULL) {
    fprintf(stderr, "Erro ao abrir o arquivo: %s\nAbortando execucao ...\n", data_base_name);
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
  i = 1;
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

  for (int i = 0; i < argc - 3; i++) {
    printf("(%s,%s) ", neighbours[i].ip.c_str(), neighbours[i].port.c_str());
    if (i % 3 == 0)
      printf("\n");
  }
  printf("\n");

  char buf[1000];
  int BUFLEN = 1000, recv_len;
  unsigned int slen = sizeof(si_other);
  while (true) {
    printf("Aguardando solicitacao...\n");
    fflush(stdout);

    // bloquear enquanto espera alguma entrada de dados
    if ((recv_len = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1) {
      perr("recvfrom()");
    }

    // imprimir detalhes da host que nos enviou dados
    printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
    printf("Data: %s\n" , &buf[2]);
    printf("\nEstou ignorando o cliente, não irei responder...\n\n");
    /*if (sendto(sockfd, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1) {
      perr("sendto()");
    }*/

  }

  close(sockfd);
  return 0;
}
