## Overview

Types used here follow `[VET][ISUF]?\d?`.  
First character:
  - `V`: some vector type
  - `E`: some scalar type
  - `T`: either scalar or vector type

Second character, if present:
  - `F`: float
  - `S`: signed integer
  - `U`: unsigned integer (may include `u1`)
  - `I`: any signedness integer (may include `u1`)

Equally-named types should match. A number is added to the type to separate unequal types.

`mt{T}` is a mask version of the argument - either `ty_u{T}` or `[vcount{T}]u1` depending on the target.

The SIMD operations listed aren't guaranteed to be supported on all targets, nor list all possible argument types.

## General definitons

- `has_simd`  
  `0` or `1` depending on whether SIMD types are available.

- `arch_defvw`  
  Target vector width for the current target architecture.  
  If `has_simd==0`, should not be used. Otherwise, vectors of all element types (except `u1`) whose width is this must be supported, but other widths (both larger and smaller) may be too.

- `reinterpret` is available as `~~`
<!-- -->
- `export_tab{'name', fs}` - export list of functions as the specified name
- `iota{knum}` - tuple `tup{0,1,...,knum-1}`
- `broadcast{knum,v}` (`knum**v`) - tuple with `knum` items, all `v`
- `tern{c, t, f}` - ternary - if `c`, return `t`, else `f`
- `eachx{F, ...args}` - `each` but broadcasting non-tuples
- `undef{T}` - return register of type `T` with undefined value; maps over a tuple of types
- `undef{T, n}` - a tuple of `n` different undefined registers
- `one_val{xs}` - assert that `xs` is a tuple of equivalent (as per `same`) items, and return the first
- `one_type{xs}` - `oneVal` but over the types of the items
- `all_same{xs}` - whether all values of `xs` are the same

## Loops & branches

- `@for` - generate body once, with indices `s,s+1,...,e-2,e-1`
- `@for_nz` - `@for`, but assumed to run at least once
- `@for_unroll_block{exp, unr}` - generate a main loop body that takes a tuple of `unr` indices to process. Handle the tail via processing one index at a time; if `exp==1`, that's done via `unr` generated bodies, otherwise it's a loop
- `@unroll(n)` - generate body `n` times
<!-- -->
- `make_branch{Ts, G=={...x:Ts}=>{}} == {...x:Ts}=>{}` - make a code segment that takes arguments of types `Ts`, and can be jumped to by invoking the result of the `make_branch`
- `make_opt_ranch{enable, Ts, G}` - `make_branch` but only generated & invokable when `enable==1`

## Scalar operations

- `popc{x:EI} : uint` - population count
- `popc_rand{x:EI} : uint` - population count; under valgrind, return a random value in the set of possible ones if the input is partly undefined
- `ctz{x:EI} : uint` - count trailing zeroes; if `x==0`, the result is undefined
- `clz{x:EI} : uint` - count leading zeroes; if `x==0`, the result is undefined
- `clzc{x:EI} : uint` - `width{EI}-clz{x}`; if `x==0`, the result is undefined
<!-- -->
- `zlow{n, x}` - zero low `n` bits of `x`
- `tail{n, x}` - keep low `n` bits of `x`
- `bit{n, x}` - get `n`-th bit of `x`
<!-- -->
- `lb{n}` - base-2 log of a constant power of two
- `base{b, x}` - convert tuple to number in little-endian base `b`
- `tree_fold{G, xs}` - fold tuple `xs` with a tree-like pattern with `G`

## Hints

- `rare{x:u1} : u1` - Expect that `x` is `0`. Returns `x`.
- `likely{x:u1} : u1` - Expect that `x` is `1`. Returns `x`.
- `assert{x:u1} : void` - assume `x` is `1`. If `x` is a compile-time constant, fails to compile if it's not `1`. If at runtime it is not `1`, undefined behavior happens (unless debug mode, via either `include './debug'` or CBQN-wide `DEBUG` mode)

## Type operations

Of the below functions which take & return vector types of the same width, they can instead take a vector value & reinterpret it to the result type.

- `w_n{E, w} == E2` - a scalar type with the quality of `E` and width `w`
- `w_d{E} == E2`, `w_h{E} == E2` - double/halve the width of a scalar type
- `n_d{V} == V2`, `n_h{V} == V2` - double/halve element count in vector (keeps the same element width)
- `el_d{V} == V2`, `el_h{V} == V2` - double/halve element width in vector (keeps the same count)
- `el_m{V} == V2`, `el_s{V} == V2` - vector with the same total width, but element width doubled (`el_m` - elements merged) or halved (`el_s` - elements split)
<!-- -->
- `ty_u{T} == TU` - unsigned integer version of the type
- `ty_s{T} == TS` - signed integer version of the type
- `ty_f{T} == TF` - float version of the type
- `re_el{E, V} == V2` - reinterpret vector to one with the specified element - `eltype{V2}==E and width{V2}==width{V}`
<!-- -->
- `minvalue{EI}`, `maxvalue{EI}` - smallest & biggest value of the specified type

