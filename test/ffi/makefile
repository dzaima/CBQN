test: build
	@../../BQN test.bqn > test.got
	@diff --color -su test.expected test.got

build:
	$(CC) -O3 -g -c -fpic ffiTest.c -o ffiTest.o
	$(CC) -shared -olib.so ffiTest.o