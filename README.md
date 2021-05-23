build/run:

1. `./genRuntime path/to/mlochbaum/BQN`
2. Optionally choose what to build by changing `src/h.h`
3. `make`
  - Options: `make o3`, `make debug` (`make rtperf` and `make heapverify` may also be useful)
  - Do `make clean` or `make [o3|debug]-clean` before to force recompile
  - `./build` and `./debugBuild` compile everything at once and allow specifying extra compiler arguments, but may be slower
4. `./BQN` (or `rlwrap ./BQN` for a fancier interface)

Time safe prim tests with self-hosted compiler: `./test.bqn ~/git/BQN -s prim > SP; time ./BQN<SP>/dev/null`

Test precompiled expression: `./precompiled.bqn path/to/mlochbaum/BQN "$PATH" '2+2'`

Any file without an explicit copyright message is copyright (c) 2021 dzaima, GNU GPLv3 - see LICENSE