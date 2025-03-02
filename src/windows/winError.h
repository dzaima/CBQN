#include <windows.h>

// memory is allocated and should be freed with LocalFree()
// for now this memory is leaked
static char* winErrorEx(DWORD dwError) {
  char* buffer = NULL;
  DWORD dwFlags = FORMAT_MESSAGE_MAX_WIDTH_MASK // no newlines
    | FORMAT_MESSAGE_ALLOCATE_BUFFER            // allocate memory
    | FORMAT_MESSAGE_FROM_SYSTEM                // get error message
    | FORMAT_MESSAGE_IGNORE_INSERTS;            // no argument (NULL)
  DWORD dwResult = FormatMessageA(dwFlags, NULL, dwError, 0, (LPSTR)&buffer, 0, NULL);
  if (dwResult==0 || buffer==NULL) {
    fatal("Failed to get error message from FormatMessageA()");
  }
  return buffer;
}

static char* winError(void) { return winErrorEx(GetLastError()); }
