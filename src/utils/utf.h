#pragma once

void fprintCodepoint(FILE* f, u32 v); // single codepoint
void fprintsUTF8(FILE* f, char* s); // UTF-8, null-terminated
void fprintsU32(FILE* f, u32* s, usz len); // unicode codepoints
void fprintsB(FILE* f, B x); // codepoints of x; doesn't consume; assumes argument is an array, doesn't care about rank, throws if its elements are not characters

u64 utf8lenB(B x); // doesn't consume; may error as it verifies whether is all chars
void toUTF8(B x, char* p); // doesn't consume; doesn't verify anything; p must have utf8lenB(x) bytes (calculating which should verify that this call is ok), add trailing null yourself
