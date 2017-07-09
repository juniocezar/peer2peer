#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>

// Enumeracao para possiveis tipos de mensagem
typedef enum msgTypes_ {CLIREQ = 1, QUERY = 2, RESPONSE = 3} msgType;

// enumeracao para possiveis applicacoes executadas
typedef enum appTypes_ {CLIENT, SERVENT} appType;


// Impressao de erro na tela do usuario e finalizacao da aplicacao
void perr (const char *str);


// Impressao de waring na tela do usuario
void pwar (const char *str);
