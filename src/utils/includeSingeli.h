#if !defined(SI_C_DEFS)
  #define SI_C_DEFS
  
  #if __GNUC__ && !__clang__ && (defined(__x86_64) || defined(__amd64__)) // old gcc versions don't define _mm_loadu_si32 & _mm_storeu_si32
    #include <xmmintrin.h>
    static __m128i custom_loadu_si32(void* p) { return (__m128i) _mm_load_ss(p); }
    static void custom_storeu_si32(void* p, __m128i x) { _mm_store_ss(p, _mm_castsi128_ps(x)); }
    #define _mm_loadu_si32 custom_loadu_si32
    #define _mm_storeu_si32 custom_storeu_si32
  #endif
  
  #define BCALL(N, X) N(b(X))
  #define interp_f64(X) b(X).f
  
  #define SINGELI_FILE0(X) #X
  #define SINGELI_FILE1(X) SINGELI_FILE0(X)
  #define si_unreachable() UD
#endif


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#ifdef SINGELI_DIR
#include SINGELI_FILE1(../../build/obj2/SINGELI_DIR/SINGELI_FILE.c)
#else
#include SINGELI_FILE1(../singeli/gen/SINGELI_FILE.c)
#endif
#pragma GCC diagnostic pop

#undef SINGELI_FILE