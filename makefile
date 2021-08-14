SHELL = /usr/bin/env bash
MAKEFLAGS+= --no-print-directory

o3:
	@${MAKE} t=o3 b
o3g:
	@${MAKE} t=o3g b
o3n:
	@${MAKE} t=o3n b
rtperf:
	@${MAKE} t=rtperf b
rtverify:
	@${MAKE} t=rtverify b
debug:
	@${MAKE} t=debug b
debug1:
	@${MAKE} t=debug1 b
heapverify:
	@${MAKE} t=heapverify b
c: # custom
	@${MAKE} t=${t} FLAGS.${t}="${f}" b

b: gen

single-o3:
	$(CC) -std=gnu11 -Wall -Wno-unused-function -fms-extensions $(CCFLAGS) -no-pie -O3 -o BQN src/opt/single.c -lm
single-o3g:
	$(CC) -std=gnu11 -Wall -Wno-unused-function -fms-extensions $(CCFLAGS) -no-pie -O3 -g -o BQN src/opt/single.c -lm
single-debug:
	$(CC) -std=gnu11 -Wall -Wno-unused-function -fms-extensions $(CCFLAGS) -no-pie -DDEBUG -g -o BQN src/opt/single.c -lm
single-c:
	$(CC) -std=gnu11 -Wall -Wno-unused-function -fms-extensions $(CCFLAGS) -no-pie $(f) -o BQN src/opt/single.c -lm


# compiler setup
CC = clang
ifeq ($(CC),gcc)
CCFLAGS = -Wno-parentheses
else
CCFLAGS = -Wno-microsoft-anon-tag
endif
CMD = $(CC) -std=gnu11 -Wall -Wno-unused-function -fms-extensions ${CCFLAGS} $(FLAGS) -fPIE -MMD -MP -MF

# `if` to allow `make clean` alone to clean everything, but `make t=debug clean` to just clean obj/debug
ifeq ($(MAKECMDGOALS),clean)
t = *
else ifeq ($(t),)
t = o3
endif

ifneq (${t},debug1)
# don't make makefile cry about something idk
ifeq (${MAKECMDGOALS},b)
	MAKEFLAGS += -j4
endif
endif




# per-type flags
FLAGS.o3 := -O3
FLAGS.o3n := -O3 -march=native
FLAGS.o3g := -O3 -g
FLAGS.debug := -g -DDEBUG
FLAGS.debug1 := -g -DDEBUG
FLAGS.rtperf := -O3 -DRT_PERF
FLAGS.heapverify := -DDEBUG -g -DHEAP_VERIFY
FLAGS.rtverify := -DDEBUG -O3 -DRT_VERIFY
FLAGS = ${FLAGS.${t}}
bd = obj/${t}




gen: builtins core base jit utils # build the final binary
	@$(CC) -no-pie -o BQN ${bd}/*.o -lm
	@echo

builddir: # create the build directory. makefiles are stupid
	@mkdir -p ${bd}

# build individual object files
core: builddir ${addprefix ${bd}/, i32arr.o c32arr.o f64arr.o harr.o fillarr.o stuff.o derv.o mm.o heap.o}
${bd}/%.o: src/core/%.c
	@echo $< | cut -c 5-
	@$(CMD) $@.d -o $@ -c $<

base: builddir ${addprefix ${bd}/, load.o main.o rtwrap.o vm.o ns.o nfns.o}
${bd}/%.o: src/%.c
	@echo $< | cut -c 5-
	@$(CMD) $@.d -o $@ -c $<

utils: builddir ${addprefix ${bd}/, utf.o hash.o file.o mut.o}
${bd}/%.o: src/utils/%.c
	@echo $< | cut -c 5-
	@$(CMD) $@.d -o $@ -c $<

jit: builddir ${addprefix ${bd}/, nvm.o}
${bd}/%.o: src/jit/%.c
	@echo $< | cut -c 5-
	@$(CMD) $@.d -o $@ -c $<

builtins: builddir ${addprefix ${bd}/, arithm.o arithd.o cmp.o sfns.o sort.o md1.o md2.o fns.o sysfn.o internal.o}
${bd}/%.o: src/builtins/%.c
	@echo $< | cut -c 5-
	@$(CMD) $@.d -o $@ -c $<



src/gen/customRuntime:
	@echo "Copying precompiled bytecode from the bytecode branch"
	git checkout remotes/origin/bytecode src/gen/{compiler,formatter,runtime0,runtime1,src}
	git reset src/gen/{compiler,formatter,runtime0,runtime1,src}
${bd}/load.o: src/gen/customRuntime

-include $(bd)/*.d


clean:
	rm -f ${bd}/*.o
	rm -f ${bd}/*.d