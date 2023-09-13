SHELL = /usr/bin/env bash
MAKEFLAGS+= --no-print-directory --no-builtin-rules --no-builtin-variables
# note: do not manually define any i_… arguments, or incremental compiling will not work properly!

default: o3

# targets that only use the makefile
for-build: # for running the build system & Singeli
	@"${MAKE}" i_t=forbuild i_CC=cc REPLXX=0 i_f="-O2 -DFOR_BUILD" i_FFI=0 i_SHARED=0 i_PIE= i_CC_PIE= i_EXPORT=0 i_OUTPUT_DEF=build/obj2/for_build4 run_incremental_0
for-bootstrap: # for bootstrapping bytecode
	@"${MAKE}" i_t=for_bootstrap REPLXX=0 i_FFI=0 i_SHARED=0 i_PIE= i_CC_PIE= i_EXPORT=0 i_f='-DNATIVE_COMPILER -DONLY_NATIVE_COMP -DFORMATTER=0 -DNO_RT -DNO_EXPLAIN' run_incremental_0 i_USE_BC_SUBMODULE=0 BYTECODE_DIR=bytecodeNone
o3-makeonly:
	@"${MAKE}" i_t=o3 i_f="-O3" run_incremental_0
c-makeonly:
	@"${MAKE}" custom=1 run_incremental_0

i_notui=0
ifeq ($(origin notui),command line)
	i_notui=1
endif
ifeq ($(origin REPLXX),command line)
	i_REPLXX_1 = "$(REPLXX)"
else
	i_REPLXX_1 = 1
endif
ifeq ($(origin singeli),command line)
	i_singeli_1 = "$(singeli)"
else
	i_singeli_1 = 1
endif

to-bqn-build:
ifeq ($(origin builddir),command line)
	@echo "Error: 'builddir' unsupported"; false
endif
ifeq ($(origin clean),command line)
	@echo "Error: build-specific 'clean' unsupported"; false
endif
	@build/build from-makefile CC="$(CC)" CXX="$(CXX)" PIE="$(ENABLE_PIE)" OUTPUT="$(OUTPUT)" j="$(j)" \
	  verbose="$(verbose)" notui="$(i_notui)" v="$(version)" \
	  f="$(f)" lf="$(lf)" CCFLAGS="$(CCFLAGS)" LDFLAGS="$(LDFLAGS)" REPLXX_FLAGS="$(REPLXX_FLAGS)" CXXFLAGS="$(CXXFLAGS)" \
	  LD_LIBS="$(LD_LIBS)" NO_LDL="$(NO_LDL)" no_fPIC="$(no_fPIC)" \
	  c="$(build_c)" debug="$(debug)" $(i_build_opts) $(build_opts) \
	  os="$(target_os)" arch="$(target_arch)" has="$(has)" \
	  shared="$(i_SHARED)" singeli="$(i_singeli_1)" replxx="$(i_REPLXX_1)" FFI="$(FFI)"

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
static-o3:
	@"${MAKE}" to-bqn-build REPLXX=0 i_build_opts="static-lib"
static-bin:
	@"${MAKE}" to-bqn-build FFI=0 i_build_opts="static-bin"

# mappings of old names
o3-singeli: o3
o3g-singeli: o3g
o3n-singeli: o3n
o3ng-singeli: o3ng
debugn-singeli: debugn
heapverifyn-singeli: heapverifyn
rtverifyn-singeli: rtverifyn

# compiler setup
i_CC := cc
i_PIE :=
i_LIBS_LD := -lm
i_LIBS_CC := -fvisibility=hidden
i_FFI := 2
i_OUTPUT := BQN

ifeq ($(origin i_OUTPUT_DEF),command line)
	i_OUTPUT := $(i_OUTPUT_DEF)
endif
ifeq ($(origin OUTPUT),command line)
	i_OUTPUT := $(OUTPUT)
endif
ifeq ($(i_emcc),1)
	i_OUTPUT_FOLDER := $(i_OUTPUT)
	i_OUTPUT_BIN := $(i_OUTPUT_FOLDER)/BQN.js
