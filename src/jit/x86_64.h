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

// 16 integer registers:
//                0 1 2  (0: not saved, 1: callee saves, 2: caller saves)
// 0 rax  result  x
// 1 rcx  arg 4   x
// 2 rdx  arg 3   x
// 3 rbx            x
// 4 rsp  stack     x
// 5 rbp  base      x
// 6 rsi  arg 2   x
// 7 rdi  arg 1   x
// 8 r8   arg 5   x
// 9 r9   arg 6   x
// . r10              x
// . r11              x
// . r12-r15        x

typedef unsigned char U;
typedef unsigned char UC;
typedef unsigned char Reg;
#define REG_RES 0
#define REG_ARG0 7
#define REG_ARG1 6
#define REG_ARG2 2
#define REG_ARG3 1
#define REG_ARG4 8
#define REG_ARG5 9
#define REG_SP 4
#define NO_REG 16
#define NO_REG_NM 17
typedef unsigned short RegM;
#define REG_NEVER 48 // Never modify rsp or rbp
#define REG_MASK 61496 // Registers which must be pushed before use
#define REG_SAVE 4039 // Registers which function calls may modify

#define MAX_C_REG 8

// Ignore leading 0x40
// TODO Doesn't drop 0xF2 0x40, etc.
#define ASM_RAW(A, OP) do { UC aa[] = OP; u8 off=aa[0]==0x40; TSADDA(A, aa+off, sizeof(aa)-off); } while(0)

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

#define MOV(O,I)  {REX8(O,I),0x89,A_REG(O,I)}
#define ADD(O,I)  {REX8(O,I),0x01,A_REG(O,I)}
#define SUB(O,I)  {REX8(O,I),0x29,A_REG(O,I)}
#define XOR(O,I)  {REX8(O,I),0x31,A_REG(O,I)}
#define TEST(O,I) {REX8(O,I),0x85,A_REG(O,I)}
#define IMUL(I,O) {REX8(O,I),0x0F,0xAF,A_REG(O,I)}
#define CMP(O,I)  {REX8(O,I),0x39,A_REG(O,I)}
#define NEG(I,O)  {REX8(O,0),0xF7,A_REG(O,3)}

#define CMP4_MI(O,I) {REX4(O,0),0x81,A_0REG(O,7),BYTES4(I)}

// TODO REX for sil, etc
#define MOVZX4(I,O) {REX4(O,I),0x0F,0xB6,A_REG(O,I)}

#define ADDI4(O,I) {REX8(O,0),0x81,A_REG(O,0),BYTES4(I)}
#define SUBI4(O,I) {REX8(O,0),0x81,A_REG(O,5),BYTES4(I)}

#define ADDI1(O,I) {REX8(O,0),0x83,A_REG(O,0),(U)(I)}
#define SUBI1(O,I) {REX8(O,0),0x83,A_REG(O,5),(U)(I)}
#define SHLI1(O,I) {REX8(O,0),0xC1,A_REG(O,4),(U)(I)}
#define SHRI1(O,I) {REX8(O,0),0xC1,A_REG(O,5),(U)(I)}

#define XOR4(O,I)  {REX4(O,I),0x31,A_REG(O,I)}

#define LEA1(O,A,B)  {REX8_3(A,O,B),0x8D,A_0REG(4,O),A_0REG(A,B)}

#define CQO(O,I)   {0x48,0x99}
#define IDIV(O,I)  {REX8(O,0),0xF7,A_REG(O,7)}
#define REG_IDIV_0  REG_RES
#define REG_IDIV_1  REG_ARG2

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

#define PUSH(O,I) {REX4(O,0),0x50+((O)&7)}
#define POP(O,I)  {REX4(O,0),0x58+((O)&7)}

#define BYTES4(I) ((UC)(I)),((UC)((I)>>8)),((UC)((I)>>16)),((UC)((I)>>24))
#define BYTES8(I) BYTES4(I) ,((UC)((I)>>32)),((UC)((I)>>40)) \
                            ,((UC)((I)>>48)),((UC)((I)>>56))

#define MOV_MR(O,I,OFF) {REX8(O,I),0x89,0x40+A_0REG(O,I),OFF}
#define MOV_MR0(O,I)    {REX8(O,I),0x89,A_0REG(O,I)}
#define MOV_RM(I,O,OFF) {REX8(O,I),0x8B,0x40+A_0REG(O,I),OFF}
#define MOV_RM0(I,O)    {REX8(O,I),0x8B,A_0REG(O,I)}
#define MOV_RI(O,I)     {REX8(O,0),0xB8+((O)&7), BYTES8(I)}

#define MOV4_MI(O,I,OFF) {REX4(O,0),0xC7,0x40+A_0REG(O,0),OFF,BYTES4(I)}
#define MOV4_RI(O,I)     {REX4(O,0),0xB8+((O)&7) , BYTES4(I)}
#define MOV4_MR(O,I,OFF) {REX4(O,I),0x89,0x40+A_0REG(O,I),OFF}
#define MOV4_MR0(O,I)    {REX4(O,I),0x89,A_0REG(O,I)}
#define MOV4_RM(I,O,OFF) {REX4(O,I),0x8B,0x40+A_0REG(O,I),OFF}
#define MOV4_RM0(I,O)    {REX4(O,I),0x8B,A_0REG(O,I)}

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

#define CALL(O,I) {REX4(O,0),0xFF,A_REG(O,2)}
#define CALLI(I,O) {0xE8,BYTES4((u32)(I))}

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

#define JO(O,I)  {0x70,((UC)(O)-2)}
#define JNO(O,I) {0x71,((UC)(O)-2)}
#define JB(O,I)  {0x72,((UC)(O)-2)}
#define JAE(O,I) {0x73,((UC)(O)-2)}
#define JE(O,I)  {0x74,((UC)(O)-2)}
#define JNE(O,I) {0x75,((UC)(O)-2)}
#define JBE(O,I) {0x76,((UC)(O)-2)}
#define JA(O,I)  {0x77,((UC)(O)-2)}
#define JS(O,I)  {0x78,((UC)(O)-2)}
#define JNS(O,I) {0x79,((UC)(O)-2)}
#define JP(O,I)  {0x7A,((UC)(O)-2)}
#define JNP(O,I) {0x7B,((UC)(O)-2)}
#define JL(O,I)  {0x7C,((UC)(O)-2)}
#define JGE(O,I) {0x7D,((UC)(O)-2)}
#define JLE(O,I) {0x7E,((UC)(O)-2)}
#define JG(O,I)  {0x7F,((UC)(O)-2)}

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

#define RET {0xC3}


#define ASM(INS, O, I) ASM_RAW(bin, INS(O, I))
#define IMM(O,I) { u64 v=(u64)(I); if(v==0) ASM(XOR,O,O); else if(v>=0 & v<(1ULL<<32)) ASM(MOV4_RI,O,v); else ASM(MOV_RI,O,v); }
#define ADDI(O, I) { i32 v=(i32)(I); if(v) { if(v==(i8)v) ASM(ADDI1,O,v); else ASM(ADDI4,O,v); } } // I must fit in i32
#define SUBI(O, I) { i32 v=(i32)(I); if(v) { if(v==(i8)v) ASM(SUBI1,O,v); else ASM(SUBI4,O,v); } } // I must fit in i32

