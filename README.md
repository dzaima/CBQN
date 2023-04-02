# A [BQN](https://github.com/mlochbaum/BQN) implementation in C

[CBQN-specific documentation](docs/README.md) • [source code overview](src/README.md)

## Running

1. `make REPLXX=1`
    - Third-party packages and other ways to run BQN are listed [here](https://mlochbaum.github.io/BQN/running.html)
    - Add `CC=cc` if `clang` isn't installed
    - Add `FFI=0` if your system doesn't have libffi (if `pkg-config` doesn't exist, extra configuration may be necessary to allow CBQN to find libffi)
    - Use `gmake` on BSD (a `NO_LDL=1` make arg may be useful if the linker complains about `-ldl`)
    - Remove `REPLXX=1` if it causes issues (will remove line editing/coloring/name completion in the REPL)
    - Run `sudo make install` afterwards to install into `/usr/local/bin/bqn` (a `PREFIX=/some/path` argument will install to `/some/path/bin/bqn`); `sudo make uninstall` to uninstall
    - `make clean` if anything breaks and you want a clean build slate
2. `./BQN somefile.bqn` to execute a file, or `./BQN` for a REPL (or `rlwrap ./BQN` if replxx wasn't enabled)

## Configuration options

- Builds with more performance:
    - `make o3n-singeli` - native for the current processor (i.e. `-march=native`); on x86-64 this assumes & uses AVX2 (and, if available, also uses BMI2)
    - `make o3-singeli` - generic build for the current architecture; on x86-64 or unknown architectures this doesn't do much, but on aarch64 it uses NEON  
      Therefore, on x86-64, `o3n-singeli` is highly recommended, but, on aarch64, `o3-singeli` is enough
    - `make o3-singeli has=avx2` - generic build for any x86-64 CPU that supports AVX2 (won't utilize BMI2 though; `has='avx2 bmi2'` to assume both AVX2 & BMI2)
    - `make o3n-singeli has=slow-pdep` - build tuned for AMD Zen 1/Zen 2 CPUs, which have BMI2, but their pdep/pext instructions are extremely slow
    - Target architecture is decided from `uname` - override with `target_arch=...` (valid values being `x86-64`, `aarch64`, `generic`). For native builds, targeted extensions are decided by `/proc/cpuinfo` (or `sysctl machdep.cpu` on macOS), and C macro checks.

- Build flags:
    - `CC=cc` - choose a different C compiler (default is `clang`)
    - `CXX=c++` - choose a different C++ compiler (default is `c++`)
    - `f=...` - add extra C compiler flags for CBQN file compilation
    - `lf=...` - add extra linker flags
    - `CCFLAGS=...` - add flags for all CC/CXX/linker invocations
    - `REPLXX=0`/`REPLXX=1` - enable/disable replxx (default depends on target and may change in the future)
    - `REPLXX_FLAGS=...` - override replxx build flags (default is `-std=c++11 -Os`)
    - `FFI=0` - disable libffi usage
    - `j=8` to override the default parallel job count
    - `OUTPUT=path/to/somewhere` - change output location; for `emcc-o3` it will be the destination folder of `BQN.js` and `BQN.wasm`, for everything else - the filename
    - For build targets which use build.bqn (currently all the `-singeli` & `shared-` ones), `target_arch` and `target_os` options can be added, specifying the target architecture/OS (mostly just changing library loading defaults/Singeli configuration; if cross-compiling, further manual configuration will be necessary)
    - Alternatively, `build/build` (aka build.bqn) can be invoked manually, though note that it has slightly different argument naming (see `build/build --help`), doesn't have predefined build types (i.e. `make o3ng-singeli` is `build/build replxx singeli native g`), and may be slightly less stable

- More build types:
    - `make o3` - `-O3`; currently the default build
    - `make shared-o3` - produce a shared library `libcbqn.so`/`libcbqn.dylib`/`cbqn.dll`
    - `make o3g` - `-O3 -g`
    - `make o3g-singeli` / `make o3ng-singeli` - Singeli builds with `-g`
    - `make debug` - unoptimized build with extra assertion checks (also includes `-g`)
    - `make debug1` - debug build without parallel compilation. Useful if everything errors, and you don't want error messages from multiple threads to be printed at the same time.
    - `make c` - a build with minimal default settings, for manual customizing
    - `make shared-c` - like `make c` but for a shared library
    - `make single-(o3|o3g|debug|c)` - compile everything as a single translation unit. Won't have incremental/parallel compilation, and isn't supported for many configurations
    - `make emcc-o3` - build with Emscripten `emcc`
    - `make wasi-o3` - build targeting WASI
- Git submodules are used for Singeli, replxx, and bytecode. It's possible to override those by linking/copying a local version to, respectively, `build/singeliLocal`, `build/replxxLocal`, and `build/bytecodeLocal`.

### Precompiled bytecode

CBQN uses [the self-hosted BQN compiler](https://github.com/mlochbaum/BQN/blob/master/src/c.bqn) & some parts of [the runtime](https://github.com/mlochbaum/BQN/blob/master/src/r1.bqn), and therefore needs to be bootstrapped. By default, the CBQN will use [precompiled bytecode](https://github.com/dzaima/cbqnBytecode).

In order to build everything from source, you can:

#### option 1: use another BQN implementation

  1. [dzaima/BQN](https://github.com/dzaima/BQN) is a BQN implementation that is completely implemented in Java (clone it & run `./build`)
  2. clone [mlochbaum/BQN](https://github.com/mlochbaum/BQN)
  3. `mkdir -p build/bytecodeLocal/gen`
  4. `other-bqn-impl ./build/genRuntime path/to/mlochbaum/BQN build/bytecodeLocal`  
     In the case of the Java impl, `java -jar path/to/dzaima/BQN/BQN.jar ./build/genRuntime path/to/mlochbaum/BQN build/bytecodeLocal`

#### option 2: use the bootstrap compilers

  1. clone [mlochbaum/BQN](https://github.com/mlochbaum/BQN)
  2. `mkdir -p build/bytecodeLocal/gen && make for-bootstrap && ./BQN build/bootstrap.bqn path/to/mlochbaum/BQN`

Note that, after either of those, the compiled bytecode may become desynchronized if you later update CBQN without also rebuilding the bytecode. Usage of the submodule can be restored by removing `build/bytecodeLocal`.

## Requirements

CBQN requires either gcc or clang as the C compiler (it defaults to `clang` as optimizations are written based on whether or not clang needs them, but a `CC=cc` make arg can be added to use the default system compiler), and, optionally, libffi for `•FFI`, and C++ (requires ≥C++11; defaults to `c++`, override with `CXX=your-c++`) for replxx.

There aren't hard requirements for versions of any of those, but nevertheless here are some configurations that CBQN is tested on by dzaima:

```
x86-64 (Linux):
  gcc 9.5; gcc 11.3; clang 10.0.0; clang 14.0.0
  libffi 3.4.2
  cpu microarchitecture: Haswell
  replxx: g++ 11.3.0
x86 (Linux):
  clang 14.0.0; known to break on gcc - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58416
  running on the above x86-64 system, compiled with CCFLAGS=-m32
AArch64 ARMv8-A (within Termux on Android 8):
  using a `lf=-landroid-spawn` make arg after `pkg install libandroid-spawn` to get •SH to work
  clang 15.0.7
  libffi 3.4.4 (structs were broken as of 3.4.3)
  replxx: clang++ 15.0.4
```
Additionally, CBQN is known to compile as-is on macOS, but Windows builds need [WinBQN](https://github.com/actalley/WinBQN) to set up an appropriate Windows build environment, or be built from Linux by cross-compilation.

## License

Most files here are copyright (c) 2021-2023 dzaima & others, [GNU GPLv3 only](licenses/LICENSE-GPLv3).
Exceptions are:
- timsort implementation - `src/builtins/sortTemplate.h`: [MIT](licenses/LICENSE-MIT-sort); [original repo](https://github.com/swenson/sort/tree/f79f2a525d03f102034b5a197c395f046eb82708)
- Ryu - `src/utils/ryu.c` & files in `src/utils/ryu/`: [Apache 2.0](licenses/LICENSE-Apache2) or [Boost 1.0](licenses/LICENSE-Boost); [original repo](https://github.com/ulfjack/ryu/tree/75d5a85440ed356ad7b23e9e6002d71f62a6255c)