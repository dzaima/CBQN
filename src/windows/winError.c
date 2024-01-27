#include <windows.h>

static char* winErrorEx(DWORD dwError) {
  char* buffer = NULL;
  DWORD dwFlags = FORMAT_MESSAGE_MAX_WIDTH_MASK
    | FORMAT_MESSAGE_ALLOCATE_BUFFER
    | FORMAT_MESSAGE_FROM_SYSTEM 
    | FORMAT_MESSAGE_IGNORE_INSERTS;
  DWORD dwResult = FormatMessageA(dwFlags, NULL, dwError, 0, (LPSTR)&buffer, 0, NULL);
  if (dwResult==0 || buffer==NULL) {
    fatal("Failed to get error message from FormatMessageA()");
  }
  return buffer;
}

static char* winError(void) { return winErrorEx(GetLastError()); }
