# A [BQN](https://mlochbaum.github.io/BQN/) implementation in C

[CBQN-specific documentation](docs/README.md) • [source code overview](src/README.md)

[![Packaging status](https://repology.org/badge/vertical-allrepos/cbqn.svg)](https://repology.org/project/cbqn/versions)

## Running

1. `make`
    - Third-party packages and other ways to run BQN are listed [here](https://mlochbaum.github.io/BQN/running.html)
    - Add `FFI=0` if your system doesn't have libffi
    - Use `gmake` on BSD
    - Add `REPLXX=0` if C++ is unavailable (will remove line editing/coloring/name completion in the REPL)
    - Run `sudo make install` afterwards to install into `/usr/local/bin/bqn` (a `PREFIX=/some/path` argument will install to `/some/path/bin/bqn`); `sudo make uninstall` to uninstall
    - `make clean` to get to a clean build state
2. `./BQN somefile.bqn` to execute a file, or `./BQN` for a REPL

## Configuration options

The default configuration enables REPLXX & Singeli, and, if not done before, implicitly runs `make for-build` to build a CBQN for running `build/src/build.bqn` and compiling Singeli.

### Builds with more performance

(TL;DR: use `make o3n` for local builds on x86-64, but `make` is fine on other architectures)

The default target (`make o3`) will target optimizations for the current architecture, but not any further extensions the specific CPU may have.

Thus, performance can be significantly improved by targeting the specific CPU via `make o3n` (with the usual drawback of `-march=native` of it not producing a binary portable to other CPUs of the same architecture).

On x86-64, a native build will, if available, enable usage of AVX2 (i.e. ability to use 256-bit SIMD vectors instead of 128-bit ones, among other things), and BMI2. But, on aarch64, NEON is always available, so a native build won't give significant benefits.

To produce a binary utilizing AVX2 not specific to any processor, it's possible to do `make has=avx2`. (`has=avx2,bmi2` for targeting both AVX2 & BMI2)

Additionally, on AMD Zen 1 & Zen 2, `make o3n has=slow-pdep` will further improve certain builtins (Zen 1/2 support BMI2, but their implementation of `pdep`/`pext` is so slow that not using it for certain operations is very beneficial).

CBQN currently does not utilize AVX-512 or SVE, or have any SIMD optimizations specific to any architectures other than x86-64 and aarch64.

For native builds, targeted extensions are determined by `/proc/cpuinfo` (or `sysctl machdep.cpu` on macOS) and C macros defined as a result of `-march=native`.

### Build flags

- `notui=1` - display build progress in a plain-text format
- `version=...` - specify the version to report in `--version` (default is commit hash)
- `nogit=1` - error if something attempts to use `git`
- `CC=...` - choose a different C compiler (default is `clang`, or `cc` if unavailable; CBQN is more tuned for clang, but gcc also works)
- `CXX=...` - choose a different C++ compiler; needed only for REPLXX (default is `c++`)
- `j=8` - override the default parallel job count (default is the output of `nproc`)
- `OUTPUT=path/to/somewhere` - change output location; for `emcc-o3` it will be the destination folder for `BQN.js` and `BQN.wasm`, for everything else - the filename
- `target_arch=(x86-64|aarch64|generic)` - target architecture; if abscent, inferred from `uname`, or `CC` if `target_from_cc=1`; used for enabling architecture-specific optimizations
- `target_os=(linux|bsd|macos|windows)` - target OS; if abscent, inferred from `uname`, or `CC` if `target_from_cc=1`; used for determining default output names and slight configuration changes
- `target_from_cc=1` - infer the target architecture and OS from C macros that `CC` defines via `-dM -E`; additionally infers available extensions, allowing e.g. `make f=-march=x86-64-v3 - target_from_cc=1` to optimize assuming AVX2, which would otherwise need `has=avx2`
- `has=...` - assume specified architecture extensions/properties (x86-64-only); takes a comma-separated list which, beyond what is architecturally guaranteed, infer additional extensions as noted which hold on existing hardware (at least as of the time of writing):
  - `pclmul` (implies SSE4.2)
  - `avx2` (implies `pclmul`, POPCNT, BMI1)
  - `bmi2` (implies `avx2`)
  - `slow-pdep` (implies `bmi2`; specifies Zen 1 & Zen 2's slow `pdep`/`pext`)
<!-- separator -->
- `REPLXX=0` - disable REPLXX
- `singeli=0` - disable usage of Singeli
- `FFI=0` - disable `•FFI`, thus not depending on libffi
- `usz=32` - use 32-bit integers for array lengths (default is 64-bit)
<!-- separator -->
- `f=...` - add extra C compiler flags for CBQN file compilation
- `lf=...` - add extra linking flags (`LDFLAGS` is a synonym)
- `CCFLAGS=...` - add flags for all CC/CXX/linking invocations
- `REPLXX_FLAGS=...` - override replxx build flags (default is `-std=c++11 -Os`)
- `CXXFLAGS=...` - add additional CXX flags

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
- `make static-lib` - build a static library archive

All of the above will go through build.bqn. If that causes problems, `make o3-makeonly` or `make c-makeonly` can be used. These still enable REPLXX by default, but do not support Singeli. Furthermore, these targets don't support some of the build flags that the others do.

## Limitations

- The current default is to use unsigned 32-bit integers for array lengths, limiting array size to about 2^32 elements. This can be switched to 64-bit via `usz=64`, but will have limited effect as some builtins don't have implementations for indices larger than 32-bit (search functions, `⊔`, `/𝕩`, `/⁼𝕩` & similar). Additionally, some things may fail even before 2^31 and may even crash CBQN.

- Throwing & catching errors will leak memory: the garbage collector has no way to scan for objects on the C stack, and checks for objects with an incorrect reference count (comparing to the expected from other heap objects / GC roots) to estimate that. Error catching is implemented as a `longjmp`, thus making the GC permanently think those are still on the C stack. Between REPL lines, a full GC can run (as then there's a guarantee of no BQN code being mid-execution), but that doesn't apply to any other context.

- Freeing highly nested objects will crash: object freeing is done recursively, and there is nothing preventing arbitrarily-nested objects.

## Requirements


CBQN requires either gcc or clang as the C compiler (by default it attempts `clang` as things are primarily optimized for clang, but, if unavailable, it'll fall back to `cc`; override with `CC=your-cc`), and, optionally, libffi for `•FFI` and C++ (requires ≥C++11; defaults to `c++`, override with `CXX=your-c++`) for replxx.

There aren't hard requirements for versions of any of those, but nevertheless here are some configurations that CBQN is tested on by dzaima:

```
x86-64 (Linux):
  gcc 9.5; gcc 14.0.1; clang 10.0.0; clang 19.1.0
  libffi 3.4.6
  cpu microarchitecture: Haswell
  replxx: g++ 14.0.1; clang++ 19.1.0
x86 (Linux):
  clang 19.1.0; CBQN is known to break on gcc x86 - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58416
  running on the above x86-64 system, compiled with CCFLAGS=-m32
AArch64 ARMv8-A (within Termux on Android 8):
  using `lf=-landroid-spawn` from `pkg install libandroid-spawn` to get •SH to work
  clang 18.1.8
  libffi 3.4.6 (structs were broken as of 3.4.3)
  replxx: clang++ 18.1.8
```
Additionally, CBQN is known to compile as-is on macOS. Windows builds can be made by cross-compilation ([Docker setup](https://github.com/vylsaz/cbqn-win-docker-build)).

The build will attempt to use `pkg-config` to find libffi, `uname` to determine `target_arch` & `target_os` if not using `target_from_cc`, and `nproc` for parallel job count, with defaults if unavailable (`-lffi` for linking libffi (+ `-ldl` on non-BSD), `target_arch=generic`, `target_os=linux`, `j=4`; these can of course also be specified manually).

Git submodules are used for Singeli, replxx, and precompiled bytecode. To avoid automatic usage of `git` here, link local copies to `build/singeliLocal`, `build/replxxLocal`, and `build/bytecodeLocal`.

Furthermore, `git` is used to determine the version that `--version` should display (override with `version=...`). Use `nogit=1` to disallow automatic `git` usage.

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

### Cross-compilation

You must manually set up a cross-compilation environment. It's possible to pass flags to all CC/CXX/linking invocations via `CCFLAGS=...`, and `LDFLAGS=...` to pass ones to the linking step specifically (more configuration options [above](#build-flags)).

A `target_arch=(x86-64|aarch64|generic)` make argument should be added (`generic` will work always, but a more specific argument will enable significant optimizations), as otherwise it'll choose based on `uname`. Similarly, `target_os=(linux|bsd|macos|windows)` should be present if the target OS differs from the host.

Alternatively, the `target_from_cc=1` make argument can be used, replacing the need of `target_arch` and `target_os` (although they can be still set, overriding the values inferred from CC).

Furthermore, all build targets (except `-makeonly` ones) will need a non-cross-compiled version of CBQN at build time to run build.bqn and Singeli. For those, a `make for-build` will need to be ran before the primary build, configured to not cross-compile. (this step only needs a C compiler (default is `CC=cc` here), and doesn't need libffi, nor a C++ compiler).

## Licensing

First, some exceptions to the general licensing:
- timsort implementation - `src/builtins/sortTemplate.h`: [MIT](licenses/LICENSE-MIT-sort); [original repo](https://github.com/swenson/sort/tree/f79f2a525d03f102034b5a197c395f046eb82708)
- Ryu - `src/utils/ryu.c` & files in `src/utils/ryu/`: [Apache 2.0](licenses/LICENSE-Apache2) or [Boost 1.0](licenses/LICENSE-Boost); [original repo](https://github.com/ulfjack/ryu/tree/75d5a85440ed356ad7b23e9e6002d71f62a6255c)


Additionally, REPLXX (optional, included by default) has its own licensing at `build/replxxSubmodule/LICENSE.md`.

Everything else (i.e. all files except `src/builtins/sortTemplate.h`, `src/utils/ryu.c`, and everything in `src/utils/ryu/`) may be treated as under any of the following licenses, as if they had the respective notice:

- [GNU LGPLv3 only](licenses/LICENSE-LGPLv3)
```
Copyright (C) 2021-2023 dzaima and contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
- [GNU GPLv3 only](licenses/LICENSE-GPLv3)
```
Copyright (C) 2021-2023 dzaima and contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
- [MPL 2.0](licenses/LICENSE-MPL2)
```
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
```
