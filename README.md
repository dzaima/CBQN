# A [BQN](https://github.com/mlochbaum/BQN) implementation in C

[CBQN-specific documentation](docs/README.md) • [source code overview](src/README.md)

## Running

1. `make`
    - `make CC=gcc` if you don't have clang installed
    - `make PIE=""` on ARM CPUs (incl. Android & M1)
    - `gmake` on BSDs
    - `make clean` if anything goes bad and you want a clean slate
    - `make [...]; sudo make install` to install into `/usr/local/bin/bqn`; `sudo make uninstall` to uninstall
    - If you want to use custom build types but your system doesn't have `shasum`/`sha256sum`, add `force_build_dir=some_identifier`. That identifier will be used to decide on the directory for incremental build object files.
2. `./BQN somefile.bqn` to execute a file, or `rlwrap ./BQN` for a REPL

## Configuration options

- `some-other-bqn-implementation ./genRuntime path/to/mlochbaum/BQN` can be used to avoid pulling precompiled bytecode with git from `remotes/origin/bytecode`.  
  This creates the dummy file `src/gen/customRuntime`, which will disable automated bytecode retrieval, leading to the bytecode going out-of-sync whenever it needs updating.  
  `make clean-runtime` (which is included in `make clean`) can be used to reset this to the default state.
- Different build types:
    - `make o3` - `-O3`, the default build
    - `make o3n` - `-O3 -march=native`
    - `make o3g` - `-g -O3`
    - `make debug` - unoptimized debug build
    - `make debug1` - debug build without parallel compilation. Useful if everything errors, and you don't want error messages of multiple threads to be written at the same time.
    - `make heapverify` - verify that refcounting is done correctly
    - `make o3n-singeli` - a Singeli build, currently only for x86-64 CPUs supporting AVX2
    - `make c` - a build with no flags, for manual customizing
    - `make single-(o3|o3g|debug|c)` - compile everything as a single translation unit. Slower for optimized builds, but may allow some more optimizations
- For any of the above (especially `make c`), you can add extra flags with `f=...` (and linker flags with `lf=...`), e.g.  
  `make f='-O3 -DSOME_MACRO=whatever -some_other_cc_flag' c`  
  Macros that you may want to define are listed in `src/h.h`.  
- A specific build type can be cleaned by adding `clean=1` to the make argument list. Similarly, adding `builddir=1` will give you the build directory.
- Tests can be run with `./BQN path/to/mlochbaum/BQN/test/this.bqn` (add `-noerr` if using `make heapverify`).
- Test precompiled expression: `some-other-bqn-impl ./precompiled.bqn path/to/mlochbaum/BQN "$PATH" '2+2'`
- [Some implementation docs](https://github.com/dzaima/CBQN/tree/master/src#readme)

## License

Any file without an explicit copyright message is copyright (c) 2021 dzaima, GNU GPLv3 - see LICENSE