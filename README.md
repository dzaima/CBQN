# A [BQN](https://mlochbaum.github.io/BQN/) implementation in C

[CBQN-specific documentation](docs/README.md) • [source code overview](src/README.md)

## Running

1. `make`
    - Third-party packages and other ways to run BQN are listed [here](https://mlochbaum.github.io/BQN/running.html)
    - Add `CC=cc` if `clang` isn't installed
    - Add `FFI=0` if your system doesn't have libffi
    - Use `gmake` on BSD
    - Add `REPLXX=0` if C++ is unavailable (will remove line editing/coloring/name completion in the REPL)
    - Run `sudo make install` afterwards to install into `/usr/local/bin/bqn` (a `PREFIX=/some/path` argument will install to `/some/path/bin/bqn`); `sudo make uninstall` to uninstall
    - `make clean` if anything breaks and you want a clean build slate
2. `./BQN somefile.bqn` to execute a file, or `./BQN` for a REPL

## Configuration options

The default configuration enables REPLXX & Singeli, and, if not done before, implicitly runs `make for-build` to build a CBQN for running `build/src/build.bqn` and compiling Singeli.

### Builds with more performance

(TL;DR: use `make o3n` for local builds on x86-64, but `make` is fine on other architectures)

The default target (`make o3`) will target optimizations for the current architecture, but not any further extensions the specific CPU may have.

Thus, performance can be significantly improved by targeting the specific CPU via `make o3n` (with the usual drawback of `-march=native` of it not producing a binary portable to other CPUs of the same architecture).

On x86-64, a native build will enable usage of AVX2 (i.e. ability to use 256-bit SIMD vectors instead of 128-bit ones, among other things), and BMI2 if available. But, on aarch64, NEON is always available, so a native build won't give significant benefits.

To produce a binary utilizing AVX2 not specific to any processor, it's possible to do `make o3 has=avx2`. (`has='avx2 bmi2'` for targeting both AVX2 & BMI2)

Additionally, on AMD Zen 1 & Zen 2, `make o3n has=slow-pdep` will further improve certain builtins (Zen 1/2 support BMI2, but their implementation of `pdep`/`pext` is so slow that not using it for certain operations is very beneficial).

CBQN currently does not utilize AVX-512 or SVE, or have any SIMD optimizations specific to any architectures other than x86-64 and aarch64.

For native builds, targeted extensions are determined by `/proc/cpuinfo` (or `sysctl machdep.cpu` on macOS) and C macros defined as a result of `-march=native`.

### Build flags

`CC=...` - choose a different C compiler (default is `clang`)  
`CXX=...` - choose a different C++ compiler; needed only for REPLXX (default is `c++`)  
`OUTPUT=path/to/somewhere` - change output location; for `emcc-o3` it will be the destination folder for `BQN.js` and `BQN.wasm`, for everything else - the filename  
`target_arch=(x86-64|aarch64|generic)` - target architecture. Inferred from `uname` by default. Used for deciding target optimizations.  
`target_os=(linux|bsd|macos|windows)` - target OS. Inferred from `uname` by default. Used for determining default output names and slight configuration changes.  
`j=8` - override the default parallel job count (default is the output of `nproc`)  
`notui=1` - display build progress in a plain-text format  
`version=...` - specify the version to report in `--version` (default is commit hash)

`REPLXX=0` - disable REPLXX
`singeli=0` - disable usage of Singeli  
`FFI=0` - disable `•FFI`, thus not depending on libffi

`f=...` - add extra C compiler flags for CBQN file compilation  
`lf=...` - add extra linking flags (`LDFLAGS` is a synonym)  
`CCFLAGS=...` - add flags for all CC/CXX/linking invocations  
`REPLXX_FLAGS=...` - override replxx build flags (default is `-std=c++11 -Os`)  
`CXXFLAGS=...` - add additional CXX flags

Alternatively, `build/build` (aka build.bqn) can be invoked manually, though note that it has slightly different argument naming (see `build/build --help`) and doesn't have predefined build types (i.e. `make o3ng` is done as `build/build replxx singeli native g`)

### More build types

- `make o3` - the default build
- `make o3g` - effectively `make o3 f=-g` (custom `f=...` can still be added on)
- `make c` - `make o3` but without `-O3`
- `make shared-o3` - produce a shared library `libcbqn.so`/`libcbqn.dylib`/`cbqn.dll`
- `make shared-c` - like `make c` but for a shared library
- `make emcc-o3` - build with Emscripten `emcc`
- `make wasi-o3` - build targeting WASI
- `make wasi-reactor-o3` - build producing a WASI Reactor
- `make debug` - unoptimized build with extra assertion checks (also includes `-g`)
- `make static-bin` - build a statically linked executable (for a fully standalone binary, try `make static-bin CC=musl-gcc REPLXX=0`)

All of the above will go through build.bqn. If that causes problems, `make o3-makeonly` or `make c-makeonly` can be used. These still enable REPLXX by default, but do not support Singeli. Furthermore, these targets don't support some of the build flags that the others do.

## Requirements

CBQN requires either gcc or clang as the C compiler (it defaults to `clang` as things are primarily optimized for it, but a `CC=cc` make arg can be added to use the default system compiler), and, optionally, libffi for `•FFI`, and C++ (requires ≥C++11; defaults to `c++`, override with `CXX=your-c++`) for replxx.

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

The build will additionally attempt to use `pkg-config` for determining how to include libffi, `uname` for `target_arch` and `target_os`, and `nproc` for parallel job count, but has defaults if any aren't present (`-lffi` for linking libffi (+ `-ldl` on non-BSD), arch → `generic`, os → `linux`, `j=4`), and the behavior of these can be overriden by build options.

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

### Submodules

Git submodules are used for Singeli, replxx, and bytecode. Thus, CBQN won't build if downloaded just as source files.

Thus, you must either clone the repo (submodules will be automatically initialized/updated as needed), or use local copies of the submodules by linking/copying local versions to `build/singeliLocal`, `build/replxxLocal`, and `build/bytecodeLocal`.

### Cross-compilation

You must manually set up a cross-compilation environment. It's possible to pass flags to all CC/CXX/linking invocations via `CCFLAGS=...`, and `LDFLAGS=...` to pass ones to the linking step specifically (more configuration options [above](#configuration-options)).

A `target_arch=(x86-64|aarch64|generic)` make argument must be present (`generic` will work always, but a more specific argument will enable significant optimizations), as the default is to choose based on `uname`.

Furthermore, all build targets (except `-makeonly` ones) will need a non-cross-compiled version of CBQN at build time to run build.bqn and (if enabled) Singeli. For those, a `make for-build` will need to be ran before the primary build, configured to not cross-compile. (this step only needs a C compiler (default is `CC=cc` here), and doesn't need libffi, nor a C++ compiler).

## License

Most files here are copyright (c) 2021-2023 dzaima & others, [GNU GPLv3 only](licenses/LICENSE-GPLv3).
Exceptions are:
- timsort implementation - `src/builtins/sortTemplate.h`: [MIT](licenses/LICENSE-MIT-sort); [original repo](https://github.com/swenson/sort/tree/f79f2a525d03f102034b5a197c395f046eb82708)
- Ryu - `src/utils/ryu.c` & files in `src/utils/ryu/`: [Apache 2.0](licenses/LICENSE-Apache2) or [Boost 1.0](licenses/LICENSE-Boost); [original repo](https://github.com/ulfjack/ryu/tree/75d5a85440ed356ad7b23e9e6002d71f62a6255c)