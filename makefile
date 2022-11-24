SHELL = /usr/bin/env bash
MAKEFLAGS+= --no-print-directory --no-builtin-rules
# note: do not manually define any i_… arguments, or incremental compiling will not work properly!

# various configurations
o3:
	@${MAKE} i_singeli=0 i_t=o3         i_f="-O3" run_incremental_0
o3g:
	@${MAKE} i_singeli=0 i_t=o3g        i_f="-O3 -g" run_incremental_0
o3n:
	@${MAKE} i_singeli=0 i_t=o3n        i_f="-O3 -march=native" run_incremental_0
debug:
	@${MAKE} i_singeli=0 i_t=debug      i_f="-g -DDEBUG" run_incremental_0
debug1:
	@${MAKE} i_singeli=0 i_t=debug1     i_f="-g -DDEBUG" manualJobs=1 run_incremental_0
rtperf:
	@${MAKE} i_singeli=0 i_t=rtperf     i_f="-O3 -DRT_PERF" run_incremental_0
rtverify:
	@${MAKE} i_singeli=0 i_t=rtverify   i_f="-DDEBUG -O3 -DRT_VERIFY -DEEQUAL_NEGZERO" run_incremental_0
heapverify:
	@${MAKE} i_singeli=0 i_t=heapverify i_f="-DDEBUG -g -DHEAP_VERIFY" run_incremental_0
o3n-singeli:
	@${MAKE} i_singeli=1 i_t=o3n_si     i_f="-O3 -march=native" run_incremental_0
o3ng-singeli:
	@${MAKE} i_singeli=1 i_t=o3ng_si    i_f="-g -O3 -march=native" run_incremental_0
debugn-singeli:
	@${MAKE} i_singeli=1 i_t=debugn_si  i_f="-g -DDEBUG -march=native" run_incremental_0
heapverifyn-singeli:
	@${MAKE} i_singeli=1 i_t=heapverifyn_si i_f="-g -DDEBUG -DHEAP_VERIFY -march=native" run_incremental_0
rtverifyn-singeli:
	@${MAKE} i_singeli=1 i_t=rtverifyn_si i_f="-O3 -DRT_VERIFY -DEEQUAL_NEGZERO -march=native" run_incremental_0
wasi-o3:
	@${MAKE} i_singeli=0 i_t=wasi_o3 i_OUTPUT=BQN.wasm i_f="-DWASM -DWASI -DNO_MMAP -O3 -DCATCH_ERRORS=0 -D_WASI_EMULATED_MMAN --target=wasm32-wasi" i_lf="-lwasi-emulated-mman --target=wasm32-wasi -Wl,-z,stack-size=8388608 -Wl,--initial-memory=67108864" i_LIBS_LD= i_PIE= i_FFI=0 run_incremental_0
emcc-o3:
	@${MAKE} i_singeli=0 i_t=emcc_o3 i_OUTPUT=. i_emcc=1 CC=emcc i_f='-DWASM -DEMCC -O3' i_lf='-s EXPORTED_FUNCTIONS=_main,_cbqn_runLine,_cbqn_evalSrc -s EXPORTED_RUNTIME_METHODS=ccall,cwrap -s ALLOW_MEMORY_GROWTH=1' i_FFI=0 run_incremental_0
shared-o3:
	@${MAKE} i_OUTPUT=libcbqn.so i_SHARED=1 i_t=shared_o3 i_f="-O3" run_incremental_0
shared-c:
	@${MAKE} i_OUTPUT=libcbqn.so i_SHARED=1 custom=1                run_incremental_0
c:
	@${MAKE} custom=1 run_incremental_0

# compiler setup
i_CC := clang
i_PIE := -no-pie
i_LIBS_LD := -lm
i_LIBS_CC := -fvisibility=hidden
i_FFI := 2
i_singeli := 0
i_OUTPUT := BQN

ifneq ($(origin OUTPUT),command line)
	OUTPUT := $(i_OUTPUT)
endif
ifeq ($(i_emcc),1)
	OUTPUT_FOLDER := $(OUTPUT)
	OUTPUT_BIN := $(OUTPUT_FOLDER)/BQN.js
else
	OUTPUT_BIN := $(OUTPUT)
endif
ifeq ($(origin CC),command line)
	i_CC := $(CC)
	custom = 1
endif
ifeq ($(origin singeli),command line)
	i_singeli := $(singeli)
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
ifeq ($(origin PIE),command line)
	i_PIE := $(PIE)
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
ifeq ($(REPLXX),1)
	custom = 1
	REPLXX_DIR = $(shell if [ -d build/replxxLocal ]; then echo build/replxxLocal; else echo build/replxxSubmodule; fi)
endif
ifeq ($(i_singeli),1)
	SINGELI_DIR = $(shell if [ -d build/singeliLocal ]; then echo build/singeliLocal; else echo build/singeliSubmodule; fi)
