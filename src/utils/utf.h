#pragma once

B fromUTF8(char* s, i64 len);
B fromUTF8l(char* s);

void printUTF8(u32 c);

u64 utf8lenB(B x); // doesn't consume; may error as it verifies whether is all chars
void toUTF8(B x, char* p); // doesn't consume; doesn't verify anything; p must have utf8lenB(x) bytes (calculating which should verify that this call is ok), add trailing null yourself
