#pragma once
#include "utf.h"
// unless otherwise specified, these assume that paths are strings

B path_rel(B base, B rel, char* name); // consumes rel; assumes base is a string or bi_N, throws if !isStr(rel)
B path_parent(B path); // consumes; returns the containing directory, with trailing slash
B path_name(B path); // consumes; returns filename from a path
B path_abs(B path); // consumes; returns absolute version of the path; propagates bi_N

FILE* file_open(B path, char* desc, char* mode); // doesn't consume path
I8Arr* path_bytes(B path); // consumes
B path_chars(B path); // consumes
B path_lines(B path); // consumes

I8Arr* stream_bytes(FILE* f);

typedef struct { char* data; bool alloc; } CharBuf;
CharBuf get_chars(B x); // convert x to character data; expects x isn't freed before free_chars call. May error.
void free_chars(CharBuf b); // free the result of the above

B mmap_file(B path); // consumes
bool dir_create(B path); // doesn't consume
bool path_rename(B old_path, B new_path); // consumes only old_path
bool path_remove(B path); // consumes

void path_wChars(B path, B x); // consumes path
void path_wBytes(B path, B x); // consumes path
void file_wBytes(FILE* file, char* name, B x); // doesn't consume

B path_list(B path); // consumes
char path_type(B path); // consumes; errors only if path isn't a string
B path_info(B path, i32 mode); // consumes; mode: 0:created 1:accessed 2:modified 3:size
void cbqn_heapDump(char* name);
