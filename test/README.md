## Tests

Must be run from the projects root directory.

``` C
test/mainCfgs.sh path/to/mlochbaum/BQN // run the test suite for a couple primary configurations
test/x86Cfgs.sh  path/to/mlochbaum/BQN // run the test suite for x86-64-specific configurations, including singeli; 32-bit build is "supposed" to fail one test involving ⋆⁼
test/moreCfgs.sh path/to/mlochbaum/BQN // run "2+2" in a bunch of configurations
./BQN test/cmp.bqn // fuzz-test scalar comparison functions =≠<≤>≥
./BQN test/equal.bqn // fuzz-test 𝕨≡𝕩
./BQN test/bitcpy.bqn // fuzz-test bit_cpy; requires a CBQN build with -DTEST_BITCPY
./BQN test/squeeze.bqn // fuzz-test bit_cpy; requires a CBQN build with -DEEQUAL_NEGZERO
```