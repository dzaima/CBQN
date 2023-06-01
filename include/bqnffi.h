#include<stddef.h>
#include<stdint.h>
#include<stdbool.h>
/*
  FFI interface for interfacing with BQN implementations.
  In general, the functions assume the argument(s) are of the expected type (if applicable), and cause undefined behavior if not.
  Functions, unless otherwise specified, don't free their arguments.
*/
typedef uint64_t BQNV;

#ifndef BQN_EXP
#define BQN_EXP
#endif

#ifdef __cplusplus
extern "C" {
#endif

BQN_EXP void bqn_init(void); // must be called at least once before any of the below functions are used; can safely be called multiple times

BQN_EXP void bqn_free(BQNV v); // free a previously obtained BQNV value, making it illegal to use it further
BQN_EXP BQNV bqn_copy(BQNV v); // create a BQNV with a value equivalent to `v`, which can be freed separately from `v`

BQN_EXP double   bqn_toF64 (BQNV v); // includes bqn_free(v)
BQN_EXP uint32_t bqn_toChar(BQNV v); // includes bqn_free(v)
BQN_EXP double   bqn_readF64 (BQNV v); // doesn't include bqn_free(v)
BQN_EXP uint32_t bqn_readChar(BQNV v); // doesn't include bqn_free(v)

BQN_EXP int bqn_type(BQNV v); // equivalent of BQN `•Type`

// Invoke BQN function; f can be any BQN object other than modifiers
BQN_EXP BQNV bqn_call1(BQNV f, BQNV x);
BQN_EXP BQNV bqn_call2(BQNV f, BQNV w, BQNV x);

// Evaluate BQN code in a fresh environment
BQN_EXP BQNV bqn_eval(BQNV src); // src must be a list of characters
BQN_EXP BQNV bqn_evalCStr(const char* str); // evaluates the null-terminated UTF8-encoded str; equal to `BQNV s = bqn_makeUTF8Str(strlen(str), str); result = bqn_eval(s); bqn_free(s);`


// Read array data
BQN_EXP size_t bqn_bound(BQNV a); // aka product of shape, aka `×´≢a`
BQN_EXP size_t bqn_rank(BQNV a); // `=a`
BQN_EXP void bqn_shape(BQNV a, size_t* buf); // writes bqn_rank(a) items in buf
BQN_EXP BQNV bqn_pick(BQNV a, size_t pos); // pos⊑⥊a

// Functions for reading all elements of an array `a` in deshape order into a pre-allocated buffer.
// Behavior is defined if and only if the input `a` consists of elements whose values losslessly fit in the wanted type.
// e.g. bqn_readObjArr is always defined (assuming `buf` is a valid buffer pointer, and `a` is an array),
// but bqn_readI8Arr is illegal to invoke on `⟨1.2,3.4⟩` or `⟨200,201⟩`, as those overflow a signed 8-bit integer.
// (internal representation of `a` does not matter - e.g. bqn_readI8Arr must succeed on all arrays whose value equals `1‿2‿3`)
BQN_EXP void bqn_readI8Arr (BQNV a, int8_t*   buf);
BQN_EXP void bqn_readI16Arr(BQNV a, int16_t*  buf);
BQN_EXP void bqn_readI32Arr(BQNV a, int32_t*  buf);
BQN_EXP void bqn_readF64Arr(BQNV a, double*   buf);
BQN_EXP void bqn_readC8Arr (BQNV a, uint8_t*  buf);
BQN_EXP void bqn_readC16Arr(BQNV a, uint16_t* buf);
BQN_EXP void bqn_readC32Arr(BQNV a, uint32_t* buf);
BQN_EXP void bqn_readObjArr(BQNV a, BQNV*     buf);

BQN_EXP bool bqn_hasField(BQNV ns, BQNV name); // test if the namespace has the wanted field
BQN_EXP BQNV bqn_getField(BQNV ns, BQNV name); // gives the value of the field with the requested name. Assumes the field exists in the namespace.
// The result of bqn_hasField is implementation-defined if `name` has underscores or isn't lowercase, but
// a result of `true` still should always result in a corresponding bqn_getField call succeeding.



// Create new BQN objects
BQN_EXP BQNV bqn_makeF64(double d);
BQN_EXP BQNV bqn_makeChar(uint32_t c);

// `shape` must point to `rank` elements; `data` must have `×´shape` readable items.
// The result will contain `data` in deshape order.
// The implementation will copy both buffers, so they can be temporary ones that are freed right after the call.
BQN_EXP BQNV bqn_makeI8Arr (size_t rank, const size_t* shape, const int8_t*   data);
BQN_EXP BQNV bqn_makeI16Arr(size_t rank, const size_t* shape, const int16_t*  data);
BQN_EXP BQNV bqn_makeI32Arr(size_t rank, const size_t* shape, const int32_t*  data);
BQN_EXP BQNV bqn_makeF64Arr(size_t rank, const size_t* shape, const double*   data);
BQN_EXP BQNV bqn_makeC8Arr (size_t rank, const size_t* shape, const uint8_t*  data);
BQN_EXP BQNV bqn_makeC16Arr(size_t rank, const size_t* shape, const uint16_t* data);
BQN_EXP BQNV bqn_makeC32Arr(size_t rank, const size_t* shape, const uint32_t* data);
BQN_EXP BQNV bqn_makeObjArr(size_t rank, const size_t* shape, const BQNV*     data); // frees the taken elements of data

// Create a vector with length `len`; equivalent to the corresponding bqn_make*Arr function with `rank` 1, and `shape` being a pointer to `len`
BQN_EXP BQNV bqn_makeI8Vec (size_t len, const int8_t*   data);
BQN_EXP BQNV bqn_makeI16Vec(size_t len, const int16_t*  data);
BQN_EXP BQNV bqn_makeI32Vec(size_t len, const int32_t*  data);
BQN_EXP BQNV bqn_makeF64Vec(size_t len, const double*   data);
BQN_EXP BQNV bqn_makeC8Vec (size_t len, const uint8_t*  data);
BQN_EXP BQNV bqn_makeC16Vec(size_t len, const uint16_t* data);
BQN_EXP BQNV bqn_makeC32Vec(size_t len, const uint32_t* data);
BQN_EXP BQNV bqn_makeObjVec(size_t len, const BQNV*     data); // frees the taken elements of data
BQN_EXP BQNV bqn_makeUTF8Str(size_t len, const char* str); // len is the number of bytes (`char`s) in `str`; result item count will be smaller than `len` if `str` contains non-ASCII characters



typedef BQNV (*bqn_boundFn1)(BQNV obj, BQNV x);
typedef BQNV (*bqn_boundFn2)(BQNV obj, BQNV w, BQNV x);

// Creates a BQN function object, which, when called, calls `f` with the first argument being (a copy of) the original `obj` argument,
// and further arguments being the passed ones. The provided `f` is responsible for freeing all its arguments when they're done being used.
// Behavior when calling bqn_makeBoundFn1's result dyadically, or bqn_makeBoundFn2's - monadically - is undefined.
BQN_EXP BQNV bqn_makeBoundFn1(bqn_boundFn1 f, BQNV obj);
BQN_EXP BQNV bqn_makeBoundFn2(bqn_boundFn2 f, BQNV obj);



// Direct (zero-copy) array item access
typedef enum { elt_unk, elt_i8, elt_i16, elt_i32, elt_f64, elt_c8, elt_c16, elt_c32 } BQNElType; // note that more types may be added in the future

BQN_EXP BQNElType bqn_directArrType(BQNV a);
// Note that a valid implementation of bqn_directArrType would be to always return elt_unk, thus disallowing the use of direct access entirely.

// The functions below can only be used only if bqn_directArrType returns the exact type.
// Mutating the result, or reading from it after `a` is freed, results in undefined behavior.
// Doing other FFI invocations between a direct access request and the last read from the result is currently allowed in CBQN,
// but that may not be true for an implementation which has a compacting garbage collector without pinning.
BQN_EXP const int8_t*   bqn_directI8 (BQNV a);
BQN_EXP const int16_t*  bqn_directI16(BQNV a);
BQN_EXP const int32_t*  bqn_directI32(BQNV a);
BQN_EXP const double*   bqn_directF64(BQNV a);
BQN_EXP const uint8_t*  bqn_directC8 (BQNV a);
BQN_EXP const uint16_t* bqn_directC16(BQNV a);
BQN_EXP const uint32_t* bqn_directC32(BQNV a);

#ifdef __cplusplus
}
#endif