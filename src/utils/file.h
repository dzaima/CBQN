#pragma once
#include "utf.h"

typedef struct TmpFile { // to be turned into a proper I8Arr
  struct Arr;
  i8 a[];
} TmpFile;

B path_resolve(B base, B rel); // consumes rel; assumes base is a char vector or bi_N
B path_dir(B path); // consumes; returns directory part of file path, with trailing slash

TmpFile* file_bytes(B path); // consumes
B file_chars(B path); // consumes

void file_write(B path, B x); // consumes path

B file_list(B path); // consumes
