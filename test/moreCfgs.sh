#!/bin/sh
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 path/to/mlochbaum/BQN"
  exit
fi
make                                    && ./BQN -p 2+2                || exit
make single-debug                       && ./BQN -p 2+2                || exit
make heapverify                         && ./BQN -p 2+2                || exit
make rtverify                           && ./BQN -p 2+2                || exit
build/build f='-DDEBUG -DDEBUG_VM'    c && ./BQN -p 2+2 2>&1 | tail -2 || exit
build/build f='-DWARN_SLOW'           c && ./BQN -p 2+2 2> /dev/null   || exit
build/build f='-DMM=0 -DENABLE_GC=0'  c && ./BQN -p 2+2 || exit
build/build f='-DMM=1'                c && ./BQN -p 2+2 || exit
build/build f='-DMM=2'         debug  c && ./BQN "$1/test/this.bqn" || exit
build/build usz=64 debug singeli arch=generic c && ./BQN "$1/test/this.bqn" || exit
build/build f='-DTYPED_ARITH=0'       c && ./BQN -p 2+2 || exit
build/build f='-DFAKE_RUNTIME'        c && ./BQN -p 2+2 || exit
build/build f='-DALL_R0'              c && ./BQN -p 2+2 || exit
build/build f='-DALL_R1'              c && ./BQN -p 2+2 || exit
build/build f='-DALL_R0 -DALL_R1'     c && ./BQN -p 2+2 || exit
build/build f='-DSFNS_FILLS=0'        c && ./BQN -p 2+2 || exit
build/build f='-DFORMATTER=0'         c && ./BQN -p 2+2 || exit
build/build f='-DVMPOS=0'             c && ./BQN -p 2+2 || exit
build/build f='-DDONT_FREE'           c && ./BQN -p 2+2 || exit
build/build f='-DOBJ_COUNTER'         c && ./BQN -p 2+2 || exit
build/build f='-DNO_RT'               c && ./BQN -p 2+2 || exit
build/build f='-DNATIVE_COMPILER'     c && ./BQN -p 2+2 || exit
build/build f='-DNATIVE_COMPILER -DONLY_NATIVE_COMP -DFORMATTER=0 -DNO_RT -DNO_EXPLAIN' c && ./BQN -p 2+2 || exit
build/build f='-DGC_LOG_DETAILED'     c && ./BQN -p 2+2 || exit
build/build f='-DUSE_PERF'            c && ./BQN -p 2+2 || exit
build/build f='-DREPL_INTERRUPT=0'    c && ./BQN -p 2+2 || exit
build/build f='-DREPL_INTERRUPT=1'    c && ./BQN -p 2+2 || exit
build/build FFI=0                     c && ./BQN -p 2+2 || exit
build/obj2/for_build4 test/precompiled.bqn "$1" "$PATH" '2+2' || exit
build/build f='-DNO_RT -DPRECOMP'     c && ./BQN        || exit
