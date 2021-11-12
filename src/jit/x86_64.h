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

typedef struct AsmStk {
  u8* s; // actual allocation
  u8* c; // position for next write
  u8* e; // position past last writable position
} AsmStk;
static AsmStk asm_ins;
static AsmStk asm_rel;
static i32 asm_depth = 0;

static NOINLINE void asm_allocBuf(AsmStk* stk, u64 sz) {
  TAlloc* a = mm_alloc(sizeof(TAlloc) + sz, t_temp);
  stk->s = a->data;
  stk->c = a->data;
  stk->e = a->data + sz;
}
static NOINLINE void asm_init() {
  if (asm_depth>0) {
    gsAdd(tag(asm_ins.c-asm_ins.s, RAW_TAG)); gsAdd(tag(asm_ins.e-asm_ins.s, RAW_TAG)); gsAdd(tag(TOBJ(asm_ins.s), OBJ_TAG));
    gsAdd(tag(asm_rel.c-asm_rel.s, RAW_TAG)); gsAdd(tag(asm_rel.e-asm_rel.s, RAW_TAG)); gsAdd(tag(TOBJ(asm_rel.s), OBJ_TAG));
  }
  asm_depth++;
  asm_allocBuf(&asm_ins, 64);
  asm_allocBuf(&asm_rel, 64);
}
static NOINLINE void asm_free() {
  mm_free((Value*) TOBJ(asm_ins.s));
  mm_free((Value*) TOBJ(asm_rel.s));
  
  assert(asm_depth>0);
  asm_depth--;
  if (asm_depth>0) { u8* t;
    t = asm_rel.s = c(TAlloc, gsPop())->data;  asm_rel.e = t + (u64)v(gsPop());  asm_rel.c = t + (u64)v(gsPop());
    t = asm_ins.s = c(TAlloc, gsPop())->data;  asm_ins.e = t + (u64)v(gsPop());  asm_ins.c = t + (u64)v(gsPop());
  }
}

static NOINLINE void asm_bufDbl(AsmStk* stk, u64 nsz) {
  u8* prevS = stk->s;
  u64 size = stk->e - prevS;
  u64 used = stk->c - prevS;
  while (size < used+nsz) size*= 2;
  asm_allocBuf(stk, size);
  stk->c+= used;
  memcpy(stk->s, prevS, used);
  mm_free((Value*) TOBJ(prevS));
}

#define ALLOC_ASM(N)
#define FREE_ASM()
#define ASM_SIZE (asm_ins.c - asm_ins.s)


static inline void asm_w1(u8* data, i8 v) { *data = v; }
static inline void asm_w4(u8* data, i32 v) { memcpy(data, (i32[]){v}, 4); }
static inline void asm_w8(u8* data, i64 v) { memcpy(data, (i64[]){v}, 8); }
static inline i32  asm_r4(u8* data) { i32 v; memcpy(&v, data, 4); return v; }

static inline void asm_r() {
  if (RARE(asm_ins.c+32 > asm_ins.e)) asm_bufDbl(&asm_ins, 32);
}

static NOINLINE void asm_addRel(u32 v) { // TODO play around with inlining
  if (RARE(asm_rel.c == asm_rel.e)) asm_bufDbl(&asm_rel, 4);
  asm_w4(asm_rel.c, v);
  asm_rel.c+= 4;
}


static NOINLINE void asm_write(u8* P, u64 SZ) {
  memcpy(P, asm_ins.s, SZ);
  u64 relAm = (asm_rel.c-asm_rel.s)/4;
  for (u64 i = 0; i < relAm; i++) {
    u8* ins = P+asm_r4(asm_rel.s+i*4);
    u32 o = asm_r4(ins);
    u32 n = o-(u32)(u64)ins;
    asm_w4(ins, n);
  }
}



// Ignore leading 0x40
// TODO Doesn't drop 0xF2 0x40, etc.
// #define ASM_RAW(A, OP) do { UC aa[] = OP; u8 off=aa[0]==0x40; AADD(aa+off, sizeof(aa)-off); } while(0)

// Instructions
#define A_0REG(O,I) ((((I)&7)<<3) + ((O)&7))
#define A_REG(O,I) (0xC0 + A_0REG(O,I))
// #define REX0(O,I) (((O)>7)+(((I)>7)<<2))
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