else
	i_OUTPUT_BIN := $(i_OUTPUT)
endif
ifeq ($(origin CC),command line)
	override i_CC := $(CC)
	custom = 1
endif
ifeq ($(origin FFI),command line)
	i_FFI := $(FFI)
	custom = 1
endif
ifeq ($(i_SHARED),1)
	i_PIE := -shared
	i_EXPORT := 1
	i_LIBS_CC += -DCBQN_SHARED
	ifneq ($(no_fPIC),1)
		i_LIBS_CC += -fPIC
	endif
endif
ifneq ($(i_FFI),0)
	i_EXPORT := 1
	ifeq ($(shell command -v pkg-config 2>&1 > /dev/null && pkg-config --exists libffi && echo $$?),0)
		i_LIBS_LD += $(shell pkg-config --libs libffi)
		i_LIBS_CC += $(shell pkg-config --cflags libffi)
	else
		i_LIBS_LD += -lffi
	endif
	ifneq ($(NO_LDL),1)
		i_LIBS_LD += -ldl
	endif
endif
ifeq ($(i_EXPORT),1)
	i_LIBS_LD += -rdynamic
	i_LIBS_CC += -DCBQN_EXPORT
endif
ifeq ($(origin LD_LIBS),command line)
	i_LIBS_LD := $(LD_LIBS)
	custom = 1
endif
ifeq ($(i_REPLXX_1),1)
	i_PIE += -fPIE
	i_CC_PIE := -fPIE
endif
ifeq ($(origin PIE),command line)
	override i_PIE := $(PIE)
	custom = 1
endif
ifeq ($(origin f),command line)
	custom = 1
else
	f:=
endif
ifeq ($(origin lf),command line)
	custom = 1
else
	lf:=
endif
ifeq ($(origin CCFLAGS),command line)
	custom = 1
endif
ifeq ($(origin LDFLAGS),command line)
	custom = 1
endif
ifeq ($(i_REPLXX_1),1)
	i_f+= -DREPLXX_STATIC=1
	custom = 1
	REPLXX_DIR = $(shell if [ -d build/replxxLocal ]; then echo build/replxxLocal; else echo build/replxxSubmodule; fi)
endif
BYTECODE_DIR = $(shell if [ -d build/bytecodeLocal ]; then echo bytecodeLocal; else echo bytecodeSubmodule; fi)
ifeq ($(BYTECODE_DIR),bytecodeLocal)
	custom = 1
endif
ifeq ($(BYTECODE_DIR),bytecodeSubmodule)
ifeq ($(i_USE_BC_SUBMODULE),)
	i_USE_BC_SUBMODULE=1
endif
endif

i_LD = $(i_CC)

CC_IS_CLANG = $(shell $(i_CC) --version | head -n1 | grep -m 1 -c "clang")
ifeq (${CC_IS_CLANG}, 1)
	NOWARN = -Wno-microsoft-anon-tag -Wno-bitwise-instead-of-logical -Wno-unknown-warning-option
else
	NOWARN = -Wno-parentheses
endif

ifeq ($(WINDOWS), 1)
	i_f+= -DNO_MMAP
	i_lf+= -lpthread
	ifeq ($(i_REPLXX_1), 1)
		i_f+= -DUSE_REPLXX_IO
	endif
endif

ALL_CC_FLAGS = -std=gnu11 -Wall -Wno-unused-function -fms-extensions -ffp-contract=off -fno-math-errno -fno-strict-aliasing $(CCFLAGS) $(f) $(i_f) $(NOWARN) -DBYTECODE_DIR=$(BYTECODE_DIR) -DFFI=$(i_FFI) $(i_LIBS_CC) $(i_CC_PIE)
ALL_LD_FLAGS = $(LDFLAGS) $(lf) $(i_lf) $(i_PIE) $(i_LIBS_LD)

j=4
ifneq (${manualJobs},1)
	ifeq (${MAKECMDGOALS},run_incremental_1)
		MAKEFLAGS+= -j${j} manualJobs=1
	endif
endif

builddir:
ifeq ($(force_build_dir),)
	@printf 'build/obj/'
