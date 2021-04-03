#include "h.h"
#include<stdlib.h>

void mm_free(Value* x) {
  onFree(x);
  free(x);
}

void* mm_allocN(usz sz, u8 type) {
  Value* x = malloc(sz);
  onAlloc(sz, type);
  x->flags = x->extra = x->mmInfo = x->type = 0;
  x->refc = 1;
  x->type = type;
  return x;
}
void mm_visit(B x) {
  
}
