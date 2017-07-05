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

// Enumeracao para possiveis tipos de mensagem
typedef enum msgTypes_ {CLIREQ = 1, QUERY = 2, RESPONSE = 3} msgType;

// enumeracao para possiveis applicacoes executadas
typedef enum appTypes_ {CLIENT, SERVENT} appType;


void getMessageType(int &msgId, char * buf);

void getKey(char * key, char * buf);

// Impressao de erro na tela do usuario e finalizacao da aplicacao
void perr (const char *str);

// Empacotar dados da mensagem a ser enviada em um unico byte array
void pack(uint16_t code, char *data, char *byte_array, int byte_array_len);

// Desempacotar byte array para extrair dados da mensagem recebida
uint16_t unpack(char *byte_array, char *data, int data_len);

/*// Funcao para checar se a quantidade de parametros passados e adequada
bool validParameters (int argc, appType ap);
*/