ifeq ($(custom),)
	@echo "def_$(i_t)"
else
	@[ -x "$$(command -v sha256sum)" ] && hashInput="sha256sum"; \
	[  -x "$$(command -v shasum)" ] && hashInput="shasum -a 256"; \
	printf "%s\0%s\0%s\0%s\0%s\0%s\0%s\0%s\0%s" "${i_CC}" "${ALL_CC_FLAGS}" "${ALL_LD_FLAGS}" "${i_REPLXX_1}" "${REPLXX_FLAGS}" "${CXXFLAGS}" "${i_CXX}" "${BYTECODE_DIR}" "${REPLXX_DIR}" | $$hashInput | grep -oE '[0-9a-z]{64}' | head -c32
endif
else
	@printf "%s" "$(force_build_dir)"
endif


# simple non-incremental builds
single-o3:
	$(i_CC) $(ALL_CC_FLAGS) -DSINGLE_BUILD -O3 -o ${i_OUTPUT_BIN} src/opt/single.c $(ALL_LD_FLAGS)
single-o3g:
	$(i_CC) $(ALL_CC_FLAGS) -DSINGLE_BUILD -O3 -g -o ${i_OUTPUT_BIN} src/opt/single.c $(ALL_LD_FLAGS)
single-debug:
	$(i_CC) $(ALL_CC_FLAGS) -DSINGLE_BUILD -DDEBUG -g -o ${i_OUTPUT_BIN} src/opt/single.c $(ALL_LD_FLAGS)
single-c:
	$(i_CC) $(ALL_CC_FLAGS) -DSINGLE_BUILD -o ${i_OUTPUT_BIN} src/opt/single.c $(ALL_LD_FLAGS)

# actual build
run_incremental_0:
ifeq ($(i_t),forbuild)
	@mkdir -p build/obj2
endif
ifeq ($(verbose),1)
	@echo "build directory: $$("${MAKE}" builddir)"
	@echo "  bytecode: build/$(BYTECODE_DIR)"
ifeq ($(i_REPLXX_1),1)
	@echo "  replxx: $(REPLXX_DIR)"
else
	@echo "  replxx: not used"
endif
	@echo "  cc invocation: $(CC_INC) \$$@.d -o \$$@ -c \$$<"
	@echo "  ld invocation: $(i_LD) ${CCFLAGS} -o [build_dir]/BQN [build_dir]/*.o $(ALL_LD_FLAGS)"
endif
ifeq ($(singeli),1)
	@echo "makefile-only builds don't support Singeli"; false
endif
	
ifeq ($(origin clean),command line)
	@"${MAKE}" clean-specific bd="$$("${MAKE}" builddir)"
else ifeq ($(origin builddir),command line)
	@echo "$$("${MAKE}" builddir)"
else # run build
	
ifeq ($(REPLXX_DIR),build/replxxSubmodule)
	@git submodule update --init build/replxxSubmodule || (echo 'Failed to initialize submodule; clone CBQN as a git repo, or place local copies in build/ (see README.md#submodules).' && false)
endif
ifeq ($(i_USE_BC_SUBMODULE),1)
	@git submodule update --init build/bytecodeSubmodule || (echo 'Failed to initialize submodule; clone CBQN as a git repo, or place local copies in build/ (see README.md#submodules).' && false)
endif
	@export bd=$$("${MAKE}" builddir); \
	[ "build/obj/" = "$$bd" ] && echo "Neither shasum nor sha256sum was found; cannot use custom configurations" && exit 1; \
	mkdir -p "$$bd";                   \
	"${MAKE}" run_incremental_1 bd="$$bd"
endif # run build

run_incremental_1: ${bd}/BQN
ifneq (${bd}/BQN,${i_OUTPUT_BIN})
ifeq ($(i_emcc),1)
	@cp -f ${bd}/BQN.wasm "${i_OUTPUT_FOLDER}/BQN.wasm"
endif
ifeq ($(WINDOWS),1)
	@cp -f ${bd}/BQN.exe "${i_OUTPUT_BIN}"
