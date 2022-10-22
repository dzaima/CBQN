#pragma once

// Radix sorting utilities
// These are leaky macros and assume counts are c0, c1,...
// which must be adjacent in memory

#define RDX_PRE(K) s##K=c##K[j]+=s##K
#define RDX_SUM_1(T)                                  T s0=0;                   for(usz j=0;j<256;j++) { RDX_PRE(0); }
#define RDX_SUM_2(T)  GRADE_UD(c1[0]=0;,)             T s0=0, s1=0;             for(usz j=0;j<256;j++) { RDX_PRE(0); RDX_PRE(1); }
#define RDX_SUM_4(T)  GRADE_UD(c1[0]=c2[0]=c3[0]=0;,) T s0=0, s1=0, s2=0, s3=0; for(usz j=0;j<256;j++) { RDX_PRE(0); RDX_PRE(1); RDX_PRE(2); RDX_PRE(3); }

#if SINGELI
extern void (*const avx2_scan_pluswrap_u8)(uint8_t* v0,uint8_t* v1,uint64_t v2,uint8_t v3);
extern void (*const avx2_scan_pluswrap_u32)(uint32_t* v0,uint32_t* v1,uint64_t v2,uint32_t v3);
#define RADIX_SUM_1_u8   avx2_scan_pluswrap_u8 (c0,c0,  256,0);
#define RADIX_SUM_2_u8   avx2_scan_pluswrap_u8 (c0,c0,2*256,0);
#define RADIX_SUM_2_u32  avx2_scan_pluswrap_u32(c0,c0,2*256,0);
#define RADIX_SUM_4_u8   avx2_scan_pluswrap_u8 (c0,c0,4*256,0);
#define RADIX_SUM_4_u32  avx2_scan_pluswrap_u32(c0,c0,4*256,0);
#else
#define RADIX_SUM_1_u8   RDX_SUM_1(u8)
#define RADIX_SUM_2_u8   RDX_SUM_2(u8)
#define RADIX_SUM_2_u32  RDX_SUM_2(u32)
#define RADIX_SUM_4_u8   RDX_SUM_4(u8)
#define RADIX_SUM_4_u32  RDX_SUM_4(u32)
#endif

#if SINGELI && !USZ_64
#define RADIX_SUM_1_usz  avx2_scan_pluswrap_u32(c0,c0,  256,0);
#define RADIX_SUM_2_usz  avx2_scan_pluswrap_u32(c0,c0,2*256,0);
#define RADIX_SUM_4_usz  avx2_scan_pluswrap_u32(c0,c0,4*256,0);
#else
#define RADIX_SUM_1_usz  RDX_SUM_1(usz)
#define RADIX_SUM_2_usz  RDX_SUM_2(usz)
#define RADIX_SUM_4_usz  RDX_SUM_4(usz)
#endif
