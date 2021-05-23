#pragma once
#include "utf.h"

typedef struct TmpFile { // to be turned into a proper I8Arr
  struct Arr;
  i8 a[];
} TmpFile;

B path_resolve(B base, B rel); // consumes rel; may error; assumes base is a char vector or bi_N
B path_dir(B path); // consumes; returns directory part of file path, with trailing slash; may error

TmpFile* file_bytes(B path); // consumes; may throw
B file_chars(B path); // consumes; may throw
void file_write(B path, B x); // consumes path; may throw