else
	@cp -f ${bd}/BQN "${i_OUTPUT_BIN}"
endif
endif
	@echo ${postmsg}

${bd}/BQN: builtins core base jit utils # build the final binary
	@$(i_LD) ${CCFLAGS} -o ${bd}/BQN ${bd}/*.o $(ALL_LD_FLAGS)

CC_INC = $(i_CC) $(ALL_CC_FLAGS) -MMD -MP -MF
# build individual object files
core: ${addprefix ${bd}/, tyarr.o harr.o fillarr.o stuff.o derv.o mm.o heap.o}
${bd}/%.o: src/core/%.c
	@echo $< | cut -c 5-
	@$(CC_INC) $@.d -o $@ -c $<

base: ${addprefix ${bd}/, load.o main.o rtwrap.o vm.o ns.o nfns.o ffi.o}
${bd}/%.o: src/%.c
	@echo $< | cut -c 5-
	@$(CC_INC) $@.d -o $@ -c $<

utils: ${addprefix ${bd}/, ryu.o utf.o hash.o file.o mut.o each.o bits.o}
${bd}/%.o: src/utils/%.c
	@echo $< | cut -c 5-
	@$(CC_INC) $@.d -o $@ -c $<

jit: ${addprefix ${bd}/, nvm.o}
${bd}/%.o: src/jit/%.c
	@echo $< | cut -c 5-
	@$(CC_INC) $@.d -o $@ -c $<

builtins: ${addprefix ${bd}/, arithm.o arithd.o cmp.o sfns.o squeeze.o select.o slash.o group.o sort.o search.o selfsearch.o transpose.o fold.o scan.o md1.o md2.o compare.o cells.o fns.o sysfn.o internal.o inverse.o}
${bd}/%.o: src/builtins/%.c
	@echo $< | cut -c 5-
	@$(CC_INC) $@.d -o $@ -c $<

.INTERMEDIATE: core base utils jit builtins bytecodeMessage

$(bd)/load.o: bytecodeMessage
bytecodeMessage:
ifeq ($(i_USE_BC_SUBMODULE),1)
	@echo "Using precompiled bytecode; see readme for how to build your own"
endif



# replxx
ifeq ($(i_REPLXX_1),1)
i_CXX := c++
ifeq ($(origin CXX),command line)
	i_CXX := $(CXX)
endif
i_LD = $(i_CXX)
REPLXX_FLAGS = -std=c++11 -Os

ALL_CC_FLAGS += -DUSE_REPLXX -I$(REPLXX_DIR)/include $(i_CC_PIE)

CXX_INC = $(i_CXX) $(CCFLAGS) $(REPLXX_FLAGS) $(CXXFLAGS) -DREPLXX_STATIC=1 -I$(REPLXX_DIR)/include -MMD -MP -MF
replxx_obj: ${addprefix ${bd}/, ConvertUTF.cpp.o wcwidth.cpp.o conversion.cxx.o escape.cxx.o history.cxx.o prompt.cxx.o replxx.cxx.o replxx_impl.cxx.o terminal.cxx.o util.cxx.o windows.cxx.o}
${bd}/%.o: $(REPLXX_DIR)/src/%
	@echo $<
	@$(CXX_INC) $@.d -o $@ -c $<
${bd}/BQN: replxx_obj
endif # replxx



# dependency files
-include $(bd)/*.d

DESTDIR =
PREFIX = /usr/local
install: uninstall
	[ -e BQN ] && cp -f BQN "$(DESTDIR)$(PREFIX)/bin/bqn"
	cp -f include/bqnffi.h "$(DESTDIR)$(PREFIX)/include/bqnffi.h"
	[ -e libcbqn.a ] && cp -f libcbqn.a "$(DESTDIR)$(PREFIX)/lib/libcbqn.a"
	[ -e libcbqn.so ] && cp -f libcbqn.so "$(DESTDIR)$(PREFIX)/lib/libcbqn.so"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/bqn"
	rm -f "$(DESTDIR)$(PREFIX)/include/bqnffi.h"
	rm -f "$(DESTDIR)$(PREFIX)/lib/libcbqn.a"
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
