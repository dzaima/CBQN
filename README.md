build/run:

1. `./genRuntime path/to/mlochbaum/BQN`
2. optionally choose what to build by changing `src/main.c`
3. `./build` (or `./debugBuild` for a quicker unoptimized build)
4. `./BQN`

Time safe prim tests with self-hosted compiler:

`time ./primSafe.bqn path/to/mlochbaum/BQN | ./BQN`

Test precompiled safe prim tests:

1. uncomment [this](https://github.com/dzaima/CBQN/blob/528279b8e3e0fb108868f47b7bdfe772c26f10c3/src/main.c#L101-L106)
2. `./test.bqn path/to/mlochbaum/BQN "$PATH"`

Any file without an explicit copyright message is copyright (c) 2021 dzaima, GNU GPLv3 - see LICENSE