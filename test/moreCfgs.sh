#!/usr/bin/env bash
make && ./BQN -p 2+2 || exit
make single-debug && ./BQN -p 2+2 || exit
make heapverify && ./BQN -p 2+2 || exit
make rtverify && ./BQN -p 2+2 || exit
make rtperf && ./BQN -p 2+2 | head -2 || exit
make t=mc_vmdbg f='-DDEBUG -DDEBUG_VM' c && ./BQN -p 2+2 | tail -2 || exit
make t=mc_mm0 f='-DMM=0 -DENABLE_GC=0' c && ./BQN -p 2+2 || exit
make t=mc_mm1 f='-DMM=1' c && ./BQN -p 2+2 || exit
make t=mc_mm2 f='-DMM=2' c && ./BQN -p 2+2 || exit
make t=mc_tyar0 f='-DTYPED_ARITH=0' c && ./BQN -p 2+2 || exit
make t=mc_frt f='-DFAKE_RUNTIME' c && ./BQN -p 2+2 || exit
make t=mc_allr0 f='-DALL_R0' c && ./BQN -p 2+2 || exit
make t=mc_allr1 f='-DALL_R1' c && ./BQN -p 2+2 || exit
make t=mc_allr01 f='-DALL_R0 -DALL_R1' c && ./BQN -p 2+2 || exit
make t=mc_nosff f='-DSFNS_FILLS=0' c && ./BQN -p 2+2 || exit
make t=mc_nofmt f='-DFORMATTER=0' c && ./BQN -p 2+2 || exit
make t=mc_novmpos f='-DVMPOS=0' c && ./BQN -p 2+2 || exit
make t=mc_dontfree f='-DDONT_FREE' c && ./BQN -p 2+2 || exit
make t=mc_objctr f='-DOBJ_COUNTER' c && ./BQN -p 2+2 || exit
make t=mc_nort f='-DNO_RT' c || exit
./precompiled.bqn "$1" "$PATH" '2+2' || exit
make t=mc_nortpre f='-DNO_RT -DPRECOMP' c && ./BQN || exit
make t=mc_loggc f='-DLOG_GC' c && ./BQN -p 2+2 || exit
make t=mc_writeasm f='-DWRITE_ASM' c && ./BQN -p 2+2 || exit
make t=mc_useperf f='-DUSE_PERF' c && ./BQN -p 2+2 || exit
