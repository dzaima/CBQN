#!/usr/bin/env bash
make heapverify && echo 'heapverify:'   && ./BQN -M 1000 "$1/test/this.bqn" -noerr bytecode header identity literal namespace prim simple syntax token under undo unhead || exit
make   rtverify && echo   'rtverify:'   && ./BQN -M 1000 "$1/test/this.bqn" || exit
echo 'gcc:';make CC=gcc t='gcc_basic' c && ./BQN -M 1000 "$1/test/this.bqn" || exit