endif
BYTECODE_DIR = $(shell if [ -d build/bytecodeLocal ]; then echo bytecodeLocal; else echo bytecodeSubmodule; fi)
ifeq ($(BYTECODE_DIR),bytecodeLocal)
	custom = 1
endif

i_LD = $(i_CC)

CC_IS_CLANG = $(shell $(i_CC) --version | head -n1 | grep -m 1 -c "clang")
ifeq (${CC_IS_CLANG}, 1)
	NOWARN = -Wno-microsoft-anon-tag -Wno-bitwise-instead-of-logical -Wno-unknown-warning-option
else
	NOWARN = -Wno-parentheses
endif

ALL_CC_FLAGS = -std=gnu11 -Wall -Wno-unused-function -fms-extensions -ffp-contract=off -fno-math-errno $(CCFLAGS) $(f) $(i_f) $(NOWARN) -DBYTECODE_DIR=$(BYTECODE_DIR) -DSINGELI=$(i_singeli) -DFFI=$(i_FFI) $(i_LIBS_CC)
ALL_LD_FLAGS = $(LDFLAGS) $(lf) $(i_lf) $(i_PIE) $(i_LIBS_LD)

ifneq (${manualJobs},1)
	ifeq (${MAKECMDGOALS},run_incremental_1)
		MAKEFLAGS+= -j4 manualJobs=1
	endif
	ifeq (${MAKECMDGOALS},build_singeli)
		MAKEFLAGS+= -j4 manualJobs=1
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
	printf "%s\0%s\0%s\0%s\0%s\0%s\0%s\0%s\0%s" "${i_CC}" "${ALL_CC_FLAGS}" "${ALL_LD_FLAGS}" "${REPLXX}" "${REPLXX_FLAGS}" "${i_CXX}" "${BYTECODE_DIR}" "${REPLXX_DIR}" "${SINGELI_DIR}" | $$hashInput | grep -oE '[0-9a-z]{64}' | head -c32
endif
else
	@printf "%s" "$(force_build_dir)"
endif


# simple non-incremental builds
single-o3:
	$(i_CC) $(ALL_CC_FLAGS) -O3 -o ${OUTPUT_BIN} src/opt/single.c $(ALL_LD_FLAGS)
single-o3g:
	$(i_CC) $(ALL_CC_FLAGS) -O3 -g -o ${OUTPUT_BIN} src/opt/single.c $(ALL_LD_FLAGS)
single-debug:
	$(i_CC) $(ALL_CC_FLAGS) -DDEBUG -g -o ${OUTPUT_BIN} src/opt/single.c $(ALL_LD_FLAGS)
single-c:
	$(i_CC) $(ALL_CC_FLAGS) -o ${OUTPUT_BIN} src/opt/single.c $(ALL_LD_FLAGS)

# actual build
run_incremental_0:
ifeq ($(verbose),1)
	@echo "build directory: $$(${MAKE} builddir)"
	@echo "  bytecode: build/$(BYTECODE_DIR)"
ifeq ($(REPLXX),1)
	@echo "  replxx: $(REPLXX_DIR)"
else
	@echo "  replxx: not used"
endif
ifeq ($(i_singeli), 1)
	@echo "  singeli: $(SINGELI_DIR)"
else
	@echo "  singeli: not used"
endif
	@echo "  cc invocation: $(CC_INC) \$$@.d -o \$$@ -c \$$<"
	@echo "  ld invocation: $(i_LD) ${CCFLAGS} -o [build_dir]/BQN [build_dir]/*.o $(ALL_LD_FLAGS)"
endif

ifeq ($(origin clean),command line)
	@export bd=$$(${MAKE} builddir); \
	${MAKE} clean-specific bd="$$bd"
else ifeq ($(origin builddir),command line)
	@export bd=$$(${MAKE} builddir); \
	echo "$$bd"
else # run build
	
ifeq ($(i_singeli), 1)
	@mkdir -p src/singeli/gen
	@mkdir -p build/obj/singeli
	@${MAKE} postmsg="post-singeli build:" build_singeli
endif
	
ifeq ($(REPLXX_DIR),build/replxxSubmodule)
	@git submodule update --init build/replxxSubmodule
endif
ifeq ($(BYTECODE_DIR),bytecodeSubmodule)
	@git submodule update --init build/bytecodeSubmodule
endif
	@export bd=$$(${MAKE} builddir); \
	mkdir -p "$$bd";                 \
	${MAKE} run_incremental_1 bd="$$bd"
endif # run build

run_incremental_1: ${bd}/BQN
ifneq (${bd}/BQN,${OUTPUT_BIN})
ifeq ($(i_emcc),1)
	@cp -f ${bd}/BQN.wasm ${OUTPUT_FOLDER}/BQN.wasm