## Arithmetic

Some may also support one scalar argument or arguments with different widths.

- `__neg{a}` - `-a`
- `__add{a:V, b:V} : V` - `a + b`
- `__sub{a:VI, b:VI} : VI` - `a - b`
- `__div{a:V, b:V} : V` - `a / b`
- `__mul{a:V, b:V} : V` - `a * b`
- `mulh{a:VI, b:VI} : VI` - high half of multiplication
<!-- -->
- `__eq`, `__ne`, `__ge`, `__gt`, `__le`, `__lt` - comparison; `{a:V, b:V} : mt{V}`
<!-- -->
- `__not{a:VI} : VI` - `~a`
- `__and{a:VI, b:VI} : VI` - `a&b`
- `__or{a:VI, b:VI} : VI` - `a|b`
- `__xor{a:VI, b:VI} : VI` - `a^b`
- `andnot{a:VI, b:VI} : VI` - `a&~b`
- `__shl{a:VI, b:TI} : VI` - `a<<b`; shift left
- `__shr{a:VS, b:TI} : VS` - `a>>b`; arithmetic shift right
- `__shr{a:VU, b:TI} : VU` - `a>>b`; logical shift right
<!-- -->
- `abs{a:V} : V` - absolute value
- `absu{a:VI} : ty_u{VI}` - absolute value, reinterpreted as unsigned
- `adds{a:VI, b:VI} : VI` - saturating add
- `subs{a:VI, b:VI} : VI` - saturating subtract
<!-- -->
- `rcpE{a:VF} : VF` - estimate of `1/a`; precision depends
- `rsqrtE{a:VF} : VF` - estimate of `sqrt{1/a}`; precision depends
<!-- -->
- `sqrt{a:VT} : VT` - square root
- `floor{a:VF} : VF` - floor
- `ceil{a:VF} : VF` - ceiling
- `max{a:V, b:V} : V` - minimum value; arch-specific behavior on NaNs
- `min{a:V, b:V} : V` - maximum value; arch-specific behavior on NaNs
<!-- -->
- `shl{U, a:V, n}` - shift vector elements within blocks of `U`
- `shr{U, a:V, n}` - shift vector elements within blocks of `U`
- `unord{a:VF, b:VF} : mt{VI}` - `(a==NaN) | (b==NaN)`
<!-- -->
- `and_bit_none{a:VI, b:VI} : u1` - `~any_bit{a & b}`

## Structural operations

- `broadcast{V, a:eltype{V}} : V` (`V**a`) - broadcast (splat) the scalar to all elements of the result
- `make{V, ...vs} : V` or `make{V, tup{...vs}}` - initialize a vector with the specified elements
- `iota{V} : V` - `make{V, range{vcount{V}}}`
- `extract{a:V, n} : eltype{V}` - extract the n-th element
- `insert{a:V, i, b:eltype{V}} : V` - a copy of `a` with the `i`-th element replaced with `b`
- `half{a:V, n} : count_half{V}` - extract either the low (`n==0`) or high (`n==1`) half of `a`
- `pair{a:V, b:V} : count_dbl{V}` - make a vector from halves
- `undef_promote{V2, a:V} : V` - promote `a` to a wider type, leaving the upper part undefined
- `vshl{a:V, b:V, n} : V` - `vshl{[0,1,2,3], [4,5,6,7], 1}` → `[1,2,3,4]`
- `sel{VI1, a:V, b:VI2} : V2` - shuffle `a` by indices `b` in `VI1`-long lanes; arch-specific behavior on out-of-bounds values

## Zip & unzip

All of these optionally accept a third argument of a constant `0` or `1` indicating which result element to give.

Some of them define variants which operate within 128-bit lanes, via a `128` postfix to the name (e.g. `zip128`).

- `zip{a:V, b:V} : tup{V, V}` - `zip{[0,1,2,3], [4,5,6,7]}` → `tup{[0,4,1,5], [2,6,3,7]}`
- `mzip{a:V, b:V} : tup{el_m{V}, el_m{V}}` - reinterpreted `zip{a, b}`
- `unzip{a:V, b:V} : tup{V, V}` - `unzip{[0,1,2,3], [4,5,6,7]}` → `tup{[0,2,4,6], [1,3,5,7]}`
- `pack{a:V, b:V} : tup{el_s{V}, el_s{V}}` - `unzip{el_s{V}~~a, el_s{V}~~b}`; i.e. low half of each element element in first result, high in second

