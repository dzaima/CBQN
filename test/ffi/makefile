test: testLib testShared



testShared: buildShared
	@LD_LIBRARY_PATH=../../ ./sharedTest > shared.got
	@diff --color -su shared.expected shared.got

buildShared:
	mkdir -p sharedLibFolder && cp ../../libcbqn.so sharedLibFolder
	$(CC) -g libTest.c -o sharedTest -LsharedLibFolder -lcbqn

buildStatic:
	mkdir -p staticLibFolder && cp ../../libcbqn.a staticLibFolder
	$(CC) -g libTest.c -o staticTest -LstaticLibFolder -lcbqn -lm -lffi



testLib: buildLib
	@../../BQN test.bqn > test.got
	@diff --color -su test.expected test.got

buildLib:
	$(CC) -O3 -g -c -fpic ffiTest.c -o ffiTest.o
	$(CC) -shared -olib.so ffiTest.o