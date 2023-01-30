#!/usr/bin/env bash
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 path/to/mlochbaum/BQN"
  exit
fi
make heapverify && echo 'heapverify:' && ./BQN -M 1000 "$1/test/this.bqn" -noerr bytecode header identity literal namespace prim simple syntax token under undo unhead || exit
make   rtverify && echo   'rtverify:' && ./BQN -M 1000 "$1/test/this.bqn" || exit
make CC=gcc c   && echo        'gcc:' && ./BQN -M 1000 "$1/test/this.bqn" || exit

