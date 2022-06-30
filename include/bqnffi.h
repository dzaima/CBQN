#include<stddef.h>
#include<stdint.h>
#include<stdbool.h>
/*
  FFI interface for interfacing with BQN implementations
  In general, the functions assume the argument(s) are of the expected type (if applicable), and cause undefined behavior if not
*/
typedef uint64_t BQNV;

#ifdef __cplusplus
extern "C" {
#endif

void bqn_init(void);

void bqn_free(BQNV v);
BQNV bqn_copy(BQNV v); // create a new BQNV which can be freed separately from `v`

double   bqn_toF64 (BQNV v); // includes bqn_free(v)
uint32_t bqn_toChar(BQNV v); // includes bqn_free(v)
double   bqn_readF64 (BQNV v); // doesn't include bqn_free(v)
uint32_t bqn_readChar(BQNV v); // doesn't include bqn_free(v)

int bqn_type(BQNV v); // equivalent of BQN •Type

// Invoke BQN function; f can be any BQN object other than modifiers
BQNV bqn_call1(BQNV f, BQNV x);
BQNV bqn_call2(BQNV f, BQNV w, BQNV x);

// Evaluate BQN code in a fresh environment
BQNV bqn_eval(BQNV src); // src must be a character vector
BQNV bqn_evalCStr(const char* str); // evaluates the null-terminated UTF8-encoded str; equal to `BQNV s = bqn_makeUTF8Str(str, strlen(str)); result = bqn_eval(s); bqn_free(s);`


// Read array data
size_t bqn_bound(BQNV a); // aka product of shape, aka "×´≢a"
size_t bqn_rank(BQNV a); // "=a"
void bqn_shape(BQNV a, size_t* buf); // writes bqn_rank(a) items in buf
BQNV bqn_pick(BQNV a, size_t pos); // pos⊑⥊a

// Functions for reading all elements of `a` into a pre-allocated buffer (whose count must be at least `bqn_bound(a)`).
// Defined if and only if the input `a` consists of elements whose types exactly fit in the wanted type.
// e.g. bqn_readObjArr is always defined (assuming `a` is an array and `buf` isn't null and has enough slots)
// but bqn_readI8Arr is illegal to invoke on `⟨1.2,3.4⟩` or `⟨200,201⟩`
void bqn_readI8Arr (BQNV a, int8_t*   buf);
void bqn_readI16Arr(BQNV a, int16_t*  buf);
void bqn_readI32Arr(BQNV a, int32_t*  buf);
void bqn_readF64Arr(BQNV a, double*   buf);
void bqn_readC8Arr (BQNV a, uint8_t*  buf);
void bqn_readC16Arr(BQNV a, uint16_t* buf);
void bqn_readC32Arr(BQNV a, uint32_t* buf);
void bqn_readObjArr(BQNV a, BQNV*     buf);

bool bqn_hasField(BQNV ns, BQNV name); // test if the namespace has the wanted field (result is unspecified if `name` isn't lowercase or has underscores)
BQNV bqn_getField(BQNV ns, BQNV name); // gives the value of the field with the requested name. Assumes the field exists in the namespace.

// Create objects
BQNV bqn_makeF64(double d);
BQNV bqn_makeChar(uint32_t c);
BQNV bqn_makeI8Arr (size_t rank, const size_t* shape, const int8_t*   data);
BQNV bqn_makeI16Arr(size_t rank, const size_t* shape, const int16_t*  data);
BQNV bqn_makeI32Arr(size_t rank, const size_t* shape, const int32_t*  data);
BQNV bqn_makeF64Arr(size_t rank, const size_t* shape, const double*   data);
BQNV bqn_makeC8Arr (size_t rank, const size_t* shape, const uint8_t*  data);
BQNV bqn_makeC16Arr(size_t rank, const size_t* shape, const uint16_t* data);
BQNV bqn_makeC32Arr(size_t rank, const size_t* shape, const uint32_t* data);
BQNV bqn_makeObjArr(size_t rank, const size_t* shape, const BQNV*     data); // frees the taken elements of data

BQNV bqn_makeI8Vec (size_t len, const int8_t*   data);
BQNV bqn_makeI16Vec(size_t len, const int16_t*  data);
BQNV bqn_makeI32Vec(size_t len, const int32_t*  data);
BQNV bqn_makeF64Vec(size_t len, const double*   data);
BQNV bqn_makeC8Vec (size_t len, const uint8_t*  data);
BQNV bqn_makeC16Vec(size_t len, const uint16_t* data);
BQNV bqn_makeC32Vec(size_t len, const uint32_t* data);
BQNV bqn_makeObjVec(size_t len, const BQNV*     data); // frees the taken elements of data
BQNV bqn_makeUTF8Str(size_t len, const char* str); // len is the number of chars in str; result item count will be lower if str contains non-ASCII characters

typedef BQNV (*bqn_boundFn1)(BQNV obj, BQNV x);
typedef BQNV (*bqn_boundFn2)(BQNV obj, BQNV w, BQNV x);

// When the result is called, the first arg given to `f` will be `obj`, and the rest will be the corresponding BQN arguments
BQNV bqn_makeBoundFn1(bqn_boundFn1 f, BQNV obj);
BQNV bqn_makeBoundFn2(bqn_boundFn2 f, BQNV obj);


// Direct (zero copy) array item access
typedef enum { elt_unk, elt_i8, elt_i16, elt_i32, elt_f64, elt_c8, elt_c16, elt_c32 } BQNElType; // note that more types may be added in the future
BQNElType bqn_directArrType(BQNV a);
// The functions below can only be used if if bqn_directArrType returns the exact equivalent type
// A valid implementation of bqn_directArrType would be to always return elt_unk, thus disallowing the use of direct access entirely
int8_t*   bqn_directI8 (BQNV a);
int16_t*  bqn_directI16(BQNV a);
int32_t*  bqn_directI32(BQNV a);
double*   bqn_directF64(BQNV a);
uint8_t*  bqn_directC8 (BQNV a);
uint16_t* bqn_directC16(BQNV a);
uint32_t* bqn_directC32(BQNV a);

#ifdef __cplusplus
}
#endif