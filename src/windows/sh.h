#include <windows.h>
#include <process.h>

typedef struct {
  HANDLE hndl;
  char* buf;
  u64 len;
} ThreadIO;

static unsigned int __stdcall winThreadWrite(void* arg0) {
  DWORD dwResult = ERROR_SUCCESS;
  ThreadIO* arg = arg0;
  HANDLE hndl = arg->hndl;
  char* wBuf = arg->buf;
  DWORD dwToWrite = arg->len, dwWritten = 0, dwOff = 0;

  for (;;) {
    BOOL bOk = WriteFile(hndl, &wBuf[dwOff], dwToWrite-dwOff, &dwWritten, NULL);
    if (!bOk) { dwResult = GetLastError(); break; }
    dwOff += dwWritten;
    if (dwOff >= dwToWrite) { break; }
  }

  CloseHandle(hndl);
  return dwResult;
}

static unsigned int __stdcall winThreadRead(void* arg0) {
  DWORD dwResult = ERROR_SUCCESS;
  ThreadIO* arg = arg0;
  HANDLE hndl = arg->hndl;
  u8 buf[4096] = {0};
  const usz bufSize = sizeof(buf)/sizeof(u8);
  DWORD dwRead = 0, dwHasRead = 0;
  char* rBuf = NULL;
  for (;;) {
    ZeroMemory(buf, bufSize);
    BOOL bOk = ReadFile(hndl, buf, bufSize, &dwRead, NULL);
    if (dwRead == 0) { break; }
    if (!bOk) {
      dwResult = GetLastError();
      break;
    }
    char* newBuf = realloc(rBuf, (dwHasRead+dwRead)*sizeof(char));
    if (newBuf == NULL) { dwResult = GetLastError(); break; }
    rBuf = newBuf;
    memcpy(&rBuf[dwHasRead], buf, dwRead);
    dwHasRead += dwRead;
  }

  if (dwResult != ERROR_SUCCESS) {
    if (rBuf != NULL) { free(rBuf); }
  } else {
    arg->buf = rBuf;
    arg->len = dwHasRead;
  }
  CloseHandle(hndl);
  return dwResult;
}

