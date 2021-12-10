# A [BQN](https://github.com/mlochbaum/BQN) implementation in C

## Running

1. `make`
    - `make CC=gcc` if you don't have clang installed
    - `make PIE=""` on ARM CPUs (incl. Android & M1)
    - `make clean` if anything goes bad and you want a clean slate
2. `./BQN somefile.bqn` to execute a file, or `rlwrap ./BQN` for a REPL

## Configuration options

- `some-other-bqn-implementation ./genRuntime path/to/mlochbaum/BQN` can be used to avoid pulling precompiled bytecode with git from `remotes/origin/bytecode`.  
  This creates the dummy file `src/gen/customRuntime` and will disable automated bytecode retrieval whenever it's updated.  
  `make clean-runtime` (or just `make-clean`) can be used to reset to the default state.
- Different build types:
    - `make o3` - `-O3`, the default build
    - `make o3n` - `-O3 -march=native`
    - `make o3g` - `-g -O3`
    - `make debug` - unoptimized debug build
    - `make debug1` - debug build without parallel compilation. Useful if everything errors, and you don't want error messages of multiple threads to be written at the same time.
    - `make heapverify` - verify that refcounting is done correctly
    - `make t=some_type clean` - clean only the specific build
    - `make o3n-singeli` - a Singeli build, currently only for CPUs supporting AVX2
    - `make t=some_custom_type f='-O3 -DSOME_MACRO=whatever -some_other_cc_flag' c` - custom build  
      Macros that you may want to define are listed in `src/h.h`.  
      The `some_custom_type` is used as the key for caching/incremental compilation, so make sure to `make t=some_custom_type clean` if you want to change the flags without changing the `t=` value!!
    - `make single-(o3|o3g|debug|c)` - compile everything as a single translation unit. Slower for optimized builds, but may allow some more optimizations
    - ... and more; see `makefile`
- Tests can be run with `./BQN path/to/mlochbaum/BQN/test/this.bqn` (add `-noerr` if using `make heapverify`).
- Test precompiled expression: `some-other-bqn-impl ./precompiled.bqn path/to/mlochbaum/BQN "$PATH" '2+2'`
- [Some implementation docs](https://github.com/dzaima/CBQN/tree/master/src#readme)

## License

Any file without an explicit copyright message is copyright (c) 2021 dzaima, GNU GPLv3 - see LICENSE