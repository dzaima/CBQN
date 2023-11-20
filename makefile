SHELL = /usr/bin/env bash
MAKEFLAGS+= --no-print-directory --no-builtin-rules --no-builtin-variables

default: o3

# targets that go to build.bqn
o3:
	@"${MAKE}" to-bqn-build
o3g:
	@"${MAKE}" to-bqn-build i_build_opts="g"
o3n:
	@"${MAKE}" to-bqn-build i_build_opts="native"
o3ng:
	@"${MAKE}" to-bqn-build i_build_opts="native g"
c:
	@"${MAKE}" to-bqn-build build_c=1

debug:
	@"${MAKE}" to-bqn-build build_c=1 i_build_opts="debug"
heapverify:
	@"${MAKE}" to-bqn-build build_c=1 i_build_opts="debug heapverify"
rtverify:
	@"${MAKE}" to-bqn-build i_build_opts="debug rtverify"

debugn:
	@"${MAKE}" to-bqn-build build_c=1 i_build_opts="native debug"
heapverifyn:
	@"${MAKE}" to-bqn-build build_c=1 i_build_opts="native debug heapverify"
rtverifyn:
	@"${MAKE}" to-bqn-build i_build_opts="native rtverify"

wasi-o3:
	@"${MAKE}" to-bqn-build REPLXX=0 i_build_opts="wasi"
wasi-reactor-o3:
	@"${MAKE}" to-bqn-build REPLXX=0 i_build_opts="wasi" i_SHARED=1
emcc-o3:
	@"${MAKE}" to-bqn-build REPLXX=0 i_build_opts="emcc"
shared-o3:
	@"${MAKE}" to-bqn-build REPLXX=0 i_SHARED=1
shared-c:
	@"${MAKE}" to-bqn-build REPLXX=0 i_SHARED=1 i_build_opts=c
static-lib:
	@"${MAKE}" to-bqn-build REPLXX=0 i_build_opts="static-lib"
static-bin:
	@"${MAKE}" to-bqn-build FFI=0 i_build_opts="static-bin"



# targets that use build/makefile
for-build: # for running the build system & Singeli
	@"${MAKE}" -f build/makefile run_incremental_0 i_t=forbuild i_CC=cc REPLXX=0 i_f="-O2 -DFOR_BUILD" i_FFI=0 i_SHARED=0 i_PIE= i_CC_PIE= i_EXPORT=0 i_OUTPUT_DEF=build/obj2/for_build4
for-bootstrap: # for bootstrapping bytecode
	@"${MAKE}" -f build/makefile run_incremental_0 i_t=for_bootstrap REPLXX=0 i_FFI=0 i_SHARED=0 i_PIE= i_CC_PIE= i_EXPORT=0 i_f='-DNATIVE_COMPILER -DONLY_NATIVE_COMP -DFORMATTER=0 -DNO_RT -DNO_EXPLAIN' i_USE_BC_SUBMODULE=0 BYTECODE_DIR=bytecodeNone
o3-makeonly:
	@"${MAKE}" -f build/makefile run_incremental_0 i_t=o3 i_f="-O3"
c-makeonly:
	@"${MAKE}" -f build/makefile run_incremental_0 custom=1
# simple non-incremental builds
single-o3:
	@"${MAKE}" -f build/makefile single-o3
single-o3g:
	@"${MAKE}" -f build/makefile single-o3g
single-debug:
	@"${MAKE}" -f build/makefile single-debug
single-c:
	@"${MAKE}" -f build/makefile single-c



i_REPLXX_1 = 1
ifeq ($(origin REPLXX),command line)
	i_REPLXX_1 = "$(REPLXX)"
endif
i_singeli_1 = 1
ifeq ($(origin singeli),command line)
	i_singeli_1 = "$(singeli)"
endif

to-bqn-build:
ifeq ($(origin builddir),command line)
	@echo "Error: 'builddir' unsupported"; false
endif
ifeq ($(origin clean),command line)
	@echo "Error: build-specific 'clean' unsupported"; false
endif
	@build/build from-makefile CC="$(CC)" CXX="$(CXX)" PIE="$(ENABLE_PIE)" OUTPUT="$(OUTPUT)" j="$(j)" \
	  verbose="$(verbose)" notui="$(notui)" v="$(version)" \
	  f="$(f)" lf="$(lf)" CCFLAGS="$(CCFLAGS)" LDFLAGS="$(LDFLAGS)" REPLXX_FLAGS="$(REPLXX_FLAGS)" CXXFLAGS="$(CXXFLAGS)" \
	  LD_LIBS="$(LD_LIBS)" NO_LDL="$(NO_LDL)" no_fPIC="$(no_fPIC)" \
	  c="$(build_c)" debug="$(debug)" $(i_build_opts) $(build_opts) \
	  os="$(target_os)" arch="$(target_arch)" has="$(has)" \
	  shared="$(i_SHARED)" singeli="$(i_singeli_1)" replxx="$(i_REPLXX_1)" FFI="$(FFI)"

# mappings of old names
o3-singeli: o3
o3g-singeli: o3g
o3n-singeli: o3n
o3ng-singeli: o3ng
debugn-singeli: debugn
heapverifyn-singeli: heapverifyn
rtverifyn-singeli: rtverifyn



DESTDIR =
PREFIX = /usr/local
install:
	rm -f "$(DESTDIR)$(PREFIX)/bin/bqn"
	cp -f BQN "$(DESTDIR)$(PREFIX)/bin/bqn"
	
	cp -f include/bqnffi.h "$(DESTDIR)$(PREFIX)/include/bqnffi.h"
	
	@if [ -f libcbqn.so ]; then \
		rm -f "$(DESTDIR)$(PREFIX)/lib/libcbqn.so"; \
		cp -f libcbqn.so "$(DESTDIR)$(PREFIX)/lib/libcbqn.so"; \
		echo 'cp -f libcbqn.so "$(DESTDIR)$(PREFIX)/lib/libcbqn.so"'; \
	else \
		echo "Not installing libcbqn.so as it wasn't built"; \
	fi



uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/bqn"
	rm -f "$(DESTDIR)$(PREFIX)/include/bqnffi.h"
	rm -f "$(DESTDIR)$(PREFIX)/lib/libcbqn.so"

clean-build:
	rm -f build/obj/*/*.o
	rm -f build/obj/*/*.d
	rm -f build/obj/*/BQN
clean-specific:
	rm -f $(bd)/*.o
	rm -f $(bd)/*.d
	rm -f $(bd)/BQN
	rmdir $(bd); true
clean-submodules:
	git submodule deinit build/singeliSubmodule/ build/replxxSubmodule/ build/bytecodeSubmodule/
clean-obj2:
	rm -rf build/obj2

clean: clean-build clean-obj2