static DWORD winCmd(WCHAR* appName, WCHAR* cmdLine,
  u64 iLen, char* iBuf,
  DWORD* code,
  u64* oLen, char** oBuf,
  u64* eLen, char** eBuf) {

  DWORD dwResult = ERROR_SUCCESS;
  HANDLE hInpR, hInpW;
  HANDLE hOutR, hOutW;
  HANDLE hErrR, hErrW;

  // Create pipes
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  CreatePipe(&hInpR, &hInpW, &sa, 0);
  CreatePipe(&hOutR, &hOutW, &sa, 0);
  CreatePipe(&hErrR, &hErrW, &sa, 0);

  SetHandleInformation(hInpW, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(hOutR, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(hErrR, HANDLE_FLAG_INHERIT, 0);

  // Set up
  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.hStdInput = hInpR;
  si.hStdOutput = hOutW;
  si.hStdError = hErrW;
  si.dwFlags |= STARTF_USESTDHANDLES;

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  // Create the child process
  BOOL bSuccess = CreateProcessW(appName, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
  if (!bSuccess) {
    dwResult = GetLastError();
    // Close all handles
    CloseHandle(hInpR); CloseHandle(hInpW);
    CloseHandle(hOutR); CloseHandle(hOutW);
    CloseHandle(hErrR); CloseHandle(hErrW);
    return dwResult;
  }

  // Close the unneeded handles
  CloseHandle(hInpR);
  CloseHandle(hOutW);
  CloseHandle(hErrW);

  // Spawn the thread to deal with redirected io
  ThreadIO data0 = {hInpW, iBuf, iLen};
  ThreadIO data1 = {hOutR, NULL, 0};
  ThreadIO data2 = {hErrR, NULL, 0};

  DWORD exitCode = -1;
  HANDLE lpThreads[3];
  lpThreads[0] = (HANDLE)_beginthreadex(NULL, 0, winThreadWrite, (void*)&data0, 0, NULL);
  lpThreads[1] = (HANDLE)_beginthreadex(NULL, 0, winThreadRead,  (void*)&data1, 0, NULL);
  lpThreads[2] = (HANDLE)_beginthreadex(NULL, 0, winThreadRead,  (void*)&data2, 0, NULL);

  for (int i = 0; i < 3; ++i) {
    if (lpThreads[i] == NULL) { dwResult = GetLastError(); goto error; }
  }

  // Wait for the threads
  if (WaitForMultipleObjects(3, lpThreads, TRUE, INFINITE) == WAIT_FAILED) {
    dwResult = GetLastError(); goto error;
  }
  for (int i = 0; i < 3; ++i) {
    GetExitCodeThread(lpThreads[i], &exitCode);
    if (exitCode != ERROR_SUCCESS) { dwResult = exitCode; goto error; }
  }

  // Get the exit code
  if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED) {
    dwResult = GetLastError(); goto error;
  }
  exitCode = -1;
  GetExitCodeProcess(pi.hProcess, &exitCode);

  // Give outputs
  *code = exitCode;
  *oLen = data1.len; *oBuf = data1.buf;
  *eLen = data2.len; *eBuf = data2.buf;

error:
  // Close handles
  for (int i = 0; i < 3; ++i) { CloseHandle(lpThreads[i]); }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return dwResult;
}

// helpers

#define WIN_MY_ALLOC(T,N,AM) T* N = malloc((AM)*sizeof(T)); if (N == NULL) thrOOM();

static WCHAR* winWcsDup(WCHAR* str) {
  size_t len = wcslen(str);
  WIN_MY_ALLOC(WCHAR, dup, len+1);
  memcpy(dup, str, (len+1) * sizeof(WCHAR));
  return dup;
}

static WCHAR* winGetEnvVar(WCHAR* name) {
  DWORD dwSize = 1024;
  WCHAR* buffer = NULL;
  while (1) {
    WCHAR* newBuffer = realloc(buffer, dwSize * sizeof(WCHAR));
    if (newBuffer == NULL) { free(buffer); thrOOM(); }
    buffer = newBuffer;
    DWORD dwLength = GetEnvironmentVariableW(name, buffer, dwSize);
    if (dwLength == 0) { free(buffer); buffer = NULL; break; }
    if (dwLength < dwSize) { break; }
    dwSize = dwLength;
  }
  return buffer;
}

static bool winFileExists(WCHAR* path) {
  DWORD attrs = GetFileAttributesW(path);
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static void winSlashToPathSep(WCHAR* path) {
  if (path == NULL) { return; }
  for (WCHAR* p = path; *p != L'\0'; ++p) {
    if (*p == L'/') { *p = L'\\'; }
  }
}

// input should only use '\\' as path separators

static bool winHasExt(WCHAR* path) {
  const WCHAR* dot = wcsrchr(path, L'.');
  const WCHAR* sep = wcsrchr(path, L'\\');
  return dot != NULL && (sep == NULL || dot > sep);
}

static bool winHasPathSep(WCHAR* path) {
  const WCHAR* sep = wcschr(path, L'\\');
  return sep != NULL;
}

static WCHAR* winWcsJoin(WCHAR* a, size_t aLen, WCHAR* b, size_t bLen, WCHAR sep) {
  if (a == NULL || b == NULL) { return NULL; }
  size_t sepLen = sep==L'\0' ? 0 : 1;
  WIN_MY_ALLOC(WCHAR, path, aLen+sepLen+bLen+1);
  memcpy(path, a, aLen * sizeof(WCHAR));
  if (sepLen > 0) { path[aLen] = sep; }
  memcpy(path + aLen + sepLen, b, bLen * sizeof(WCHAR));
  path[aLen + sepLen + bLen] = L'\0';
  return path;
}

struct winWstrView {
  WCHAR* ptr;
  size_t len;
};

static void winTrimToken(struct winWstrView* token) {
  WCHAR* start = token->ptr;
  WCHAR* end = token->ptr + token->len;
  // trim leading and trailing quotes
  while (start < end && (*start == L'\'' || *start == L'\"')) { start++; }
  while (end > start && (*(end - 1) == L'\'' || *(end - 1) == L'\"')) { end--; }
  // trim trailing path separators
  while (end > start && (*(end - 1) == L'\\' || *(end - 1) == L'/')) { end--; }
  token->ptr = start;
  token->len = (size_t)(end - start);
}

static bool winNextSemiToken(WCHAR* input, size_t inputLen, size_t* cursor, struct winWstrView* token) {
  if (*cursor >= inputLen) { return false; }
  size_t start = *cursor;
  while (*cursor < inputLen && input[*cursor] != L';') {
    (*cursor)++;
  }
  token->ptr = input + start;
  token->len = *cursor - start;
  winTrimToken(token);
  if (*cursor < inputLen && input[*cursor] == L';') {
    (*cursor)++;
  }
  return true;
}

static WCHAR* winSearchInPath(WCHAR* name) {
  WCHAR* path = winGetEnvVar(L"PATH");
  // cannot find PATH env variable
  if (path == NULL) { return NULL; }

  WCHAR* resolved = NULL;
  size_t cursor = 0;
  size_t pathLen = wcslen(path);
  size_t nameLen = wcslen(name);
  struct winWstrView dir;
  while (winNextSemiToken(path, pathLen, &cursor, &dir)) {
    if (dir.len == 0) { continue; }

    WCHAR* candidate = winWcsJoin(dir.ptr, dir.len, name, nameLen, L'\\');
    if (winFileExists(candidate)) {
      resolved = candidate;
      break;
    }
    free(candidate);
  }

  free(path);
  return resolved;
}

// resolve application name for CreateProcessW (winCmd):
// 1. if the name already has a file extension, use it as is; otherwise, append ".exe"
// 2. if the name is a relative or absolute path, use it as is; otherwise, search for it in PATH
// PATHEXT is not used
// input should only use '\\' as path separators
static WCHAR* winResolveApplicationName(WCHAR* app) {
  static WCHAR exeExt[] = L".exe";
  static size_t exeExtLen = sizeof(exeExt) / sizeof(WCHAR) - 1;

  WCHAR* name = winWcsDup(app);

  if (!winHasExt(name)) {
    WCHAR* oldName = name;
    name = winWcsJoin(name, wcslen(name), exeExt, exeExtLen, L'\0');
    free(oldName);
  }

  if (winHasPathSep(name)) { return name; } // let CreateProcessW handle it

  WCHAR* resolved = winSearchInPath(name);
  winSlashToPathSep(resolved); // PATH entries may contain '/'
  free(name);
  return resolved;
}

#undef WIN_MY_ALLOC