## Mask stuff

Homogeneous definitions (i.e. ones with `hom` in their name) assume that each element in the mask type has all its bits equal.

- `all_hom{a:VI} : u1` - whether all elements are set
- `any_hom{a:VI} : u1` - whether any element is set
- `any_top{a:VI} : u1` - whether all elements have their top bit set
- `all_top{a:VI} : u1` - whether any element has its top bit set
- `any_bit{a:VI} : u1` - whether any bit in any element is set
- `all_bit{a:VI} : u1` - whether all bits in all elements is set
- `blend_hom{f:V, t:V, m:mt{V}} : V` - blend by `m`, setting to `f` where `0` and `t` where `1`
- `blend_top{f:V, t:V, m:V} : V` - blend by top bit of `m`
- `blend_bit{f:V, t:V, m:M} : V` - bitwise blend
- `hom_to_int{a:VI} : uint` - integer mask of whether each element is set (assumes each element has all its bits equal)
- `hom_to_int{...vs} : uint` - merged mask of `each{hom_to_int,vs}`
- `top_to_int{a:VI} : uint` - integer mask of the top bit of each element
- `hom_to_int_ext{a:VI} : tup{knum, uint}` - integer mask where each element is represented by `knum` bits (possibly more efficient to calculate than `hom_to_int`)
- `ctz_ext{tup{knum, uint}}` - count trailing zeroes from a result of `hom_to_int_ext`

## Load/store

Alignment requirements are target-specific, but at most one element.

For unaligned scalar loads & stores, `loadu` & `storeu` should be used.

