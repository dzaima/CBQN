#include <windows.h>

// https://github.com/libuv/libuv/blob/v1.23.0/src/win/process.c#L454-L524

static char* winQuoteCmdArg(u64 len, char* source, char* target) {
  if (len == 0) {
    // Need double quotation for empty argument 
    *(target++) = '"'; 
    *(target++) = '"'; 
    return target;
  }
  if (NULL == strpbrk(source, " \t\"")) {
    // No quotation needed 
    memcpy(target, source, len * sizeof(char)); target += len;
    return target;  
  }
  if (NULL == strpbrk(source, "\"\\")) {
    // No embedded double quotes or backlashes, so I can just wrap
    // quote marks around the whole thing.
    *(target++) = '"';
    memcpy(target, source, len * sizeof(char)); target += len;
    *(target++) = '"';
    return target;
  }

  *(target++) = '"';
  char *start = target;
  int quote_hit = 1;

  for (u64 i = 0; i < len; ++i) {
    *(target++) = source[len - 1 - i];
    
    if (quote_hit && source[len - 1 - i] == '\\') {
      *(target++) = '\\';
    } else if (source[len - 1 - i] == '"') {
      quote_hit = 1;
      *(target++) = '\\';
    } else {
      quote_hit = 0;
    }
  }
  target[0] = '\0'; _strrev(start);
  *(target++) = '"';
  return target;
}

typedef struct {
  HANDLE hndl;
  char* buf;
  u64 len;
} ThreadIO;

static DWORD WINAPI winThreadWrite(LPVOID arg0) {
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

static DWORD WINAPI winThreadRead(LPVOID arg0) {
  DWORD dwResult = ERROR_SUCCESS;
  ThreadIO* arg = arg0;
  HANDLE hndl = arg->hndl;
  u8 buf[1024] = {0};
  const usz bufSize = sizeof(buf)/sizeof(u8);
  DWORD dwRead = 0, dwHasRead = 0;
  char* rBuf = NULL;

  for (;;) {
    ZeroMemory(buf, bufSize);
    BOOL bOk = ReadFile(hndl, buf, bufSize, &dwRead, NULL);
    if (!bOk) {
      DWORD dwErr = GetLastError();
      if (dwErr == ERROR_BROKEN_PIPE) { break; }
      else { dwResult = dwErr; break; }
    }
    char* newBuf = (rBuf==NULL)?
      calloc(dwHasRead+dwRead, sizeof(char)) :
      realloc(rBuf, (dwHasRead+dwRead)*sizeof(char));
    if (newBuf == NULL) { dwResult = GetLastError(); break; }
    rBuf = newBuf;
    memcpy(&rBuf[dwHasRead], buf, dwRead);
    dwHasRead += dwRead;
  }

  if (dwResult != ERROR_SUCCESS) {
    if (dwHasRead > 0 && rBuf != NULL) { free(rBuf); }
  } else {
    arg->buf = rBuf;
    arg->len = dwHasRead;
  }
  CloseHandle(hndl);
  return dwResult;
}

static DWORD winCmd(char* arg, 
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
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  CreatePipe(&hInpR, &hInpW, &sa, 0);
  CreatePipe(&hOutR, &hOutW, &sa, 0);
  CreatePipe(&hErrR, &hErrW, &sa, 0);

  SetHandleInformation(hInpW, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(hOutR, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(hErrR, HANDLE_FLAG_INHERIT, 0);

  // Set up 
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.hStdInput = hInpR;
  si.hStdOutput = hOutW;
  si.hStdError = hErrW;
  si.dwFlags |= STARTF_USESTDHANDLES;

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  // Create the child process
  BOOL bSuccess = CreateProcessA(NULL, arg, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
  if (!bSuccess) { return GetLastError(); }

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
  lpThreads[0] = CreateThread(NULL, 0, winThreadWrite, (LPVOID)&data0, 0, NULL);
  lpThreads[1] = CreateThread(NULL, 0, winThreadRead,  (LPVOID)&data1, 0, NULL);
  lpThreads[2] = CreateThread(NULL, 0, winThreadRead,  (LPVOID)&data2, 0, NULL);

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
