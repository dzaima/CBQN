## Tests

Must be run from the projects root directory.

``` C
test/mainCfgs.sh path/to/mlochbaum/BQN // run the test suite for a couple primary configurations
test/x86Cfgs.sh  path/to/mlochbaum/BQN // run the test suite for x86-64-specific configurations, including singeli; 32-bit build is "supposed" to fail one test involving ⋆⁼
test/moreCfgs.sh path/to/mlochbaum/BQN // run "2+2" in a bunch of configurations; requires dzaima/BQN to be accessible as dbqn
test/run.bqn // run tests in test/cases/
./BQN test/cmp.bqn // fuzz-test scalar comparison functions =≠<≤>≥
./BQN test/equal.bqn // fuzz-test 𝕨≡𝕩
./BQN test/copy.bqn // fuzz-test creating new arrays with elements copied from another
./BQN test/bitcpy.bqn // fuzz-test bit_cpy; requires -DTEST_BITCPY
./BQN test/bit.bqn // fuzz-test •bit functions
./BQN test/mut.bqn // fuzz-test mut.h (currently just bitarr fill); requires -DTEST_MUT
./BQN test/hash.bqn // fuzz-test hashing
./BQN test/squeezeValid.bqn // fuzz-test squeezing giving a correct result
./BQN test/squeezeExact.bqn // fuzz-test squeezing giving the exact smallest result
./BQN test/various.bqn // tests for various small things
./BQN test/random.bqn // test (•MakeRand n).Range
./BQN test/joinReuse.bqn // test in-place join; requires -DPRINT_JOIN_REUSE
make -C test/ffi // test FFI functionality; expects both regular and shared library CBQN builds to already exist

legacy utilities:
  test/readTests.bqn // read mlochbaum/BQN tests in various formats
  test/precompiled.bqn // run a precompiled expression 
```

Format of tests in `test/cases/`:
```python
%DEF somename some code # add a definition to be used by later tests or other %DEFs
%USE somename # copy-paste in the code of the given definition to here
tests:
  code that runs some !assertions # if there are no '!'s, 'lint' will complain
  !"error message" % erroring code # can be written as "!% erroring code" at first, and let 'update-messages' insert the message
  code %% expected value
  (
    multiline
    code
    running !assertions
    # the '(' and ')' lines must have no other characters in them
  )

# flags addable anywhere in code:
  %SLOW # enable only if 'slow' argument present
  %!DEBUG # disable if 'debug' argument present
  %!HEAPVERIFY # disable if 'heapverify' argument present
  %!PROPER_FILLS # enable only if has PROPER_FILLS==0
  %PROPER_FILLS # enable only if has PROPER_FILLS==1
```