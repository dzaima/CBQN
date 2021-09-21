#pragma once
#include "utf.h"

B path_resolve(B base, B rel); // consumes rel; assumes base is a char vector or bi_N
B path_dir(B path); // consumes; returns directory part of file path, with trailing slash
B path_abs(B path); // consumes; returns absolute version of the path; propagates bi_N

FILE* file_open(B path, char* desc, char* mode); // doesn't consume path
I8Arr* file_bytes(B path); // consumes
B file_chars(B path); // consumes
B file_lines(B path); // consumes

I8Arr* stdin_allBytes();

void file_wChars(B path, B x); // consumes path
void file_wBytes(B path, B x); // consumes path

B file_list(B path); // consumes
