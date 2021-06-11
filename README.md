build/run:

1. (optional) run `./genRuntime path/to/mlochbaum/BQN`; Otherwise, the necessary precompiled bytecode will be retrieved from `remotes/origin/bytecode`
2. If needed, customize settings in `src/h.h`
3. `make`
  - Options: `make o3`, `make debug` (`make rtperf`, `make heapverify` and `make rtverify` also exist for further testing/debugging)
  - Do `make clean` or `make [o3|debug|…]-clean` before to force recompile
  - `./build` and `./debugBuild` compile everything at once and allow specifying extra compiler arguments, but may be slower
4. `./BQN` (or `rlwrap ./BQN` for a fancier interface)

Run tests with `./BQN mlochbaum/BQN/test/this.bqn` (with `-noerr` for a heapverify build).

Time REPL-executed safe prim tests: `./test.bqn mlochbaum/BQN -s prim > SP; time ./BQN<SP>/dev/null`

Test precompiled expression: `./precompiled.bqn mlochbaum/BQN "$PATH" '2+2'`

Any file without an explicit copyright message is copyright (c) 2021 dzaima, GNU GPLv3 - see LICENSE