## Tests

Must be run from the projects root directory. Some tests require `-DTEST_UTILS` (enabled by default in debug builds)

``` C
test/mainCfgs.sh path/to/mlochbaum/BQN // run the test suite for a couple primary configurations
test/x86Cfgs.sh  path/to/mlochbaum/BQN // run the test suite for x86-64-specific configurations, including singeli; 32-bit build is "supposed" to fail one test involving ⋆⁼
test/moreCfgs.sh path/to/mlochbaum/BQN // run "2+2" in a bunch of configurations; requires dzaima/BQN to be accessible as dbqn
test/run.bqn // run tests in test/cases/
./BQN test/cmp.bqn // fuzz-test scalar comparison functions =≠<≤>≥
./BQN test/copy.bqn // fuzz-test creating new arrays with elements copied from another
./BQN test/mut.bqn // fuzz-test mut.h (currently just bitarr fill)
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

# flags addable anywhere in code to restrict when it's run:
  %SLOW # enable only if 'slow' argument present
  %!DEBUG # disable if 'debug' argument present
  %!HEAPVERIFY # disable if 'heapverify' argument present
  %ALLOW_CATCH # allow running in noerr mode even if ⎊ is present
  %!PROPER_FILLS # enable only if has PROPER_FILLS==0
  %PROPER_FILLS # enable only if has PROPER_FILLS==1
```