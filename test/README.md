## Tests

Must be run from the projects root directory.

``` C
test/mainCfgs.sh path/to/mlochbaum/BQN // run the test suite for a couple primary configurations
test/x86Cfgs.sh  path/to/mlochbaum/BQN // run the test suite for x86-64-specific configurations, including singeli; 32-bit build is "supposed" to fail one test involving ⋆⁼
test/moreCfgs.sh path/to/mlochbaum/BQN // run "2+2" in a bunch of configurations; requires dzaima/BQN to be accessible as dbqn
./BQN test/cmp.bqn // fuzz-test scalar comparison functions =≠<≤>≥
./BQN test/equal.bqn // fuzz-test 𝕨≡𝕩
./BQN test/copy.bqn // fuzz-test creating new arrays with elements copied from another
./BQN test/bitcpy.bqn // fuzz-test bit_cpy; requires -DTEST_BITCPY
./BQN test/bit.bqn // fuzz-test •bit functions
./BQN test/mut.bqn // fuzz-test mut.h (currently just bitarr fill); requires -DTEST_MUT
./BQN test/hash.bqn // fuzz-test hashing
./BQN test/squeezeValid.bqn // fuzz-test squeezing giving a correct result; requires -DEEQUAL_NEGZERO
./BQN test/squeezeExact.bqn // fuzz-test squeezing giving the exact smallest result; requires -DEEQUAL_NEGZERO
./BQN test/various.bqn // tests for various small things
./BQN test/random.bqn // test (•MakeRand n).Range
./BQN test/joinReuse.bqn // test in-place join; requires -DPRINT_JOIN_REUSE
make -C test/ffi // test FFI functionality; expects both regular and shared library CBQN builds to already exist

legacy utilities:
  test/readTests.bqn // read mlochbaum/BQN tests in various formats
  test/precompiled.bqn // run a precompiled expression 
```