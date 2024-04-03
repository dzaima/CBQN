#!/usr/bin/env bash
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 path/to/mlochbaum/BQN"
  exit
fi
make f='-DDEBUG -DHEAP_VERIFY -DUSE_SETJMP=0 -DJIT_START=0' single-c
echo 'alljit+heapverify:'                           && ./BQN -M 1000 "$1/test/this.bqn" -noerr bytecode header identity literal namespace prim simple syntax token under undo unhead || exit
echo 'native vfy:';make heapverifyn                 && ./BQN -M 1000 "$1/test/this.bqn" -noerr bytecode header identity literal namespace prim simple syntax token under undo unhead || exit
echo 'native:';make o3n                             && ./BQN -M 1000 "$1/test/this.bqn" || exit
echo 'sse2:';build/build singeli native=0           && ./BQN -M 1000 "$1/test/this.bqn" || exit
echo '32-bit:';make f='-DDEBUG -m32' single-c FFI=0 && ./BQN -M 1000 "$1/test/this.bqn" || exit
echo '(expected one failed test: 0.3‿1.01‿π(⋆⁼⌜≡÷˜⌜○(⋆⁼))0‿2‿5.6‿∞)'