- `loadu{p:*E} : E` - load scalar from unaligned memory
- `storeu{p:*E, a:E} : void` - store scalar to unaligned memory
- `load{[k]E, p:*E} : [k]E` - load full vector
- `store{p:*E, a:[k]E} : void` - store full vector
- `load{[k]E, p:*E, vl} : [k]E` - load first `vl` elements of vector (memory of upper ones won't be touched, and their values are unspecified)
- `store{p:*E, a:[k]E, vl} : void` - store first `vl` elements (upper ones won't be touched)
- `store_masked_hom{p:*E, m:mt{V}, a:V=[_]E}` - conditionally store elements based on mask; won't touch masked-off elements; `p` may also be `p:*V`
- `store_masked_top{p:*E, m:V, a:V=[_]E}` - conditionally store elements based on top bit of `m`; won't touch masked-off elements; `p` may also be `p:*V`
- `store_blended_hom` - `store_masked_hom` but may touch masked-off elements and thus be supported on more types
- `store_blended_top` - `store_masked_top` but may touch masked-off elements and thus be supported on more types

<!-- useless x86 defs for vector-width-aligned load/store: loada storea -->

## Conversion

For float conversions, the used rounding mode is unspecified.

- `narrow{eltype{V2}, a:V} : V2` - narrow elements of `a` to desired type.  
    For integer-to-integer conversion, completely undefined result if the values don't exactly fit.  
    Provides `vcount{V2} >= vcount{V}`.
- `widen{V2, a:V} : V2` - widen elements of `a` to desired type.  
    Discards elements of `a` past `vcount{V2}`.
- `cvt{eltype{V2}, a:V} : V2` - convert element type at the same width.  
    Requires `vcount{V}==vcount{V2} and elwidth{V}==elwidth{V2}`.

<!-- cvt2 -->


## x86-specific

- `mul32{a:VI, b:VI} : VI` - multiply, reading only low 32 bits
- `blend{L, a:V, b:V, m}` - blend `L`-sized blocks via the immediate
- `shuf{L, x:V, n} : V` - shuffle by immediate in `L`-sized lanes
- `packs128` - saturating pack 128-bit lanes (`packs`/`packus`) for 16-bit & 32-bit elements
- `packQQ` - `packs128` but also defined for 64-bit elements, assuming the high halves are zeroes
- `packs` - 128-bit `packs`/`packus`
- `shuf16Hi`, `shuf16Lo` - 16-bit shuffles with immediate
- `shufHalves`

## NEON-specific

### folds
- `fold_add{x:V} : eltype{V}`
- `fold_addw{x:V} : ty_dbl{eltype{V}}`
- `fold_max{x:V} : eltype{V}`
- `fold_min{x:V} : eltype{V}`
- `vfold{F, x:V} : eltype{V}` for F in `min`, `max`, `__add`

### arithmetic
- `addp` - add pairwise
- `addpw` - add pairwise, widening
- `addpwa` - add pairwise, widening; accumulate
- `addw`, `subw`, `mulw` - widening add/subtract/multiply
- `andnz` - element-wise (a&b)!=0? ~0 : 0
<!-- -->
- `clz` - count leading zeroes
- `cls` - count leading sign bits
- `copy_lane`
- `mla` - multiply-add
- `mls` - multiply-subtract
- `ornot` - a|~b
- `popc` - popcount of each byte
- `rbit` - reverse bits in bytes
- `rev` - reverse elements in lanes
- `shlm`, shrm - fun shifts with defaults
- `shrn` - narrowing shift right
- `trn1`, `trn2` - 2×2 transposes from pairs of elements across the two arguments

<!--
  widen_upper{x:V}
  narrowPair, narrowUpper
-->
## mask.singeli

- `mask_of_first{TU, n}` - get a mask of type `TU` whose first `n` elements are all `1`s, and the remaining are `0`s. `0≤n≤vcount{TU}`
- `mask_none` - a mask generator that matches all items
- `mask_first{n}` - a mask generator that enables the first `n` items
- `load_widen{p:*E, n, [k]W}` - load the `n`-th batch of `k` elements from `p`; if `W` isn't `E`, the result is sign- or zero-extended.
- `store_narrow{p:*E, n, x:[k]N, M}` - store equivalent of the `load_widen`; if `E` isn't `N`, `x` is narrowed via `narrow{}`

To test whether a mask object `M` is `mask_none` or `mask_first`, `M{0}` can be used - `mask_none{0}` is `0`, but `mask_first{n}{0}` is `1`.

### Loops

- `@for_masked{bulk}` - loop that generates its body twice, once with a `mask_none` mask, and once with a `mask_first{n}` one to handle the tail.

- `@for_mu{bulk, unr}` - masked & unrolled loop - generates its body three times (or two if `unr==1`) - once for unrolled main loop, once for the unrolling leftover, and once for the masked end.
  Unrolling is handled by passing a tuple of indices to process as the index variable (the tail generated bodies get a tuple of the one index)

- `@for_mu{bulk, unr, fromunr}` - `fromunr` is additionally ran after exiting the unrolled loop (such that you can have separate accumulators for the unrolled bit, and convert them to single-vector acccumulators for the tail).


Tuples can be used in the iterated variable list for various things:
```
p:*T - regular pointer
tup{VT,p:*T} - load_widen/store_narrow vector data
tup{'b',p:P} - load_bits (bits.singeli)
tup{'b',VT,p:P} - load_expand_bits (bits.singeli)
tup{'g',VT,p:*T} - gives a generator, used as g{} to load_widen and g{newValue} to store_narrow
tup{'g',p:*T} - the above, but without load support
'm' - get the mask object - either mask_none or some mask_first{n}
```

Stores via those will implicitly do a masked store when required.

Loads will load past the end, so `M in 'm'` must be used to mask off elements if they're used for something other than the above stores.

For `for_mu`, `M in 'm'` still gives a single generator, but it's only ever `mask_first{n}` when there's only one index.

Example usage for a loop that adds `u16` and bit boolean elements to a `u32` accumulator, early-exiting on overflow:
```
fn acc_u32_u16_bit_u1(r:*u32, x:*u16, bits:*u64, len:u64) : u1 = { # @for_masked
  def bulk = 8
  def VT = [8]u32
  
  @for_masked{bulk}(
    r in tup{VT, r},
    x in tup{VT, x},
    b in tup{'b', VT, bits},
    M in 'm'
    over i to len
  ) {
    # x, r, and b now have type VT
    r1:= r + x - b # subtracting b because it's all 1s for a one
    if (any_hom{M{r1 < r}}) return{0} # detect overflow
    r = r1 # will be mask-stored if needed
  }
  1
}
export {'acc_u32_u16_bit_u1', acc_u32_u16_bit_u1}

fn acc_u32_u16_bit_u2(r:*u32, x:*u16, bits:*u64, len:u64) : u1 = { # @for_mu, 2x unrolled
  def bulk = 8
  def VT = [8]u32
  
  @for_mu{bulk,2}(
    r in tup{'g', VT, r},
    x in tup{VT, x},
    b in tup{'b', VT, bits},
    M in 'm'
    over i to len
  ) {
    def r0 = r{}
    def r1 = each{-, each{+,r0,x}, b}
    if (any_hom{M{tree_fold{|, each{<,r1,r0}}}}) return{0}
    r{r1}
  }
  1
}
export {'acc_u32_u16_bit_u2', acc_u32_u16_bit_u2}
```

<!--

bitops.singeli
bmi2.singeli
  pdep
  pext
clmul.singeli
  clmul

-->
