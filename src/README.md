# Overview of the CBQN source

## Conventions

Functions starting with `m_` allocate a new object.  
Functions starting with `q_` are queries/predicates, and return a boolean.  
Functions ending with `R` are either supposed to be called rarely, or the caller expects that a part of it happens rarely.  
Functions ending with `U` return (or take) a non-owned object (`U` = "unincremented").  
Functions ending with `_c1` are monadic implementations, `_c2` are dyadic (see [builtin implementations](#builtin-implementations))
Variables starting with `bi_` are builtins (primitives or special values).  
Which arguments are consumed usually is described in a comment after the function or its prototype. Otherwise, check the source.  

```
src/
  builtins/
    sfns.c      structural functions
    fns.c       other functions
    arithd.c    dyadic arithmetic functions
    arithm.c    monadic arithmetic functions (incl •math stuff)
    cmp.c       dyadic comparison functions
    sort.c      sort/grade/bins
    md1.c       1-modifiers
    md2.c       2-modifiers
    sysfn.c     •-definitions
    internal.c  •internal
  opt/    files which aren't needed for every build configuration
  gen/    generated files
  jit/    simple JIT compiler for x86-64
  utils/  utilities included as needed
    builtins.h  definitions of all built-in functions (excluding things defined by means of nfns.c)
    file.h      file system operations
    hash.h      hashing things
    mut.h       copying multiple arrays into a single array
    talloc.h    temporary allocations (described more below)
    utf.h       UTF-8 things
  core/   things included everywhere
  core.h  file imported everywhere that defines the base BQN model
  nfns.c  native functions for things that need to keep some state (e.g. •FLines needs to also hold the path its relative to)
  load.c  loads the self-hosted compiler, runtime and formatter, initializes CBQN globals
  main.c  main function & commandline stuff
  ns.c    namespaces
  vm.c    virtual machine interpreter
)
```

## Base

`B` represents any BQN object. Some are heap-allocated, some are not.
```C
Type checks (all are safe to execute on any B instance):
  test       tag       instance description
  isVal(x)   [many]    any heap-allocated object
  isFun(x)   FUN_TAG   a function    (guaranteed heap-allocated)
  isMd1(x)   MD1_TAG   a 1-modifier  (guaranteed heap-allocated)
  isMd2(x)   MD2_TAG   a 2-modifier  (guaranteed heap-allocated)
  isMd (x)   [many]    any modifier  (guaranteed heap-allocated)
  isCallable(x) [many] isFun|isMd    (guaranteed heap-allocated)
  isArr(x)   ARR_TAG   an array type (guaranteed heap-allocated)
  isAtm(x)   [many]    !isArr(x)     can be either heap-allocated or tagged
  isChr(x)   C32_TAG   a character   (never heap-allocated)
  isF64(x)   F64_TAG   a number      (never heap-allocated)
  isNsp(x)   NSP_TAG   a namespace   (guaranteed heap-allocated)
  isObj(x)   OBJ_TAG   internal      (guaranteed heap allocated)
  and then there are some extra types for variables for the VM & whatever; see h.h *_TAG definitions

Extra functions for converting types:
  m_f64(x)  // f64 → B
  m_c32(x)  // codepoint → B
  m_i32(x)  // i32 → B
  m_usz(x)  // usz → B
  o2i(x)    // B → i32, throw if impossible
  o2i64(x)  // B → i64, throw if impossible
  o2u64(x)  // B → u64, throw if impossible
  o2s(x)    // B → usz, throw if impossible
  o2c(x)    // B → c32, throw if impossible
  o2f(x)    // B → f64, throw if impossible
  o2iu(x)   // B → i32, assumes is valid (u ≡ unchecked)
  o2i64u(x) // B → i64, assumes is valid
  o2su(x)   // B → usz, assumes is valid
  o2cu(x)   // B → c32, assumes is valid
  o2fu(x)   // B → f64, assumes is valid
  o2b(x)    // B → bool, throw if impossible
  q_TYPE(x) // query if x is convertible to TYPE (see definitions in h.h)
  q_N(x)    // query if x is · (≡ bi_N)
  noFill(x)    // if x represents undefined fill (returned by getFill*; ≡ bi_noFill)
  tag(x,*_TAG) // pointer → B

See src/h.h for more basic functions
```

An object can be allocated with `mm_alloc(sizeInBytes, t_something)`. The returned object is a `Value*`, but should be used as some subtype. `mm_free` can be used to force-free an object regardless of its reference count.

A heap-allocated object can be cast to a `Value*` with `v(x)`, to an `Arr*` with `a(x)`, or to any pointer type with `c(Type,x)`. `v(x)->type` stores the type of an object (see `enum Type` in `src/h.h`), which is used by runtime functions to decide how to interpret an object.

Reference count of any `B` object can be incremented/decremented with `inc(x)`/`dec(x)` (`inc(x)` also returns `x`, so you can use it inline in the consumer). A pointer type has `ptr_inc(x)`/`ptr_dec(x)`. `dec`/`ptr_dec` will return the object to the memory manager if the refcount goes to zero.

Since reference counting is hard, there's `make heapverify` that verifies that code executed does it right (and screams unreadable messages when it doesn't). After any changes, I'd suggest running:
```bash
#!/usr/bin/env bash
make   rtverify && echo   'rtverify:' && ./BQN -M 1000 path/to/mlochbaum/BQN/test/this.bqn
make heapverify && echo 'heapverify:' && ./BQN -M 1000 path/to/mlochbaum/BQN/test/this.bqn -noerr bytecode header identity literal namespace prim simple syntax token under undo
```

Temporary allocations can be made with `utils/talloc.h`:
```C
#include "utils/talloc.h"
TALLOC(char, buf, 123); // allocate char* buf with 123 elements
// buf is now a regular char* and can be stored/passed around as needed
TREALLOC(buf, 456); // extend buf
TFREE(buf); // free buf
// if the size is guaranteed to be small enough, using VLAs is fine i guess

TSALLOC(i32, stack, 10); // allocate an i32 stack with initially reserved 10 items (initial reserve must be positive!)
TSADD(stack, 15); // add a single item
TSADD(stack, (i32*){1,2,3}, 3); // add many items
usz sz = TSSIZE(stack); // get the current height of the stack
i32 item = stack[1]; // get the 2nd item
TSFREE(stack); // free the stack
// note that TSALLOC creates multiple local variables, and as such cannot be passed around to other functions
```

## Virtual functions

All virtual method accesses require that the argument is heap-allocated.

You can get a virtual function of a `B` instance with `TI(x, something)`. There's also `TIv(x, something)` for a pointer `x` instead. See `#define FOR_TI` in `src/h.h` for available functions.

Call a BQN function with `c1(f, x)` or `c2(f, w, x)`. A specific builtin can be called by looking up the appropriate name in `src/utils/builtins.h` (and adding the `bi_` prefix).

Calling a modifier involves deriving it with `m1_d`/`m2_d`, using a regular `c1`/`c2`, and managing the refcounts of everything while at that.

## Builtin implementations

The list of builtin functions is specified in the initial macros of `src/utils/builtins.h`, where `A`/`M`/`D` are used for ambivalent/monadic/dyadic. Once added, `bi_yourName` will be available, and the required of the following functions must be defined somewhere in the source:

```C
// functions:
B yourName_c1(B t, B x);
B yourName_c2(B t, B w, B x);
// 1-modifiers:
B yourName_c1(Md1D* d, B x);
B yourName_c2(Md1D* d, B w, B x);
// 2-modifiers:
B yourName_c1(Md2D* d, B x);
B yourName_c2(Md2D* d, B w, B x);
```

For functions, in most cases, the `t` parameter (representing `𝕊`/"this") is unused (it _must_ be ignored for functions managed by `builtins.h`), but can be used for objects from `nfns.h` to store state with a function.

For modifiers, the `d` parameter stores the operands and the modifier itself. Use `d->f` for `𝔽`, `d->g` for `𝔾`, `d->m1` for `_𝕣`, `d->m2` for `_𝕣_`, and `tag(d,FUN_TAG)` for `𝕊`.


## Arrays

If you know that `x` is an array (e.g. by testing `isArr(x)` beforehand), `a(x)->ia` will give you the product of the shape, `rnk(x)` will give you the rank, and `a(x)->sh` will give you a `usz*` to the full shape.

Allocating an array:
```C
i32* rp; B r = m_i32arrv(&rp, 123); // allocate a 123-element i32 vector
i32* rp; B r = m_i32arrc(&rp, x); // allocate an array with the same shape as x (x must be an array; x isn't consumed)

i32* rp; Arr* r = m_i32arrp(&rp, 123); // allocate a 123-element i32-array without allocating shape
// then do one of these:
arr_shVec(r); // set shape of r to a vector
usz* sh = arr_shAlloc(r, 4); // allocate a rank 4 shape; write to sh the individual items; sh will be NULL for ranks 0 and 1
arr_shCopy(r, x); // copy the shape object of x (doesn't consume x)
B result = taga(r);
// see stuff.h for m_shArr/arr_shSetI/arr_shSetU for ways to batch-assign a single shape object to multiple objects

u32* rp; B r = m_c32arrv(%rp, 10); // 10-char string
// etc for m_(c32|f64|i32)arr[vcp]

usz ri=0; HArr_p r = m_harrs(123, &ri); // allocate 123-item arbitrary type array
// write items with r.a[ri++] or equivalent. Whenever GC could happen, ri must point to after the end of the currently set items
// then do one of these to finish the array:
B result = harr_fv(r); // sets shape to a vector
B result = harr_fc(r, x); // copies the shape of x, doesn't consume x
B result = harr_fcd(r, x); // copies the shape of x and consumes it
usz* sh = harr_fa(r, 4); // allocate shape for a rank 4 array. To get the result `B` object, just do r.b
// If at any point you want to free the object before finishing it, use harr_abandon(r)

// If you're sure GC cannot happen before all items in the array are set, you can use: (U means "unsafe")
HArr_p r = m_harrUv(10); // 10-item vector
HArr_p r = m_harrUc(10, x); // 10-item array with the same shape as x
HArr_p r = m_harrUp(10); // 10-item array without any set shape. Use the arr_shWhatever(r.c, …)

// you can use withFill to add a fill element to a created array (or manually create a fillarr, see src/core/fillarr.h)

B r = m_str32(U"⟨1⋄2⋄3⟩"); // a constant string with unicode chars
B r = m_str8(5, "hello"); // an ASCII string with specified length
B r = m_str8l("hello"); // an ASCII string, read until null byte
#include "utils/utf.h"
B r = fromUTF8l("⟨1⋄2⋄3⟩") // parse UTF-8 from a char*
B r = fromUTF8("⟨1⋄2⋄3⟩", 17) // ↑ but with a specified length
u64 sz = utf8lenB(x); TALLOC(char, buf, sz+1); toUTF8(x, buf); buf[sz]=0; /*use buf as a C-string*/ TFREE(buf);

// src/utils/mut.h provides a way to build an array by copying parts of other arrays

// some functions for specific cases:
B r = m_unit(x); // equivalent to <𝕩
B r = m_hunit(x); // like the above, except no fill is set
B r = m_atomUnit(x); // if x is likely to be an atom, this is a better alternative to m_unit
B r = m_v1(a); // ⟨a⟩
B r = m_v2(a,b); // ⟨a,b⟩
B r = m_v3(a,b,c); // ⟨a,b,c⟩
B r = m_v4(a,b,c,d); // ⟨a,b,c,d⟩
B r = emptyHVec(); // an empty vector with no fill
B r = emptyIVec(); // an empty integer vector
B r = emptyCVec(); // an empty character vector
B r = emptySVec(); // an empty string vector
// rank can be manually set for an exsiting object with sprnk or srnk, but be careful to keep the shape object in sync!
```


Retrieving data from arrays:
```C
// generic methods:
SGet(x) // initializes the getter for fast reads; the argument must be a variable name
B c = Get(x,n); // in a loop, reating the n-th item

SGetU(x)
B c = GetU(x,n); // alternatively, GetU can be used to not increment the result. Useful for temporary usage of the item

B c = IGet(x,n); // skip the initialize/call separation; don't use in loops
B c = IGetU(x,n);

// for specific array types:
if (TI(x,elType)==el_i32) i32* xp = i32any_ptr(x); // for either t_i32arr or t_i32slice; for t_i32arr only, there's i32arr_ptr(x)
if (TI(x,elType)==el_c32) u32* xp = c32any_ptr(x); // ↑
if (TI(x,elType)==el_f64) f64* xp = f64any_ptr(x); // ↑
if (v(x)->type==t_harr) B* xp = harr_ptr(x);
if (v(x)->type==t_harr || v(x)->type==t_hslice) B* xp = hany_ptr(x); // note that elType==el_B doesn't imply hany_ptr is safe!
if (v(x)->type==t_fillarr) B* xp = fillarr_ptr(x);
B* xp = arr_bptr(x); // will return NULL if the array isn't backed by contiguous B*-s
```

## Errors

Throw an error with `thrM("some message")` or `thr(some B instance)` or `thrOOM()`. What to do with active references at the time of the error is TBD when GC is actually completed, but you can just not worry about that for now.

A fancier message can be created with `thrF(message, …)` with printf-like (but different!!) varargs (source in `do_fmt`):
```
%i   decimal i32 (also for i8/i16/ur)
%l   decimal i64
%ui  decimal u32 (also for u8/u16)
%ul  decimal u64
%xi  hex u32
%xl  hex u64
%s   decimal usz
%f   f64
%p   pointer
%c   char
%S   char* C-string consisting of ASCII
%U   char* of UTF-8 data
%R   a B instance of a string or number
%H   the shape of a B instance
%%   "%"
```
See `#define CATCH` in `src/h.h` for how to catch errors.

Use `assert(predicate)` for checks (for optimized builds they're replaced with `if (!predicate) invoke_undefined_behavior();` so it's still invoked!!). `UD;` can be used to explicitly invoke undefined behavior (equivalent in behavior to `assert(false);`), which is useful for things like untaken `default` branches in `switch` statements.

There's also `err("message")` that (at least currently) is kept in optimized builds as-is, and always kills the process on being called.