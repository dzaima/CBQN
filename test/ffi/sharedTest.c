#include <stdlib.h>
#include <stdio.h>
#include "../../include/bqnffi.h"

int main() {
  bqn_init();
  double res = bqn_toF64(bqn_evalCStr("2+2"));
  printf("%g\n", res);
}
