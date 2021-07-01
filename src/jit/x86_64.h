/* taken from I: https://github.com/mlochbaum/ILanguage/blob/master/x86_64.h
ISC License

Copyright (c) 2016, Marshall Lochbaum <mwlochbaum@gmail.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#pragma once
#include "../core.h"
#include "../utils/talloc.h"

//       V - volatile (overwritten by calls)
// 0 rax V result
// 1 rcx V arg 3
// 2 rdx V arg 2
// 3 rbx  
// 4 rsp   stack
// 5 rbp   base
// 6 rsi V arg 1
// 7 rdi V arg 0
// 8 r8  V arg 4
// 9 r9  V arg 5
// . r10 V
// . r11 V
// . r12
// . r13
// . r14
// . r15

typedef u8 U;
typedef u8 UC;
typedef u8 Reg;
#define R_RES 0 // rax
#define R_SP 4 // rsp
#define R_BP 5 // rbp
// aregument registers
#define R_A0 7 // rdi
#define R_A1 6 // rsi
#define R_A2 2 // rdx
#define R_A3 1 // rcx; may be used by non-immediate shifts
#define R_A4 8 // r8
#define R_A5 9 // r9
// volatile, aka caller-saved registers (like argument registers, but never actually arguments)
#define R_V0 10
#define R_V1 11
// non-volatile/callee-saved/preserved registers
#define R_P0  3 // rbx
#define R_P1 14 // r14
#define R_P2 15 // r15
#define R_P3 13 // r13
#define R_P4 12 // r12


#define ALLOC_ASM_ARR(N) TStack* b_o = (TStack*)mm_allocN(sizeof(TStack)+(N), t_temp); b_o->size=0; b_o->cap=(N)
#define ALLOC_ASM(N) ALLOC_ASM_ARR(N); TSALLOC(u32, b_r, 64);
#define GET_ASM() u8* bin = b_o->data;
#define AADD(P,N) b_o=asm_add(b_o, P, N)
#define ASM_SIZE (b_o->size)
#define FREE_ASM() mm_free((Value*)b_o); TSFREE(b_r);
#define ASM_WRITE(P) {                 \
  memcpy(binEx, bin, sz);              \
  u64 relAm = TSSIZE(b_r);             \
  for (u64 i = 0; i < relAm; i++) {    \
    u8* ins = binEx+b_r[i];            \
    u32 o = readBytes4(ins);           \
    u32 n = o-(u32)(u64)ins;           \
    memcpy(ins, (u8[]){BYTES4(n)}, 4); \
  }                                    \
}                                      \

static NOINLINE TStack* asm_reserve(TStack* o) {
  u64 osz = o->size;
  u64 ncap = o->cap*2;
  ALLOC_ASM_ARR(ncap);
  memcpy(b_o->data, o->data, osz);
  b_o->size = osz;
  mm_free((Value*)o);
  return b_o;
}
static inline TStack* asm_r(TStack* o) {
  if (o->size+16>o->cap) return asm_reserve(o);
  return o;
}

static inline void asm_1(TStack* o, i8 v) {
  o->data[o->size++] = v;
}
static inline void asm_4(TStack* o, i32 v) { int size = o->size;
  // o->data[size+0] = (v    ) & 0xff; o->data[size+1] = (v>> 8) & 0xff;
  // o->data[size+2] = (v>>16) & 0xff; o->data[size+3] = (v>>24) & 0xff;
  *(u32*)(o->data+size) = v; // clang for whatever reason fails to optimize the above when it's inlined
  o->size = size+4;
}
static inline void asm_8(TStack* o, i64 v) { int size = o->size; u8* p = o->data;
  // p[size+0] = (u8)(v    ); p[size+1] = (u8)(v>> 8); p[size+2] = (u8)(v>>16); p[size+3] = (u8)(v>>24);
  // p[size+4] = (u8)(v>>32); p[size+5] = (u8)(v>>40); p[size+6] = (u8)(v>>48); p[size+7] = (u8)(v>>56);
  *(u64*)(p+size) = v;
  o->size+= 8;
}
static inline void asm_a(TStack* o, u64 len, u8 v[]) {
  memcpy(o->data, v, len);
  o->size+= len;
}
#define ASM1(X) asm_1(b_o,X)
#define ASM4(X) asm_4(b_o,X)
#define ASM8(X) asm_8(b_o,X)
#define ASMA(A) { u8 t[]=A; asm_a(b_o,sizeof(t),t); }



// Ignore leading 0x40
// TODO Doesn't drop 0xF2 0x40, etc.
#define ASM_RAW(A, OP) do { UC aa[] = OP; u8 off=aa[0]==0x40; AADD(aa+off, sizeof(aa)-off); } while(0)

// Instructions
#define A_0REG(O,I) ((((I)&7)<<3) + ((O)&7))
#define A_REG(O,I) (0xC0 + A_0REG(O,I))
#define REX0(O,I) (((O)>7)+(((I)>7)<<2))
#define REX0_3(O,I,E) (REX0(O,I)+(((E)>7)<<1))
#define REX8(O,I) (0x48 + REX0(O,I))
#define REX4(O,I) (0x40 + REX0(O,I))
#define REX8_3(O,I,E) (0x48 + REX0_3(O,I,E))
#define REX4_3(O,I,E) (0x40 + REX0_3(O,I,E))
#define REX1(O,I) 0x40 , (0x40 + REX0(O,I))

// #define MOV(O,I) {REX8(O,I),0x89,A_REG(O,I)}
// #define ADD(O,I)  {REX8(O,I),0x01,A_REG(O,I)}
// #define SUB(O,I)  {REX8(O,I),0x29,A_REG(O,I)}
// #define XOR(O,I)  {REX8(O,I),0x31,A_REG(O,I)}
#define TEST(O,I) {REX8(O,I),0x85,A_REG(O,I)}
#define IMUL(I,O) {REX8(O,I),0x0F,0xAF,A_REG(O,I)}
// #define CMP(O,I)  {REX8(O,I),0x39,A_REG(O,I)}
#define NEG(I,O)  {REX8(O,0),0xF7,A_REG(O,3)}

#define CMP4_MI(O,I) {REX4(O,0),0x81,A_0REG(O,7),BYTES4(I)}

// TODO REX for sil, etc
#define MOVZX4(I,O) {REX4(O,I),0x0F,0xB6,A_REG(O,I)}

// #define ADDI4(O,I) {REX8(O,0),0x81,A_REG(O,0),BYTES4(I)}
// #define SUBI4(O,I) {REX8(O,0),0x81,A_REG(O,5),BYTES4(I)}

// #define ADDI1(O,I) {REX8(O,0),0x83,A_REG(O,0),(U)(I)}
// #define SUBI1(O,I) {REX8(O,0),0x83,A_REG(O,5),(U)(I)}
// #define SHLI1(O,I) {REX8(O,0),0xC1,A_REG(O,4),(U)(I)}
// #define SHRI1(O,I) {REX8(O,0),0xC1,A_REG(O,5),(U)(I)}

// #define XOR4(O,I)  {REX4(O,I),0x31,A_REG(O,I)}

#define LEA1(O,A,B)  {REX8_3(A,O,B),0x8D,A_0REG(4,O),A_0REG(A,B)}

#define CQO(O,I)   {0x48,0x99}
#define IDIV(O,I)  {REX8(O,0),0xF7,A_REG(O,7)}
#define REG_IDIV_0  R_RES
#define REG_IDIV_1  R_A2

#define CVTSI2SD(O,I)  {0xF2,REX8(I,O),0x0F,0x2A,A_REG(I,O)}
#define CVTTSD2SI(O,I) {0xF2,REX8(I,O),0x0F,0x2C,A_REG(I,O)}
#define MOVQ(O,I)  {0x66,REX8(O,I),0x0F,0x7E,A_REG(O,I)}

#define CVTSI2SD_RM(O,I,OFF) {0xF2,REX8(I,O),0x0F,0x2A,0x40+A_0REG(I,O),OFF}
#define CVTSI2SD_RM0(O,I)    {0xF2,REX8(I,O),0x0F,0x2A,A_0REG(I,O)}

#define MOVSD(O,I)  {0xF2,REX4(I,O),0x0F,0x10,A_REG(I,O)}
#define ADDSD(O,I)  {0xF2,REX4(I,O),0x0F,0x58,A_REG(I,O)}
#define MULSD(O,I)  {0xF2,REX4(I,O),0x0F,0x59,A_REG(I,O)}
#define SUBSD(O,I)  {0xF2,REX4(I,O),0x0F,0x5C,A_REG(I,O)}
#define MINSD(O,I)  {0xF2,REX4(I,O),0x0F,0x5D,A_REG(I,O)}
#define DIVSD(O,I)  {0xF2,REX4(I,O),0x0F,0x5E,A_REG(I,O)}
#define MAXSD(O,I)  {0xF2,REX4(I,O),0x0F,0x5F,A_REG(I,O)}
#define SQRTSD(O,I) {0xF2,REX4(I,O),0x0F,0x51,A_REG(I,O)}
#define PXOR(O,I)   {0x66,REX4(I,O),0x0F,0xEF,A_REG(I,O)}

#define UCOMISD(O,I)  {0x66,REX4(I,O),0x0F,0x2E,A_REG(I,O)}

// #define PUSH(O,I) {REX4(O,0),0x50+((O)&7)}
// #define POP(O,I)  {REX4(O,0),0x58+((O)&7)}

#define BYTES4(I) ((UC)(I)),((UC)((I)>>8)),((UC)((I)>>16)),((UC)((I)>>24))
#define BYTES8(I) BYTES4(I) ,((UC)((I)>>32)),((UC)((I)>>40)) \
                            ,((UC)((I)>>48)),((UC)((I)>>56))

// #define MOV_MR(O,I,OFF) {REX8(O,I),0x89,0x40+A_0REG(O,I),OFF}
// #define MOV_MR0(O,I)    {REX8(O,I),0x89,A_0REG(O,I)} // TODO is broken on (12,14)
// #define MOV_RM(I,O,OFF) {REX8(O,I),0x8B,0x40+A_0REG(O,I),OFF}
// #define MOV_RM0(I,O)    {REX8(O,I),0x8B,A_0REG(O,I)}
// #define MOV_RI(O,I)     {REX8(O,0),0xB8+((O)&7), BYTES8(I)}
// #define LEAo1(I,O,OFF)  {REX8(O,I),0x8D,0x40+A_0REG(O,I),OFF}

#define MOV4_MI(O,I,OFF) {REX4(O,0),0xC7,0x40+A_0REG(O,0),OFF,BYTES4(I)}
#define MOV4_RI(O,I)     {REX4(O,0),0xB8+((O)&7) , BYTES4(I)}
// #define MOV4_MR(O,I,OFF) {REX4(O,I),0x89,0x40+A_0REG(O,I),OFF}
// #define MOV4_MR0(O,I)    {REX4(O,I),0x89,A_0REG(O,I)}
// #define MOV4_RM(I,O,OFF) {REX4(O,I),0x8B,0x40+A_0REG(O,I),OFF}
// #define MOV4_RM0(I,O)    {REX4(O,I),0x8B,A_0REG(O,I)}

#define MOV1_MR(O,I,OFF) {REX4(O,I),0x88,0x40+A_0REG(O,I),OFF}
#define MOV1_MR0(O,I)    {REX4(O,I),0x88,A_0REG(O,I)}
#define MOV1_RM(I,O,OFF) {REX4(O,I),0x0F,0xB6,0x40+A_0REG(O,I),OFF}
#define MOV1_RM0(I,O)    {REX4(O,I),0x0F,0xB6,A_0REG(O,I)}

#define MOVSD_MR(O,I,OFF) {0xF2,REX4(O,I),0x0F,0x11,0x40+A_0REG(O,I),OFF}
#define MOVSD_MR0(O,I)    {0xF2,REX4(O,I),0x0F,0x11,A_0REG(O,I)}
#define MOVSD_RM(I,O,OFF) {0xF2,REX4(O,I),0x0F,0x10,0x40+A_0REG(O,I),OFF}
#define MOVSD_RM0(I,O)    {0xF2,REX4(O,I),0x0F,0x10,A_0REG(O,I)}

#define ASM_MOV_PRE(A,T)  if ((T)==R_t) ASM_RAW(A, {0xF2})
#define MOV1_MR_STUB(O,I,E)   {REX4_3(O,I,E),0x88}
#define MOV1_RM_STUB(I,O,E)   {REX4_3(O,I,E),0x0F,0xB6}
#define MOV_MR_STUB(O,I,E)    {REX8_3(O,I,E),0x89}
#define MOV_RM_STUB(I,O,E)    {REX8_3(O,I,E),0x8B}
#define MOVSD_MR_STUB(O,I,E)  {REX4_3(O,I,E),0x0F,0x11}
#define MOVSD_RM_STUB(I,O,E)  {REX4_3(O,I,E),0x0F,0x10}
#define AD_MR0(O,I)      {A_0REG(O,I)}
#define AD_RM0(I,O)      AD_MR0(O,I)
#define AD_MR(O,I,OFF)   {0x40+A_0REG(O,I),OFF}
#define AD_RM(I,O,OFF)   AD_MR(O,I,OFF)
#define AD_MRRS(O,A,B,S) {A_0REG(4,A),(((S)&3)<<6)+A_0REG(O,B)}
#define AD_RMRS(A,O,B,S) AD_MRRS(O,A,B,S)

// #define CALL(O,I) {REX4(O,0),0xFF,A_REG(O,2)}
// #define CALLI(I,O) {0xE8,BYTES4((u32)(I))}

#define LOOP(O,I) {0xE2,((UC)(O)-2)}
#define REG_LOOP 1

#define C_O   0x0
#define C_NO  0x1
#define C_B   0x2
#define C_AE  0x3
#define C_E   0x4
#define C_NE  0x5
#define C_BE  0x6
#define C_A   0x7
#define C_S   0x8
#define C_NS  0x9
#define C_P   0xA
#define C_NP  0xB
#define C_L   0xC
#define C_GE  0xD
#define C_LE  0xE
#define C_G   0xF

#define JX(O,C)    {0x70+(C),(UC)(O)-2}
#define J4X(O,C)   {0x0F,0x80+(C),BYTES4((O)-6)}
#define SETX(O,C)  {REX1(O,0),0x0F,0x90+(C),A_REG(O,0)}
#define CMOVX(I,O,C)  {REX8(O,I),0x0F,0x40+(C),A_REG(O,I)}

// #define JO(O,I)  {0x70,((UC)(O)-2)}
// #define JNO(O,I) {0x71,((UC)(O)-2)}
// #define JB(O,I)  {0x72,((UC)(O)-2)}
// #define JAE(O,I) {0x73,((UC)(O)-2)}
// #define JE(O,I)  {0x74,((UC)(O)-2)}
// #define JNE(O,I) {0x75,((UC)(O)-2)}
// #define JBE(O,I) {0x76,((UC)(O)-2)}
// #define JA(O,I)  {0x77,((UC)(O)-2)}
// #define JS(O,I)  {0x78,((UC)(O)-2)}
// #define JNS(O,I) {0x79,((UC)(O)-2)}
// #define JP(O,I)  {0x7A,((UC)(O)-2)}
// #define JNP(O,I) {0x7B,((UC)(O)-2)}
// #define JL(O,I)  {0x7C,((UC)(O)-2)}
// #define JGE(O,I) {0x7D,((UC)(O)-2)}
// #define JLE(O,I) {0x7E,((UC)(O)-2)}
// #define JG(O,I)  {0x7F,((UC)(O)-2)}

#define JMP(O,I)  {0xEB,((UC)(O)-2)}
#define JMP4(O,I) {0xE9,BYTES4((O)-5)}

#define SETO(O,I)  {REX1(O,0),0x0F,0x90,A_REG(O,0)}
#define SETNO(O,I) {REX1(O,0),0x0F,0x91,A_REG(O,0)}
#define SETB(O,I)  {REX1(O,0),0x0F,0x92,A_REG(O,0)}
#define SETAE(O,I) {REX1(O,0),0x0F,0x93,A_REG(O,0)}
#define SETE(O,I)  {REX1(O,0),0x0F,0x94,A_REG(O,0)}
#define SETNE(O,I) {REX1(O,0),0x0F,0x95,A_REG(O,0)}
#define SETBE(O,I) {REX1(O,0),0x0F,0x96,A_REG(O,0)}
#define SETA(O,I)  {REX1(O,0),0x0F,0x97,A_REG(O,0)}
#define SETS(O,I)  {REX1(O,0),0x0F,0x98,A_REG(O,0)}
#define SETNS(O,I) {REX1(O,0),0x0F,0x99,A_REG(O,0)}
#define SETP(O,I)  {REX1(O,0),0x0F,0x9A,A_REG(O,0)}
#define SETNP(O,I) {REX1(O,0),0x0F,0x9B,A_REG(O,0)}
#define SETL(O,I)  {REX1(O,0),0x0F,0x9C,A_REG(O,0)}
#define SETGE(O,I) {REX1(O,0),0x0F,0x9D,A_REG(O,0)}
#define SETLE(O,I) {REX1(O,0),0x0F,0x9E,A_REG(O,0)}
#define SETG(O,I)  {REX1(O,0),0x0F,0x9F,A_REG(O,0)}

#define CMOVO(I,O)  {REX8(O,I),0x0F,0x40,A_REG(O,I)}
#define CMOVNO(I,O) {REX8(O,I),0x0F,0x41,A_REG(O,I)}
#define CMOVB(I,O)  {REX8(O,I),0x0F,0x42,A_REG(O,I)}
#define CMOVAE(I,O) {REX8(O,I),0x0F,0x43,A_REG(O,I)}
#define CMOVE(I,O)  {REX8(O,I),0x0F,0x44,A_REG(O,I)}
#define CMOVNE(I,O) {REX8(O,I),0x0F,0x45,A_REG(O,I)}
#define CMOVBE(I,O) {REX8(O,I),0x0F,0x46,A_REG(O,I)}
#define CMOVA(I,O)  {REX8(O,I),0x0F,0x47,A_REG(O,I)}
#define CMOVS(I,O)  {REX8(O,I),0x0F,0x48,A_REG(O,I)}
#define CMOVNS(I,O) {REX8(O,I),0x0F,0x49,A_REG(O,I)}
#define CMOVP(I,O)  {REX8(O,I),0x0F,0x4A,A_REG(O,I)}
#define CMOVNP(I,O) {REX8(O,I),0x0F,0x4B,A_REG(O,I)}
#define CMOVL(I,O)  {REX8(O,I),0x0F,0x4C,A_REG(O,I)}
#define CMOVGE(I,O) {REX8(O,I),0x0F,0x4D,A_REG(O,I)}
#define CMOVLE(I,O) {REX8(O,I),0x0F,0x4E,A_REG(O,I)}
#define CMOVG(I,O)  {REX8(O,I),0x0F,0x4F,A_REG(O,I)}

// #define RET {0xC3}












#define ASM(INS, O, I) ASM_RAW(b_o, INS(O, I))
#define ASM3(INS, O, A, B) ASM_RAW(b_o, INS(O, A, B))
// #define ADDI(O, I) { i32 v=(i32)(I); if(v) { if(v==(i8)v) ASM(ADDI1,O,v); else ASM(ADDI4,O,v); } } // I must fit in i32
// #define SUBI(O, I) { i32 v=(i32)(I); if(v) { if(v==(i8)v) ASM(SUBI1,O,v); else ASM(SUBI4,O,v); } } // I must fit in i32
// #define IMM(O, I) b_o=imm_impl(b_o, O, (u64)(I));
// static NOINLINE TStack* imm_impl(TStack* b_o, Reg o, u64 i) {
//   if(i==0) {
//     ASM(XOR,o,o);
//   } else if(i>=0 & i<(1ULL<<32)) {
//     ASM(MOV4_RI,o,i);
//   } else {
//     ASM(MOV_RI,o,i);
//   }
//   return b_o;
// }



// #define nREX0_3(O,I,E) (REX0(O,I)+(((E)>7)<<1))
// #define nREX8_3(O,I,E) (0x48 + nREX0_3(O,I,E))
// #define nREX4_3(O,I,E) (0x40 + nREX0_3(O,I,E))

// #define nREX1(O,I) 0x40 , (0x40 + REX0(O,I))
#define nREX8(O,I) ASM1(0x48 + REX0(O,I))
#define nREX4(O,I) {u8 t=REX0(O,I); if(t) ASM1(0x40+t); }
#define CHK4(O) (((O)&7)==4)
#define CHK5(O) (((O)&7)==5)
#define CHK45(O) (((O)&7)>>1==2)

// ModR/M mess
#define MRM(O,I) ASM1((((I)&7)<<3) + ((O)&7) + 0x40*CHK5(O)); if(CHK45(O)) ASM1(CHK4(O)?0x24:0);
#define MRMo(O,I,OFF) { i64 t=OFF;                       \
  bool b1 = t||CHK5(O); bool b4 = t!=(i8)t;              \
  ASM1((((I)&7)<<3) + ((O)&7) + (b1? b4?0x80:0x40 : 0)); \
  if(CHK4(O)) ASM1(0x24);                                \
  if(b4) ASM4(t); else if (b1) ASM1(t);                  \
}
#define MRMp(OFF,I) ASM1((((I)&7)<<3) + 5); ASM4(OFF); // offset to rip
#define nA_0REG(O,I) (((I)&7)<<3) + ((O)&7)
#define nA_REG(O,I) ASM1(0xC0 + nA_0REG(O,I)) // aka MRM immediate

#define ASMI(N, ...) static NOINLINE void i##N(TStack* b_o, __VA_ARGS__)
#define  AC1(N,A    ) (i##N(b_o=asm_r(b_o),A    ))
#define  AC2(N,A,B  ) (i##N(b_o=asm_r(b_o),A,B  ))
#define  AC3(N,A,B,C) (i##N(b_o=asm_r(b_o),A,B,C))

// meaning of lowercase after basic instr name:
//   'r' means the corresponding argument is the direct register contents, 'm' - that it's dereferenced, 'p' - offset to rip; if all are 'r' or they don't have other options, they can be omitted
//   add 'o' for an offset argument (e.g. the 5 in 'mov rax, [rdi+5]'), 'i' for an immediate (e.g. 5 in 'add r12,5', or in 'lea rax, [rdi+5]')
#define INC4mo(O,IMM) AC2(INC4mo,O,IMM)
#define INC8mo(O,IMM) AC2(INC8mo,O,IMM)
#define DEC4mo(O,IMM) AC2(DEC4mo,O,IMM)
#define DEC8mo(O,IMM) AC2(DEC8mo,O,IMM)

#define MOV(O,I) AC2(MOV,O,I)
#define MOV4(O,I) AC2(MOV4,O,I)
#define ADD(O,IMM) AC2(ADD,O,IMM)
#define SUB(O,IMM) AC2(SUB,O,IMM)
#define XOR(O,IMM) AC2(XOR,O,IMM)
#define  OR(O,IMM) AC2( OR,O,IMM)
#define AND(O,IMM) AC2(AND,O,IMM)
#define CMP(O,IMM) AC2(CMP,O,IMM)
#define ADDi(O,IMM) AC2(ADDi,O,IMM)
#define SUBi(O,IMM) AC2(SUBi,O,IMM)
#define SHLi(O,IMM) AC2(SHLi,O,IMM)
#define SHRi(O,IMM) AC2(SHRi,O,IMM)
#define MOVi(O,IMM) AC2(MOVi,O,IMM)
#define MOVi1l(O,IMM) AC2(MOVi1l,O,IMM)
#define ADD4mi(O,IMM) AC2(ADD4mi,O,IMM) // ADD4mi(o,1) is an alternative for INC4mo(o,0)

#define MOV8rm(O,I) AC2(MOV8rm,O,I) // O ← *(u64*)(nullptr + I)
#define MOV8mr(I,O) AC2(MOV8mr,I,O) // *(u64*)(nullptr + I) ← O
#define MOV4rm(O,I) AC2(MOV4rm,O,I) // O ← *(u32*)(nullptr + I)
#define MOV4mr(I,O) AC2(MOV4mr,I,O) // *(u32*)(nullptr + I) ← O
#define MOV8rmo(O,I,OFF) AC3(MOV8rmo,O,I,OFF) // O ← *(u64*)(nullptr + I + OFF)
#define MOV8mro(I,O,OFF) AC3(MOV8mro,I,O,OFF) // *(u64*)(nullptr + I + OFF) ← O
#define MOV4rmo(O,I,OFF) AC3(MOV4rmo,O,I,OFF)
#define MOV4mro(I,O,OFF) AC3(MOV4mro,I,O,OFF)
#define MOV8pr(OFF,I) {AC2(MOV8pr,OFF,I); TSADD(b_r, ASM_SIZE-4);}
#define MOV8rp(I,OFF) {AC2(MOV8rp,OFF,I); TSADD(b_r, ASM_SIZE-4);}

#define LEAi(O,I,IMM) AC3(LEAi,O,I,IMM)
#define BZHI(O,I,N) AC3(BZHI,O,I,N) // requires __BMI2__

#define PUSH(O) AC1(PUSH,O)
#define POP(O) AC1(POP,O)
#define CALL(IMM) AC1(CALL,IMM)
#define CALLi(I) {AC1(CALLi,(i64)(I)); TSADD(b_r, ASM_SIZE-4);} // I must be 32-bit
#define RET() {b_o=asm_r(b_o); ASM1(0xC3);}

#define JO(L)  u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x70);ASM1(-2);}
#define JNO(L) u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x71);ASM1(-2);}
#define JB(L)  u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x72);ASM1(-2);}
#define JAE(L) u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x73);ASM1(-2);}
#define JE(L)  u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x74);ASM1(-2);}
#define JNE(L) u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x75);ASM1(-2);}
#define JBE(L) u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x76);ASM1(-2);}
#define JA(L)  u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x77);ASM1(-2);}
#define JS(L)  u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x78);ASM1(-2);}
#define JNS(L) u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x79);ASM1(-2);}
#define JP(L)  u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x7A);ASM1(-2);}
#define JNP(L) u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x7B);ASM1(-2);}
#define JL(L)  u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x7C);ASM1(-2);}
#define JGE(L) u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x7D);ASM1(-2);}
#define JLE(L) u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x7E);ASM1(-2);}
#define JG(L)  u64 L=ASM_SIZE; {b_o=asm_r(b_o); ASM1(0x7F);ASM1(-2);}
#define LBL1(L) { i64 t=(i8)b_o->data[L+1] + ASM_SIZE-(i64)L; if(t!=(i8)t)thrM("x86-64 codegen: jump too long!"); b_o->data[L+1] = t; }

ASMI(ADD, Reg o, Reg i) { nREX8(o,i); ASM1(0x01); nA_REG(o,i); }
ASMI(SUB, Reg o, Reg i) { nREX8(o,i); ASM1(0x29); nA_REG(o,i); }
ASMI(XOR, Reg o, Reg i) { nREX8(o,i); ASM1(0x31); nA_REG(o,i); }
ASMI( OR, Reg o, Reg i) { nREX8(o,i); ASM1(0x09); nA_REG(o,i); }
ASMI(AND, Reg o, Reg i) { nREX8(o,i); ASM1(0x21); nA_REG(o,i); }
ASMI(CMP, Reg o, Reg i) { nREX8(o,i); ASM1(0x39); nA_REG(o,i); }
ASMI(XOR4, Reg o, Reg i) { nREX4(o,i); ASM1(0x31); nA_REG(o,i); }
ASMI(ADDi, Reg o, i32 imm) { if(!imm) return; nREX8(o,0); if(imm==(i8)imm) { ASM1(0x83); nA_REG(o,0); ASM1(imm); } else { ASM1(0x81); nA_REG(o,0); ASM4(imm); } }
ASMI(SUBi, Reg o, i32 imm) { if(!imm) return; nREX8(o,0); if(imm==(i8)imm) { ASM1(0x83); nA_REG(o,5); ASM1(imm); } else { ASM1(0x81); nA_REG(o,5); ASM4(imm); } }
ASMI(SHLi, Reg o, i8 imm) { if(!imm) return; nREX8(o,0); ASM1(0xC1); nA_REG(o,4); ASM1(imm); }
ASMI(SHRi, Reg o, i8 imm) { if(!imm) return; nREX8(o,0); ASM1(0xC1); nA_REG(o,5); ASM1(imm); }
ASMI(ADD4mi, Reg o, i32 imm) { nREX4(o,0); ASM1(0x83); nA_REG(o,0); ASM1(imm); }

ASMI(INC4mo, Reg o, i32 off) { nREX4(o,0); ASM1(0xff); MRMo(o,0,off); }
ASMI(INC8mo, Reg o, i32 off) { nREX8(o,0); ASM1(0xff); MRMo(o,0,off); }
ASMI(DEC4mo, Reg o, i32 off) { nREX4(o,0); ASM1(0xff); MRMo(o,1,off); }
ASMI(DEC8mo, Reg o, i32 off) { nREX8(o,0); ASM1(0xff); MRMo(o,1,off); }


ASMI(MOV4, Reg o, Reg i) { nREX4(o,i); ASM1(0x89); nA_REG(o,i); }
ASMI(MOV , Reg o, Reg i) { nREX8(o,i); ASM1(0x89); nA_REG(o,i); }
ASMI(MOVi, Reg o, i64 i) {
  if (i==0) { iXOR4(b_o,o,o); }
  else if (i==(i32)i) { nREX4(o,0); ASM1(0xB8+(o&7)); ASM4(i); }
  else                { nREX8(o,0); ASM1(0xB8+(o&7)); ASM8(i); }
}
ASMI(MOVi1l, Reg o, i8 imm) { if (o>=4) ASM1(o>=8?0x41:0x40); ASM1(0xb0+(o&7)); ASM1(imm); }
ASMI(MOV8mr, Reg o, Reg i) { nREX8(o,i); ASM1(0x89); MRM(o,i); }   ASMI(MOV8mro, Reg o, Reg i, i32 off) { nREX8(o,i); ASM1(0x89); MRMo(o,i, off); }
ASMI(MOV8rm, Reg i, Reg o) { nREX8(o,i); ASM1(0x8B); MRM(o,i); }   ASMI(MOV8rmo, Reg i, Reg o, i32 off) { nREX8(o,i); ASM1(0x8B); MRMo(o,i, off); }
ASMI(MOV4mr, Reg o, Reg i) { nREX4(o,i); ASM1(0x89); MRM(o,i); }   ASMI(MOV4mro, Reg o, Reg i, i32 off) { nREX4(o,i); ASM1(0x89); MRMo(o,i, off); }
ASMI(MOV4rm, Reg i, Reg o) { nREX4(o,i); ASM1(0x8B); MRM(o,i); }   ASMI(MOV4rmo, Reg i, Reg o, i32 off) { nREX4(o,i); ASM1(0x8B); MRMo(o,i, off); }
ASMI(MOV8pr,i32 off, Reg i) { nREX8(0,i); ASM1(0x89); MRMp(off,i); }
ASMI(MOV8rp,i32 off, Reg i) { nREX8(0,i); ASM1(0x8B); MRMp(off,i); }

ASMI(LEAi, Reg o, Reg i, i32 imm) { if(imm==0) iMOV(b_o,o,i); else { nREX8(i,o); ASM1(0x8D); MRMo(i,o,imm); } }
ASMI(BZHI, Reg o, Reg i, Reg n) { ASM1(0xC4); ASM1(0x42+(i<8)*0x20 + (o<8)*0x80); ASM1(0xf8-n*8); ASM1(0xF5); nA_REG(i, o); }

ASMI(PUSH, Reg O) { nREX4(O,0); ASM1(0x50+((O)&7)); }
ASMI(POP , Reg O) { nREX4(O,0); ASM1(0x58+((O)&7)); }

ASMI(CALL, Reg i) { nREX4(i,0); ASM1(0xFF); nA_REG(i,2); }
ASMI(CALLi, i32 imm) { ASM1(0xE8); if (imm>I32_MAX)err("immediate call outside of 32-bit range!"); ASM4(imm); }

#define IMM(A,B) MOVi(A,(u64)(B))