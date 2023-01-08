
#include "getline.h"

ssize_t getline (char **lptr, size_t *n, FILE *fp) {
  wchar_t buf[MAX_LINE_LENGTH/3] = {0};
  DWORD bytes;
  DWORD chars, read_chars;

  int convertResult;
  char *m;

  bytes = MAX_LINE_LENGTH;
  chars = bytes/3;

  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  
  if (!ReadConsoleW(hIn, &buf, chars, &read_chars, NULL)) {
    fprintf(stderr, "Failed to read console input: %d", GetLastError());
    goto error;
  }

  convertResult = WideCharToMultiByte(CP_UTF8, 0, buf, -1, NULL, 0, NULL, NULL);
  if (convertResult == 0) {
    fprintf(stderr, "Failed to get MultiByte length: %d", GetLastError());
    goto error;
  }

  m = *lptr = (char*) calloc(convertResult, sizeof(char));

  if (WideCharToMultiByte(CP_UTF8, 0, buf, -1, m, convertResult, NULL, NULL) == 0 ) {
    fprintf(stderr, "Failed to convert wide characters: %d", GetLastError());
    free(m);
    goto error;
  }
  
  return convertResult-1;

error:
  CloseHandle(hIn);
  return -1;
}