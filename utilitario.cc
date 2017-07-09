#include "utilitario.h"


void perr (const char *str) {

  fprintf(stderr, "***********************************************************\n");
  fprintf(stderr, "ERROR: %s\n", str);
  fprintf(stderr, "***********************************************************\n");
  exit(EXIT_FAILURE);

}


void pwar (const char *str) {

  fprintf(stdout, "***********************************************************\n");
  fprintf(stdout, "Warning: %s\n", str);
  fprintf(stdout, "***********************************************************\n");

}
