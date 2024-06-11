SHELL = /usr/bin/env sh
MAKEFLAGS+= --no-print-directory --no-builtin-rules --no-builtin-variables
MAKEHERE = "$(MAKE) -f build/makefile"
# note: do not manually define any i_… arguments, or incremental compiling may not work properly!



# simple non-incremental builds
single-o3:
	$(i_CC) $(ALL_CC_FLAGS) -DSINGLE_BUILD -O3 -o ${i_OUTPUT} src/opt/single.c $(ALL_LD_FLAGS)
single-o3g:
	$(i_CC) $(ALL_CC_FLAGS) -DSINGLE_BUILD -O3 -g -o ${i_OUTPUT} src/opt/single.c $(ALL_LD_FLAGS)
single-debug:
	$(i_CC) $(ALL_CC_FLAGS) -DSINGLE_BUILD -DDEBUG -g -o ${i_OUTPUT} src/opt/single.c $(ALL_LD_FLAGS)
single-c:
	$(i_CC) $(ALL_CC_FLAGS) -DSINGLE_BUILD -o ${i_OUTPUT} src/opt/single.c $(ALL_LD_FLAGS)



# dependency files
-include $(bd)/*.d
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
	NOWARN = -Wno-microsoft-anon-tag -Wno-bitwise-instead-of-logical -Wno-unknown-warning-option -Wno-pass-failed
else
	NOWARN = -Wno-parentheses
endif

ifeq ($(WINDOWS), 1)
	i_f+= -DNO_MMAP
	i_lf+= -lpthread
	
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
	printf "%s\0%s\0%s\0%s\0%s\0%s\0%s\0%s\0%s" "${i_CC}" "${ALL_CC_FLAGS}" "${ALL_LD_FLAGS}" "${CXXFLAGS}" "${i_CXX}" "${BYTECODE_DIR}" | $$hashInput | grep -oE '[0-9a-z]{64}' | head -c32
endif
else
	@printf "%s" "$(force_build_dir)"
endif

# actual build
run_incremental_0:
ifeq ($(i_t),forbuild)
	@mkdir -p build/obj2
endif
ifeq ($(verbose),1)
	@echo "build directory: $$("$(MAKEHERE)" builddir)"
	@echo "  bytecode: build/$(BYTECODE_DIR)"
	@echo "  cc invocation: $(CC_INC) \$$@.d -o \$$@ -c \$$<"
	@echo "  ld invocation: $(i_LD) ${CCFLAGS} -o [build_dir]/BQN [build_dir]/*.o $(ALL_LD_FLAGS)"
endif
ifeq ($(singeli),1)
	@echo "makefile-only builds don't support Singeli"; false
endif
	
ifeq ($(origin clean),command line)
	@"$(MAKEHERE)" clean-specific bd="$$("$(MAKEHERE)" builddir)"
else ifeq ($(origin builddir),command line)
	@echo "$$("$(MAKEHERE)" builddir)"
else # run build
	
ifeq ($(i_USE_BC_SUBMODULE),1)
	@git submodule update --init build/bytecodeSubmodule || (echo 'Failed to initialize submodule; clone CBQN as a git repo, or place local copies in build/ (see README.md#submodules).' && false)
endif
	@export bd=$$("$(MAKEHERE)" builddir); \
	[ "build/obj/" = "$$bd" ] && echo "Neither shasum nor sha256sum was found; cannot use custom configurations" && exit 1; \
	mkdir -p "$$bd";                   \
	"$(MAKEHERE)" run_incremental_1 bd="$$bd"
endif # run build

run_incremental_1: ${bd}/BQN
ifneq (${bd}/BQN,${i_OUTPUT})
ifeq ($(WINDOWS),1)
	@cp -f ${bd}/BQN.exe "${i_OUTPUT}"
else
	@cp -f ${bd}/BQN "${i_OUTPUT}"
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
