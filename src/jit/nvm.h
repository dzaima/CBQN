#pragma once
#include "../vm.h"

B evalJIT(Body* b, Scope* sc, u8* ptr);
typedef struct Nvm_res { u8* p; B refs; } Nvm_res;
Nvm_res m_nvm(Body* b);
void nvm_free(u8* ptr);
