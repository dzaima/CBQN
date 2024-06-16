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
    #if __GNUC__<=9
      #define _mm256_zextsi128_si256(X) _mm256_setr_m128i(X, _mm_setzero_si128())
    #endif
  #endif
  #if defined(__aarch64__)
    #define unpacked_vqtbl2q_s8(A,B,I)     vqtbl2q_s8((int8x16x2_t){A,B}, I)
    #define unpacked_vqtbl3q_s8(A,B,C,I)   vqtbl3q_s8((int8x16x3_t){A,B,C}, I)
    #define unpacked_vqtbl4q_s8(A,B,C,D,I) vqtbl4q_s8((int8x16x4_t){A,B,C,D}, I)
    #define unpacked_vqtbl2q_u8(A,B,I)     vqtbl2q_u8((uint8x16x2_t){A,B}, I)
    #define unpacked_vqtbl3q_u8(A,B,C,I)   vqtbl3q_u8((uint8x16x3_t){A,B,C}, I)
    #define unpacked_vqtbl4q_u8(A,B,C,D,I) vqtbl4q_u8((uint8x16x4_t){A,B,C,D}, I)
  #endif
  
  
  #define BCALL(N, X) N(b(X))
  #define interp_f64(X) b(X).f
  
  #define SINGELI_FILE0(X) #X
  #define SINGELI_FILE1(X) SINGELI_FILE0(X)
  #define si_unreachable() UD
#endif


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include SINGELI_FILE1(../../build/obj2/SINGELI_DIR/SINGELI_FILE.c)
#pragma GCC diagnostic pop

#undef SINGELI_FILE