// #define BYTES4(I) ((UC)(I)),((UC)((I)>>8)),((UC)((I)>>16)),((UC)((I)>>24))
// #define BYTES8(I) BYTES4(I) ,((UC)((I)>>32)),((UC)((I)>>40)) \
//                             ,((UC)((I)>>48)),((UC)((I)>>56))

// #define MOV_MR(O,I,OFF) {REX8(O,I),0x89,0x40+A_0REG(O,I),OFF}
// #define MOV_MR0(O,I)    {REX8(O,I),0x89,A_0REG(O,I)} // TODO is broken on (12,14)
// #define MOV_RM(I,O,OFF) {REX8(O,I),0x8B,0x40+A_0REG(O,I),OFF}
// #define MOV_RM0(I,O)    {REX8(O,I),0x8B,A_0REG(O,I)}
// #define MOV_RI(O,I)     {REX8(O,0),0xB8+((O)&7), BYTES8(I)}
// #define LEAo1(I,O,OFF)  {REX8(O,I),0x8D,0x40+A_0REG(O,I),OFF}

// #define MOV4_MI(O,I,OFF) {REX4(O,0),0xC7,0x40+A_0REG(O,0),OFF,BYTES4(I)}
// #define MOV4_RI(O,I)     {REX4(O,0),0xB8+((O)&7) , BYTES4(I)}
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

// #define JX(O,C)    {0x70+(C),(UC)(O)-2}
// #define J4X(O,C)   {0x0F,0x80+(C),BYTES4((O)-6)}
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

// #define JMP(O,I)  {0xEB,((UC)(O)-2)}
// #define JMP4(O,I) {0xE9,BYTES4((O)-5)}

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












#define ASM1(X) ({ asm_w1(ic, X); ic+= 1; })
#define ASM4(X) ({ asm_w4(ic, X); ic+= 4; })
#define ASM8(X) ({ asm_w8(ic, X); ic+= 8; })
#define ASMS u8* ic=asm_ins.c
#define ASME asm_ins.c = ic; asm_r()



#define nREX0(O,I) (((O)>7)+(((I)>7)<<2))
#define nREX8(O,I) ASM1(0x48 + nREX0(O,I))
#define nREX4(O,I) {u8 t=nREX0(O,I); if(t) ASM1(0x40+t); }
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

#define ASMI(N, ...) static NOINLINE void i##N(__VA_ARGS__)
#define  AC1(N,A    ) i##N(A    )
#define  AC2(N,A,B  ) i##N(A,B  )
#define  AC3(N,A,B,C) i##N(A,B,C)

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
#define MOV4moi(I,OFF,IMM) AC3(MOV4moi,I,OFF,IMM)
#define ADD4mi(O,IMM) AC2(ADD4mi,O,IMM) // ADD4mi(o,1) is an alternative for INC4mo(o,0)

#define MOV8rm(O,I) AC2(MOV8rm,O,I) // O ← *(u64*)(nullptr + I)
#define MOV8mr(I,O) AC2(MOV8mr,I,O) // *(u64*)(nullptr + I) ← O
#define MOV4rm(O,I) AC2(MOV4rm,O,I) // O ← *(u32*)(nullptr + I)
#define MOV4mr(I,O) AC2(MOV4mr,I,O) // *(u32*)(nullptr + I) ← O
#define MOV8rmo(O,I,OFF) AC3(MOV8rmo,O,I,OFF) // O ← *(u64*)(nullptr + I + OFF)
#define MOV8mro(I,O,OFF) AC3(MOV8mro,I,O,OFF) // *(u64*)(nullptr + I + OFF) ← O
#define MOV4rmo(O,I,OFF) AC3(MOV4rmo,O,I,OFF)
#define MOV4mro(I,O,OFF) AC3(MOV4mro,I,O,OFF)
#define MOV8pr(POS,I) { AC2(MOV8pr,(u64)(POS),I); asm_addRel(ASM_SIZE-4); }
#define MOV8rp(I,POS) { AC2(MOV8rp,(u64)(POS),I); asm_addRel(ASM_SIZE-4); }

#define LEAi(O,I,IMM) AC3(LEAi,O,I,IMM)
#define BZHI(O,I,N) AC3(BZHI,O,I,N) // requires __BMI2__

