Build & run (tl;dr: `make; rlwrap ./BQN`):

1. (optional) Run `./genRuntime path/to/mlochbaum/BQN`; Otherwise, the necessary precompiled bytecode will be retrieved from `remotes/origin/bytecode`
2. If wanted, customize settings in `src/h.h`
3. `make`
    - Options: `make o3` (the default), `make debug` (more presets exist for more specific debugging)
    - `make CC=gcc` can be used to compile with gcc. Otherwise, `clang` is used.
    - Do `make clean` or `make t=[o3|debug|…] clean` beforehand to force recompile
    - `make single-(o3|o3g|debug|c)` compile everything as a single translation unit and thus will compile slower, but allows specifying extra compiler arguments with `make f='…' single-…` and allows the compiler to optimize more
4. `./BQN` (or `rlwrap ./BQN` for a better REPL; see `./BQN --help` for more options)

Run tests with `./BQN mlochbaum/BQN/test/this.bqn` (add `-noerr` for a heapverify build).

Time REPL-executed safe prim tests: `./test.bqn mlochbaum/BQN -s prim > SP; time ./BQN<SP>/dev/null`

Test precompiled expression: `./precompiled.bqn mlochbaum/BQN "$PATH" '2+2'`

Any file without an explicit copyright message is copyright (c) 2021 dzaima, GNU GPLv3 - see LICENSE