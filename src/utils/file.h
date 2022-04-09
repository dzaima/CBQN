#pragma once
#include "utf.h"

B path_rel(B base, B rel); // consumes rel; assumes base is a char vector or bi_N
B path_dir(B path); // consumes; returns directory part of file path, with trailing slash
B path_name(B path); // consumes; returns filename from a path
B path_abs(B path); // consumes; returns absolute version of the path; propagates bi_N

FILE* file_open(B path, char* desc, char* mode); // doesn't consume path
I8Arr* path_bytes(B path); // consumes
B path_chars(B path); // consumes
B path_lines(B path); // consumes

I8Arr* stream_bytes(FILE* f);

B mmap_file(B path); // consumes

void path_wChars(B path, B x); // consumes path
void path_wBytes(B path, B x); // consumes path
void file_wBytes(FILE* file, B name, B x); // consumes x

B path_list(B path); // consumes
char path_type(B path); // consumes
void cbqn_heapDump();
