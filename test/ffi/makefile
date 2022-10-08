test: testLib testShared



testShared: buildShared
	@LD_LIBRARY_PATH=../../ ./sharedTest > shared.got
	@diff --color -su shared.expected shared.got
	
buildShared:
	$(CC) -g sharedTest.c -o sharedTest -L../.. -lcbqn



testLib: buildLib
	@../../BQN test.bqn > test.got
	@diff --color -su test.expected test.got

buildLib:
	$(CC) -O3 -g -c -fpic ffiTest.c -o ffiTest.o
	$(CC) -shared -olib.so ffiTest.o