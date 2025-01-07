#include <windows.h>
#include "../utils/talloc.h"

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
  DWORD dwRead = 0;
  TSALLOC(char, rBuf, 8);

  for (;;) {
    ZeroMemory(buf, bufSize);
    BOOL bOk = ReadFile(hndl, buf, bufSize, &dwRead, NULL);
    if (!bOk) {
      DWORD dwErr = GetLastError();
      if (dwErr == ERROR_BROKEN_PIPE) { break; }
      else { dwResult = dwErr; break; }
    }
    TSADDA(rBuf, buf, dwRead);
  }

  if (dwResult == ERROR_SUCCESS) {
    arg->len = TSSIZE(rBuf);
    arg->buf = TALLOCP(char, arg->len);
    memcpy(arg->buf, rBuf, arg->len);
  }
  TSFREE(rBuf);
  CloseHandle(hndl);
  return dwResult;
}

static DWORD winCmd(WCHAR* arg, 
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
  BOOL bSuccess = CreateProcessW(NULL, arg, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
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
