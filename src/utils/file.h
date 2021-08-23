#pragma once
#include "utf.h"

typedef struct TmpFile { // to be turned into a proper I8Arr
  struct Arr;
  i8 a[];
} TmpFile;

B path_resolve(B base, B rel); // consumes rel; assumes base is a char vector or bi_N
B path_dir(B path); // consumes; returns directory part of file path, with trailing slash
B path_abs(B path); // consumes; returns absolute version of the path; propagates bi_N

FILE* file_open(B path, char* desc, char* mode); // doesn't consume path
TmpFile* file_bytes(B path); // consumes
B file_chars(B path); // consumes
B file_lines(B path); // consumes

void file_wChars(B path, B x); // consumes path
void file_wBytes(B path, B x); // consumes path

B file_list(B path); // consumes
