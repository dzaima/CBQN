// based on I: https://github.com/mlochbaum/ILanguage/blob/master/x86_64.h
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
static AsmStk asm_ins; // TODO add as root
static AsmStk asm_rel;
static i32 asm_depth = 0;

static NOINLINE void asm_allocBuf(AsmStk* stk, u64 sz) {
  TAlloc* a = mm_alloc(sizeof(TAlloc) + sz, t_temp);
  stk->s = a->data;
  stk->c = a->data;
  stk->e = a->data + sz;
}
typedef struct AsmRestorer {
  struct CustomObj;
  i32 depth;
  AsmStk ins, rel;
} AsmRestorer;
static void asmRestorer_free(Value* v) {
  asm_depth = ((AsmRestorer*)v)->depth;
  asm_ins   = ((AsmRestorer*)v)->ins;
  asm_rel   = ((AsmRestorer*)v)->rel;
}
static NOINLINE void asm_init() {
  AsmRestorer* r = customObj(sizeof(AsmRestorer), noop_visit, asmRestorer_free);
  r->depth = asm_depth;
  r->ins = asm_ins;
  r->rel = asm_rel;
  gsAdd(tag(r, OBJ_TAG));
  
  asm_depth++;
  asm_allocBuf(&asm_ins, 64);
  asm_allocBuf(&asm_rel, 64);
}
static NOINLINE void asm_free() {
  mm_free((Value*) TOBJ(asm_ins.s));
  mm_free((Value*) TOBJ(asm_rel.s));
  
  assert(asm_depth>0);
  B v = gsPop();
  assert(TY(v)==t_customObj);
  decG(v);
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

#define ASM_SIZE (asm_ins.c - asm_ins.s)


static inline void asm_w1(u8* data, i8 v) { *data = v; }
static inline void asm_w2(u8* data, i16 v) { memcpy(data, (i16[]){v}, 2); }
static inline void asm_w4(u8* data, i32 v) { memcpy(data, (i32[]){v}, 4); }
static inline void asm_w8(u8* data, i64 v) { memcpy(data, (i64[]){v}, 8); }
#define ASM1(X) ({ asm_w1(ic, X); ic+= 1; })
#define ASM2(X) ({ asm_w2(ic, X); ic+= 2; })
#define ASM4(X) ({ asm_w4(ic, X); ic+= 4; })
#define ASM8(X) ({ asm_w8(ic, X); ic+= 8; })
static inline i32  asm_r4(u8* data) { i32 v; memcpy(&v, data, 4); return v; }

static inline void asm_r() {
  if (RARE(asm_ins.c+32 > asm_ins.e)) asm_bufDbl(&asm_ins, 32);
}

static NOINLINE void asm_addRel(u32 v) {
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



#define REX0(O,I) (((O)>7)+(((I)>7)<<2))
#define REX4(O,I) {u8 t=REX0(O,I); if(t) ASM1(0x40+t); }
#define REX8(O,I) ASM1(0x48 + REX0(O,I))
#define CHK4(O) (((O)&7)==4)
#define CHK5(O) (((O)&7)==5)
#define CHK45(O) (((O)&7)>>1==2)

// ModR/M mess
#define A_0REG(O,I,A) ((((I)&7)<<3) + ((O)&7) + (A))
#define MRM(O,I) ASM1(A_0REG(O,I,0x40*CHK5(O))); if(CHK45(O)) ASM1(CHK4(O)?0x24:0);
#define MRMo(O,I,OFF) { i64 t=OFF;          \
  bool b1 = t||CHK5(O); bool b4 = t!=(i8)t; \
  ASM1(A_0REG(O, I, b1? b4?0x80:0x40 : 0)); \
  if(CHK4(O)) ASM1(0x24);                   \
  if(b4) ASM4(t); else if (b1) ASM1(t);     \
}
#define MRMp(OFF,I) ASM1(A_0REG(5,I,0x00)); ASM4(OFF); // offset to rip
#define MRMr(O,I)  ASM1(A_0REG(O,I,0xC0)) // only register operands

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

#define ASMS u8* ic=asm_ins.c
#define ASME asm_ins.c = ic; asm_r()
#define ASMI(N, ...) static NOINLINE void N(__VA_ARGS__)

// meaning of lowercase after basic instr name:
//   'r' means the corresponding argument is the direct register contents, 'm' - that it's dereferenced, 'p' - offset to rip; if all are 'r' or they don't have other options, they can be omitted
//   add 'o' for an offset argument (e.g. the 5 in 'mov rax, [rdi+5]'), 'i' for an immediate (e.g. 5 in 'add r12,5', or in 'lea rax, [rdi+5]')
// MOV8rm: O ← *(u64*)(nullptr + I)
// MOV8mr: *(u64*)(nullptr + I) ← O
// MOV4rm: O ← *(u32*)(nullptr + I)
// MOV4mr: *(u32*)(nullptr + I) ← O
// MOV8rmo: // O ← *(u64*)(nullptr + I + OFF)
// MOV8mro: // *(u64*)(nullptr + I + OFF) ← O
// MOV4rmo: // O ← *(u32*)(nullptr + I + OFF)
// MOV4mro: // *(u32*)(nullptr + I + OFF) ← O
// BZHI: requires __BMI2__
ASMI(ADD,  Reg o, Reg i) { ASMS; REX8(o,i); ASM1(0x01); MRMr(o,i); ASME; }
ASMI(SUB,  Reg o, Reg i) { ASMS; REX8(o,i); ASM1(0x29); MRMr(o,i); ASME; }
ASMI( OR,  Reg o, Reg i) { ASMS; REX8(o,i); ASM1(0x09); MRMr(o,i); ASME; }
ASMI(AND,  Reg o, Reg i) { ASMS; REX8(o,i); ASM1(0x21); MRMr(o,i); ASME; }
ASMI(CMP,  Reg o, Reg i) { ASMS; REX8(o,i); ASM1(0x39); MRMr(o,i); ASME; }
ASMI(XOR4, Reg o, Reg i) { ASMS; REX4(o,i); ASM1(0x31); MRMr(o,i); ASME; }
ASMI(XOR,  Reg o, Reg i) { ASMS; REX8(o,i); ASM1(0x31); MRMr(o,i); ASME; }
ASMI(ADDi, Reg o, i32 imm) { if(!imm) return; ASMS; REX8(o,0); if(imm==(i8)imm) { ASM1(0x83); MRMr(o,0); ASM1(imm); } else { ASM1(0x81); MRMr(o,0); ASM4(imm); } ASME; }
ASMI(SUBi, Reg o, i32 imm) { if(!imm) return; ASMS; REX8(o,0); if(imm==(i8)imm) { ASM1(0x83); MRMr(o,5); ASM1(imm); } else { ASM1(0x81); MRMr(o,5); ASM4(imm); } ASME; }
ASMI(SHLi, Reg o, i8 imm) { if(!imm) return; ASMS; REX8(o,0); ASM1(0xC1); MRMr(o,4); ASM1(imm); ASME; }
ASMI(SHRi, Reg o, i8 imm) { if(!imm) return; ASMS; REX8(o,0); ASM1(0xC1); MRMr(o,5); ASM1(imm); ASME; }
ASMI(ADD4mi, Reg o, i32 imm) { ASMS; REX4(o,0); ASM1(0x83); MRMr(o,0); ASM1(imm); ASME; }

ASMI(INC4mo, Reg o, i32 off) { ASMS; REX4(o,0); ASM1(0xff); MRMo(o,0,off); ASME; }
ASMI(INC8mo, Reg o, i32 off) { ASMS; REX8(o,0); ASM1(0xff); MRMo(o,0,off); ASME; }
ASMI(DEC4mo, Reg o, i32 off) { ASMS; REX4(o,0); ASM1(0xff); MRMo(o,1,off); ASME; }
ASMI(DEC8mo, Reg o, i32 off) { ASMS; REX8(o,0); ASM1(0xff); MRMo(o,1,off); ASME; }


ASMI(MOV4, Reg o, Reg i) { ASMS; REX4(o,i); ASM1(0x89); MRMr(o,i); ASME; }
ASMI(MOV , Reg o, Reg i) { ASMS; REX8(o,i); ASM1(0x89); MRMr(o,i); ASME; }
ASMI(MOVi, Reg o, i64 i) {
  if (i==0) { XOR4(o,o); return; }
  ASMS;
  if (i==(i32)i) { REX4(o,0); ASM1(0xB8+(o&7)); ASM4(i); }
  else           { REX8(o,0); ASM1(0xB8+(o&7)); ASM8(i); }
  ASME;
}
ASMI(MOVi1l, Reg o, i8 imm) { ASMS; if (o>=4) ASM1(o>=8?0x41:0x40); ASM1(0xb0+(o&7)); ASM1(imm); ASME; }
ASMI(MOV4moi, Reg o, i32 off, i32 imm) { ASMS; REX4(o,0); ASM1(0xc7); MRMo(o,0,off); ASM4(imm); ASME; }
ASMI(MOV4mr, Reg o, Reg i) { ASMS; REX4(o,i); ASM1(0x89); MRM(o,i); ASME; }   ASMI(MOV4mro, Reg o, Reg i, i32 off) { ASMS; REX4(o,i); ASM1(0x89); MRMo(o,i, off); ASME; }
ASMI(MOV4rm, Reg i, Reg o) { ASMS; REX4(o,i); ASM1(0x8B); MRM(o,i); ASME; }   ASMI(MOV4rmo, Reg i, Reg o, i32 off) { ASMS; REX4(o,i); ASM1(0x8B); MRMo(o,i, off); ASME; }
ASMI(MOV8mr, Reg o, Reg i) { ASMS; REX8(o,i); ASM1(0x89); MRM(o,i); ASME; }   ASMI(MOV8mro, Reg o, Reg i, i32 off) { ASMS; REX8(o,i); ASM1(0x89); MRMo(o,i, off); ASME; }
ASMI(MOV8rm, Reg i, Reg o) { ASMS; REX8(o,i); ASM1(0x8B); MRM(o,i); ASME; }   ASMI(MOV8rmo, Reg i, Reg o, i32 off) { ASMS; REX8(o,i); ASM1(0x8B); MRMo(o,i, off); ASME; }

ASMI(MOV8pr_i, u64 pos, Reg i) { ASMS; REX8(0,i); ASM1(0x89); MRMp(pos-4,i); ASME; }
ASMI(MOV8rp_i, u64 pos, Reg i) { ASMS; REX8(0,i); ASM1(0x8B); MRMp(pos-4,i); ASME; }

ASMI(LEAi, Reg o, Reg i, i32 imm) { if(imm==0) MOV(o,i); else { ASMS; REX8(i,o); ASM1(0x8D); MRMo(i,o,imm); ASME; } }
ASMI(BZHI, Reg o, Reg i, Reg n) { ASMS; ASM1(0xC4); ASM1(0x42+(i<8)*0x20 + (o<8)*0x80); ASM1(0xf8-n*8); ASM1(0xF5); MRMr(i, o); ASME; }

ASMI(iPUSH, Reg O) { ASMS; REX4(O,0); ASM1(0x50+((O)&7)); ASME; }
ASMI(iPOP , Reg O) { ASMS; REX4(O,0); ASM1(0x58+((O)&7)); ASME; }

ASMI(CALLi_i, u64 pos) { ASMS; ASM1(0xE8); ASM4(pos-4); ASME; }
ASMI(CALL, Reg i) { ASMS; REX4(i,0); ASM1(0xFF); MRMr(i,2); ASME; }
ASMI(RET) { ASMS; ASM1(0xC3); ASME; }

#define MOV8pr(POS,I) { MOV8pr_i((u64)(POS),I); asm_addRel(ASM_SIZE-4); }
#define MOV8rp(I,POS) { MOV8rp_i((u64)(POS),I); asm_addRel(ASM_SIZE-4); }
#define CALLi(POS) { CALLi_i((u64)(POS)); asm_addRel(ASM_SIZE-4); } // POS must be 32-bit

#define IMM(A,B) MOVi(A,(u64)(B))
