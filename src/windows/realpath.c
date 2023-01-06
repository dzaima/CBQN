
#include "realpath.h"

char* realpath (const char *__restrict path, char *__restrict resolved_path) {
  unsigned long len=strlen(path)+1;
  void* r = malloc(len);
  memcpy(r, path, len);
  return r;
}