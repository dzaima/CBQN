#!/usr/bin/env bash
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 path/to/mlochbaum/BQN"
  exit
fi
make              && ./BQN -p 2+2               || exit
make single-debug && ./BQN -p 2+2               || exit
make heapverify   && ./BQN -p 2+2               || exit
make rtverify     && ./BQN -p 2+2               || exit
make rtperf       && ./BQN -p 2+2 | head -2     || exit
make f='-DDEBUG -DDEBUG_VM'   c && ./BQN -p 2+2 | tail -2 || exit
make f='-DWARN_SLOW'          c && ./BQN -p 2+2 2> /dev/null || exit
make f='-DMM=0 -DENABLE_GC=0' c && ./BQN -p 2+2 || exit
make f='-DMM=1'               c && ./BQN -p 2+2 || exit
make f='-DMM=2'               c && ./BQN -p 2+2 || exit
make f='-DTYPED_ARITH=0'      c && ./BQN -p 2+2 || exit
make f='-DFAKE_RUNTIME'       c && ./BQN -p 2+2 || exit
make f='-DALL_R0'             c && ./BQN -p 2+2 || exit
make f='-DALL_R1'             c && ./BQN -p 2+2 || exit
make f='-DALL_R0 -DALL_R1'    c && ./BQN -p 2+2 || exit
make f='-DSFNS_FILLS=0'       c && ./BQN -p 2+2 || exit
make f='-DFORMATTER=0'        c && ./BQN -p 2+2 || exit
make f='-DVMPOS=0'            c && ./BQN -p 2+2 || exit
make f='-DDONT_FREE'          c && ./BQN -p 2+2 || exit
make f='-DOBJ_COUNTER'        c && ./BQN -p 2+2 || exit
make f='-DNO_RT'              c                 || exit; echo "(nothing executed for this test)"
make f='-DLOG_GC'             c && ./BQN -p 2+2 || exit
make f='-DWRITE_ASM'          c && ./BQN -p 2+2 || exit
make f='-DUSE_PERF'           c && ./BQN -p 2+2 || exit
make f='-DUSZ_64'             c && ./BQN -p 2+2 || exit
make f='-DREPL_INTERRUPT=0'   c && ./BQN -p 2+2 || exit
make f='-DREPL_INTERRUPT=1'   c && ./BQN -p 2+2 || exit
make FFI=0                    c && ./BQN -p 2+2 || exit
dbqn test/precompiled.bqn "$1" "$PATH" '2+2'    || exit
make f='-DNO_RT -DPRECOMP'    c && ./BQN        || exit
