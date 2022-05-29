#include<stddef.h>
#include<stdint.h>
#include<stdbool.h>

typedef uint64_t BQNV;

#ifdef __cplusplus
extern "C" {
#endif

void bqn_free(BQNV v);

double   bqn_toF64 (BQNV v); // includes bqn_free(v)
uint32_t bqn_toChar(BQNV v); // includes bqn_free(v)
double   bqn_readF64 (BQNV v); // doesn't include bqn_free(v)
uint32_t bqn_readChar(BQNV v); // doesn't include bqn_free(v)

int bqn_type(BQNV v); // equivalent of BQN •Type

// invoke BQN function
BQNV bqn_call1(BQNV f, BQNV x);
BQNV bqn_call2(BQNV f, BQNV w, BQNV x);

// evaluate BQN code in a fresh environment
BQNV bqn_eval(BQNV src);
BQNV bqn_evalCStr(char* str); // evaluates the null-terminated UTF8-encoded str; equal to `BQNV s = bqn_makeUTF8Str(str, strlen(str)); result = bqn_eval(s); bqn_free(s);`


// read array data
size_t bqn_bound(BQNV a); // aka product of shape, ×´≢a
size_t bqn_rank(BQNV a);
void bqn_shape(BQNV a, size_t* buf); // writes bqn_rank(a) items in buf
BQNV bqn_pick(BQNV a, size_t pos);

// read all elements of `a` into the specified buffer
void bqn_readI8Arr (BQNV a, int8_t*   buf);
void bqn_readI16Arr(BQNV a, int16_t*  buf);
void bqn_readI32Arr(BQNV a, int32_t*  buf);
void bqn_readF64Arr(BQNV a, double*   buf);
void bqn_readC8Arr (BQNV a, uint8_t*  buf);
void bqn_readC16Arr(BQNV a, uint16_t* buf);
void bqn_readC32Arr(BQNV a, uint32_t* buf);
void bqn_readObjArr(BQNV a, BQNV*     buf);


// create objects
BQNV bqn_makeF64(double d);
BQNV bqn_makeChar(uint32_t c);
BQNV bqn_makeI8Arr (size_t rank, size_t* shape, int8_t*   data);
BQNV bqn_makeI16Arr(size_t rank, size_t* shape, int16_t*  data);
BQNV bqn_makeI32Arr(size_t rank, size_t* shape, int32_t*  data);
BQNV bqn_makeF64Arr(size_t rank, size_t* shape, double*   data);
BQNV bqn_makeC8Arr (size_t rank, size_t* shape, uint8_t*  data);
BQNV bqn_makeC16Arr(size_t rank, size_t* shape, uint16_t* data);
BQNV bqn_makeC32Arr(size_t rank, size_t* shape, uint32_t* data);
BQNV bqn_makeObjArr(size_t rank, size_t* shape, BQNV*     data); // frees the taken elements of data

BQNV bqn_makeI8Vec (size_t len, int8_t*   data);
BQNV bqn_makeI16Vec(size_t len, int16_t*  data);
BQNV bqn_makeI32Vec(size_t len, int32_t*  data);
BQNV bqn_makeF64Vec(size_t len, double*   data);
BQNV bqn_makeC8Vec (size_t len, uint8_t*  data);
BQNV bqn_makeC16Vec(size_t len, uint16_t* data);
BQNV bqn_makeC32Vec(size_t len, uint32_t* data);
BQNV bqn_makeObjVec(size_t len, BQNV*     data); // frees the taken elements of data
BQNV bqn_makeUTF8Str(size_t len, char* str);

typedef BQNV (*bqn_boundFn1)(BQNV obj, BQNV x);
typedef BQNV (*bqn_boundFn2)(BQNV obj, BQNV w, BQNV x);

// when called, 1st arg to `f` will be `obj`
BQNV bqn_makeBoundFn1(bqn_boundFn1 f, BQNV obj);
BQNV bqn_makeBoundFn2(bqn_boundFn2 f, BQNV obj);


// direct (zero copy) array item access
typedef enum { elt_unk, elt_i8, elt_i16, elt_i32, elt_f64, elt_c8, elt_c16, elt_c32 } BQNElType; // note that more types may be added in the future
BQNElType bqn_directArrType(BQNV a);
// can only use the functions below if bqn_elType returns the corresponding type
// a valid implementation of bqn_elType would be to always return elt_unk, thus disallowing the use of direct access entirely
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