#define PUSH(O) AC1(PUSH,O)
#define POP(O) AC1(POP,O)
#define CALL(I) AC1(CALL,I)
#define CALLi(POS) { AC1(CALLi,(u64)(POS)); asm_addRel(ASM_SIZE-4); } // POS must be 32-bit
#define RET() { ASMS; ASM1(0xC3); ASME; }

static const u8 cO  = 0x0; static const u8 cNO = 0x1;
static const u8 cB  = 0x2; static const u8 cAE = 0x3;
static const u8 cE  = 0x4; static const u8 cNE = 0x5;
static const u8 cBE = 0x6; static const u8 cA  = 0x7;
static const u8 cS  = 0x8; static const u8 cNS = 0x9;
static const u8 cP  = 0xA; static const u8 cNP = 0xB;
static const u8 cL  = 0xC; static const u8 cGE = 0xD;
static const u8 cLE = 0xE; static const u8 cG  = 0xF;
#define J1(T, L) u64 L=ASM_SIZE; { ASMS;            ASM1((i8)(0x70+(T)));ASM1(-2);ASME; } // -2 comes from the instruction being 2 bytes long and L being defined at the start
#define J4(T, L) u64 L=ASM_SIZE; { ASMS; ASM1(0x0f);ASM1((i8)(0x80+(T)));ASM4(-6);ASME; }
#define LBL1(L) { i64 t=    (i8)asm_ins.s[L+1]  + ASM_SIZE-(i64)L; if(t!=(i8 )t) err("x86-64 codegen: jump too long!"); asm_ins.s[L+1] = t; }
#define LBL4(L) { i64 t=asm_r4(asm_ins.s+(L+2)) + ASM_SIZE-(i64)L; if(t!=(i32)t) err("x86-64 codegen: jump too long!"); asm_w4(asm_ins.s+(L+2), t); }

ASMI(ADD, Reg o, Reg i) { ASMS; nREX8(o,i); ASM1(0x01); nA_REG(o,i); ASME; }
ASMI(SUB, Reg o, Reg i) { ASMS; nREX8(o,i); ASM1(0x29); nA_REG(o,i); ASME; }
ASMI(XOR, Reg o, Reg i) { ASMS; nREX8(o,i); ASM1(0x31); nA_REG(o,i); ASME; }
ASMI( OR, Reg o, Reg i) { ASMS; nREX8(o,i); ASM1(0x09); nA_REG(o,i); ASME; }
ASMI(AND, Reg o, Reg i) { ASMS; nREX8(o,i); ASM1(0x21); nA_REG(o,i); ASME; }
ASMI(CMP, Reg o, Reg i) { ASMS; nREX8(o,i); ASM1(0x39); nA_REG(o,i); ASME; }
ASMI(XOR4, Reg o, Reg i) { ASMS; nREX4(o,i); ASM1(0x31); nA_REG(o,i); ASME; }
ASMI(ADDi, Reg o, i32 imm) { if(!imm) return; ASMS; nREX8(o,0); if(imm==(i8)imm) { ASM1(0x83); nA_REG(o,0); ASM1(imm); } else { ASM1(0x81); nA_REG(o,0); ASM4(imm); } ASME; }
ASMI(SUBi, Reg o, i32 imm) { if(!imm) return; ASMS; nREX8(o,0); if(imm==(i8)imm) { ASM1(0x83); nA_REG(o,5); ASM1(imm); } else { ASM1(0x81); nA_REG(o,5); ASM4(imm); } ASME; }
ASMI(SHLi, Reg o, i8 imm) { if(!imm) return; ASMS; nREX8(o,0); ASM1(0xC1); nA_REG(o,4); ASM1(imm); ASME; }
ASMI(SHRi, Reg o, i8 imm) { if(!imm) return; ASMS; nREX8(o,0); ASM1(0xC1); nA_REG(o,5); ASM1(imm); ASME; }
ASMI(ADD4mi, Reg o, i32 imm) { ASMS; nREX4(o,0); ASM1(0x83); nA_REG(o,0); ASM1(imm); ASME; }

