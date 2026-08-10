SHELL = /usr/bin/env bash
test: test-lib test-shared test-extended



sharedTestBin: libraryTest.c ../../libcbqn.so
	mkdir -p sharedLibFolder && cp ../../libcbqn.so sharedLibFolder
	$(CC) -g libraryTest.c -o sharedTestBin -LsharedLibFolder -lcbqn

staticTestBin: libraryTest.c ../../libcbqn.a
	mkdir -p staticLibFolder && cp ../../libcbqn.a staticLibFolder
	$(CC) -g libraryTest.c -o staticTestBin -LstaticLibFolder -lcbqn -lm -lffi -rdynamic

lib.so: testlib.c
	$(CC) -O3 -g -c -fpic testlib.c -o testlib.o
	$(CC) -shared -olib.so testlib.o



test-shared: sharedTestBin
	@LD_LIBRARY_PATH=../../ ./sharedTestBin > shared.got
	@diff --color -su library.expected shared.got

test-static: staticTestBin
	@./staticTestBin > static.got
	@diff --color -su library.expected static.got

test-extended: lib.so
	@../../BQN $(BQN_FLAGS) extended.bqn > extended.got
	@diff --color -su extended.expected extended.got

test-lib: lib.so
	@../../BQN $(BQN_FLAGS) test.bqn > test.got
	@diff --color -su test.expected test.got

clean:
	rm -f sharedTest{,Bin} staticTest{,Bin} testlib.o lib.so {test,shared,extended}.got ffiTest.{o,so} sharedLibFolder/libcbqn.so staticLibFolder/libcbqn.a
	rmdir sharedLibFolder staticLibFolder;true