#if !defined(SI_C_DEFS)
  #define SI_C_DEFS
  
  #if EXACTLY_GCC && (defined(__x86_64) || defined(__amd64__)) // old gcc versions don't define some smaller-size loads/stores
    #include <xmmintrin.h>
    static __m128i custom_loadu_si32(void* p) { return (__m128i) _mm_load_ss(p); }
    static __m128i custom_loadu_si16(void* p) { return _mm_insert_epi16(_mm_setzero_si128(), *(i16*)p, 0); }
    static void custom_storeu_si32(void* p, __m128i x) { _mm_store_ss(p, _mm_castsi128_ps(x)); }
    static void custom_storeu_si16(void* p, __m128i x) { *(i16*)p = _mm_extract_epi16(x, 0); }
    #define _mm_loadu_si16 custom_loadu_si16
    #define _mm_loadu_si32 custom_loadu_si32
    #define _mm_storeu_si16 custom_storeu_si16
    #define _mm_storeu_si32 custom_storeu_si32
  #endif
  
  static void storeu_u64(u64* p, u64 v) { memcpy(p, &v, 8); }  static u64 loadu_u64(u64* p) { u64 v; memcpy(&v, p, 8); return v; }
  static void storeu_u32(u32* p, u32 v) { memcpy(p, &v, 4); }  static u32 loadu_u32(u32* p) { u32 v; memcpy(&v, p, 4); return v; }
  static void storeu_u16(u16* p, u16 v) { memcpy(p, &v, 2); }  static u16 loadu_u16(u16* p) { u16 v; memcpy(&v, p, 2); return v; }
  
  #define BCALL(N, X) N(b(X))
  #define interp_f64(X) b(X).f
  
  #define SINGELI_FILE0(X) #X
  #define SINGELI_FILE1(X) SINGELI_FILE0(X)
  #define si_unreachable() UD
#endif


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include SINGELI_FILE1(../../build/obj2/SINGELI_DIR/SINGELI_FILE.c)
#pragma GCC diagnostic pop

#undef SINGELI_FILE