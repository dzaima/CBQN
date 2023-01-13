
#include "realpath.h"

char* realpath (const char *__restrict path, char *__restrict resolved_path) {
  return _fullpath(NULL, path, 0);
}
bool winIsAbsolute(const char* path) { // TODO something more proper
  return *path && path[1]==':' && (!path[2] || path[2]=='/' || path[2]=='\\');
}