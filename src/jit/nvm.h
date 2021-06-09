#pragma once
#include "../vm.h"

B evalJIT(Body* b, Scope* sc, u8* ptr);
u8* m_nvm(Body* b);
void nvm_free(u8* ptr);
