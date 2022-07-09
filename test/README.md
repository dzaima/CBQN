## Tests

Must be run from the projects root directory.

``` C
test/mainCfgs.sh path/to/mlochbaum/BQN // run the test suite for a couple primary configurations
test/x86Cfgs.sh  path/to/mlochbaum/BQN // run the test suite for x86-64-specific configurations, including singeli; 32-bit build is "supposed" to fail one test involving ⋆⁼
test/moreCfgs.sh path/to/mlochbaum/BQN // run "2+2" in a bunch of configurations; requires dzaima/BQN to be accessible as dbqn
./BQN test/cmp.bqn // fuzz-test scalar comparison functions =≠<≤>≥
./BQN test/equal.bqn // fuzz-test 𝕨≡𝕩
./BQN test/copy.bqn // fuzz-test creating new arrays with elements copied from another
./BQN test/bitcpy.bqn // fuzz-test bit_cpy; requires a CBQN build with -DTEST_BITCPY
./BQN test/squeezeValid.bqn // fuzz-test squeezing giving a correct result; requires a CBQN build with -DEEQUAL_NEGZERO
./BQN test/squeezeExact.bqn // fuzz-test squeezing giving the exact smallest result; requires a CBQN build with -DEEQUAL_NEGZERO
./BQN test/random.bqn // various random tests
make -C test/ffi // test FFI functionality; expects both regular and shared library CBQN builds to already exist

legacy utilities:
  test/readTests.bqn // read mlochbaum/BQN tests in various formats
  test/precompiled.bqn // run a precompiled expression 
```