endif
	@cp -f ${bd}/BQN ${OUTPUT_BIN}
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

utils: ${addprefix ${bd}/, utf.o hash.o file.o mut.o each.o bits.o}
${bd}/%.o: src/utils/%.c
	@echo $< | cut -c 5-
	@$(CC_INC) $@.d -o $@ -c $<

jit: ${addprefix ${bd}/, nvm.o}
${bd}/%.o: src/jit/%.c
	@echo $< | cut -c 5-
	@$(CC_INC) $@.d -o $@ -c $<

builtins: ${addprefix ${bd}/, arithm.o arithd.o cmp.o sfns.o squeeze.o select.o slash.o group.o sort.o search.o selfsearch.o fold.o scan.o md1.o md2.o fns.o sysfn.o internal.o inverse.o}
${bd}/%.o: src/builtins/%.c
	@echo $< | cut -c 5-
	@$(CC_INC) $@.d -o $@ -c $<

.INTERMEDIATE: core base utils jit builtins bytecodeMessage

$(bd)/load.o: bytecodeMessage
bytecodeMessage:
ifeq ($(BYTECODE_DIR),bytecodeSubmodule)
	@echo "Using precompiled bytecode; see readme for how to build your own"
endif


# singeli
.INTERMEDIATE: preSingeliBin
preSingeliBin:
ifeq ($(SINGELI_DIR),build/singeliSubmodule)
	@git submodule update --init build/singeliSubmodule
endif
	@echo "pre-singeli build:"
	@${MAKE} i_singeli=0 singeli=0 force_build_dir=build/obj/presingeli REPLXX=0 f= lf= postmsg="singeli sources:" i_t=presingeli i_f='-O1 -DPRE_SINGELI' FFI=0 OUTPUT=build/obj/presingeli/BQN c


build_singeli: ${addprefix src/singeli/gen/, cmp.c dyarith.c copy.c equal.c squeeze.c fold.c scan.c neq.c slash.c constrep.c bits.c}
	@echo $(postmsg)
src/singeli/gen/%.c: src/singeli/src/%.singeli preSingeliBin
	@echo $< | cut -c 17- | sed 's/^/  /'
	@build/obj/presingeli/BQN build/singeliMake.bqn "$(SINGELI_DIR)" $< $@ "build/obj/singeli/"

ifeq (${i_singeli}, 1)
# arithmetic table generator
src/builtins/arithd2.c: src/singeli/c/dyarith.c
src/singeli/src/builtins/arithd.c: src/singeli/gen/arTables.c
src/singeli/gen/dyarith.c: src/singeli/gen/arDefs.singeli genArithTables

src/singeli/gen/arDefs.singeli: genArithTables
src/singeli/gen/arTables.c: genArithTables

.INTERMEDIATE: genArithTables
genArithTables: src/singeli/src/genArithTables.bqn preSingeliBin
	@echo "  generating arDefs.singeli & arTables.c"
	@build/obj/presingeli/BQN src/singeli/src/genArithTables.bqn "$$PWD/src/singeli/gen/arDefs.singeli" "$$PWD/src/singeli/gen/arTables.c"
endif



# replxx
ifeq ($(REPLXX),1)
i_CXX := c++
ifeq ($(origin CXX),command line)
	i_CXX := $(CXX)
endif
i_LD = $(i_CXX)
REPLXX_FLAGS = -Os -std=c++11

ALL_CC_FLAGS += -DUSE_REPLXX -I$(REPLXX_DIR)/include

CXX_INC = $(i_CXX) $(CCFLAGS) $(REPLXX_FLAGS) -DREPLXX_STATIC=1 -I$(REPLXX_DIR)/include -MMD -MP -MF
replxx_obj: ${addprefix ${bd}/, ConvertUTF.cpp.o wcwidth.cpp.o conversion.cxx.o escape.cxx.o history.cxx.o prompt.cxx.o replxx.cxx.o replxx_impl.cxx.o terminal.cxx.o util.cxx.o windows.cxx.o}
${bd}/%.o: $(REPLXX_DIR)/src/%
	@echo $<
	@$(CXX_INC) $@.d -o $@ -c $<
${bd}/BQN: replxx_obj
endif # replxx



# dependency files
-include $(bd)/*.d
ifeq (${i_singeli}, 1)
-include build/obj/singeli/*.d
endif

DESTDIR =
PREFIX = /usr/local
install: uninstall
	cp -f BQN "$(DESTDIR)$(PREFIX)/bin/bqn"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/bqn"

clean-singeli:
	rm -rf src/singeli/gen/
	rm -rf build/obj/singeli/
	@${MAKE} clean-specific bd=build/obj/presingeli
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
	

clean: clean-build clean-singeli