ASMI(INC4mo, Reg o, i32 off) { ASMS; nREX4(o,0); ASM1(0xff); MRMo(o,0,off); ASME; }
ASMI(INC8mo, Reg o, i32 off) { ASMS; nREX8(o,0); ASM1(0xff); MRMo(o,0,off); ASME; }
ASMI(DEC4mo, Reg o, i32 off) { ASMS; nREX4(o,0); ASM1(0xff); MRMo(o,1,off); ASME; }
ASMI(DEC8mo, Reg o, i32 off) { ASMS; nREX8(o,0); ASM1(0xff); MRMo(o,1,off); ASME; }


ASMI(MOV4, Reg o, Reg i) { ASMS; nREX4(o,i); ASM1(0x89); nA_REG(o,i); ASME; }
ASMI(MOV , Reg o, Reg i) { ASMS; nREX8(o,i); ASM1(0x89); nA_REG(o,i); ASME; }
ASMI(MOVi, Reg o, i64 i) {
  if (i==0) { iXOR4(o,o); return; }
  ASMS;
  if (i==(i32)i) { nREX4(o,0); ASM1(0xB8+(o&7)); ASM4(i); }
  else           { nREX8(o,0); ASM1(0xB8+(o&7)); ASM8(i); }
  ASME;
}
ASMI(MOVi1l, Reg o, i8 imm) { ASMS; if (o>=4) ASM1(o>=8?0x41:0x40); ASM1(0xb0+(o&7)); ASM1(imm); ASME; }
ASMI(MOV4moi, Reg o, i32 off, i32 imm) { ASMS;  nREX4(o,0); ASM1(0xc7); MRMo(o,0,off); ASM4(imm); ASME; }
ASMI(MOV8mr, Reg o, Reg i) { ASMS; nREX8(o,i); ASM1(0x89); MRM(o,i); ASME; }   ASMI(MOV8mro, Reg o, Reg i, i32 off) { ASMS; nREX8(o,i); ASM1(0x89); MRMo(o,i, off); ASME; }
ASMI(MOV8rm, Reg i, Reg o) { ASMS; nREX8(o,i); ASM1(0x8B); MRM(o,i); ASME; }   ASMI(MOV8rmo, Reg i, Reg o, i32 off) { ASMS; nREX8(o,i); ASM1(0x8B); MRMo(o,i, off); ASME; }
ASMI(MOV4mr, Reg o, Reg i) { ASMS; nREX4(o,i); ASM1(0x89); MRM(o,i); ASME; }   ASMI(MOV4mro, Reg o, Reg i, i32 off) { ASMS; nREX4(o,i); ASM1(0x89); MRMo(o,i, off); ASME; }
ASMI(MOV4rm, Reg i, Reg o) { ASMS; nREX4(o,i); ASM1(0x8B); MRM(o,i); ASME; }   ASMI(MOV4rmo, Reg i, Reg o, i32 off) { ASMS; nREX4(o,i); ASM1(0x8B); MRMo(o,i, off); ASME; }
ASMI(MOV8pr, u64 pos, Reg i) { ASMS; nREX8(0,i); ASM1(0x89); MRMp(pos-4,i); ASME; }
ASMI(MOV8rp, u64 pos, Reg i) { ASMS; nREX8(0,i); ASM1(0x8B); MRMp(pos-4,i); ASME; }

ASMI(LEAi, Reg o, Reg i, i32 imm) { if(imm==0) iMOV(o,i); else { ASMS; nREX8(i,o); ASM1(0x8D); MRMo(i,o,imm); ASME; } }
ASMI(BZHI, Reg o, Reg i, Reg n) { ASMS; ASM1(0xC4); ASM1(0x42+(i<8)*0x20 + (o<8)*0x80); ASM1(0xf8-n*8); ASM1(0xF5); nA_REG(i, o); ASME; }

ASMI(PUSH, Reg O) { ASMS; nREX4(O,0); ASM1(0x50+((O)&7)); ASME; }
ASMI(POP , Reg O) { ASMS; nREX4(O,0); ASM1(0x58+((O)&7)); ASME; }

ASMI(CALL, Reg i) { ASMS; nREX4(i,0); ASM1(0xFF); nA_REG(i,2); ASME; }
ASMI(CALLi, u64 pos) { ASMS; ASM1(0xE8); ASM4(pos-4); ASME; }

#define IMM(A,B) MOVi(A,(u64)(B))
