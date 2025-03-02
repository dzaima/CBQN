#include "../nfns.h"
#include "../vm.h"
#include "../ns.h"
#include "../utils/mut.h"
// minimal compiler, capable of running mlochbaum/BQN/src/bootstrap/boot2.bqn
// supports:
//   parentheses, ⟨…⟩ literals, •-values
//   non-destructuring variable assignment
//   function, 1-modifier, and 2-modifier invocation
//   value and function class variables, namespace .-access
// input must be either an immediate expression, or a block potentially containing 𝕨 or 𝕩
// goal is to either error, or compile correctly


B native_comp;

#ifndef FAST_NATIVE_COMP
  #define FAST_NATIVE_COMP 1
#endif


NOINLINE B nc_emptyI32Vec() {
  i32* rp;
  return m_i32arrv(&rp, 0);
}
B nc_ivec1(i32 a              ) { i32* rp; B r = m_i32arrv(&rp, 1); rp[0]=a;                   return r; }
B nc_ivec2(i32 a, i32 b       ) { i32* rp; B r = m_i32arrv(&rp, 2); rp[0]=a; rp[1]=b;          return r; }
B nc_ivec3(i32 a, i32 b, i32 c) { i32* rp; B r = m_i32arrv(&rp, 3); rp[0]=a; rp[1]=b; rp[2]=c; return r; }
NOINLINE void nc_iadd(B* w, i32 x) { // consumes x
  assert(TY(*w)==t_i32arr);
  *w = vec_add(*w, m_f64(x));
}
NOINLINE void nc_ijoin(B* w, B x) { // doesn't consume x
  assert(TY(*w)==t_i32arr && TI(x,elType)==el_i32);
  #if FAST_NATIVE_COMP
    *w = vec_join(*w, incG(x));
  #else
    SGetU(x)
    usz ia = IA(x);
    for (usz i = 0; i < ia; i++) nc_iadd(w, o2i(GetU(x, i)));
  #endif
}



#define nc_equal equal
NOINLINE void nc_add(B* w, B x) { // consumes x
  *w = vec_add(*w, x);
}
NOINLINE B nc_pop(B* wp) {
  B w = *wp;
  assert(isArr(w) && v(w)->refc==1 && IA(w)>0);
  B r = IGetU(w, a(w)->ia-1);
  a(w)->ia--;
  #if VERIFY_TAIL
    assert(TY(w) == t_harr);
    ux start = offsetof(HArr, a) + a(w)->ia * sizeof(B);
    FINISH_OVERALLOC(a(w), start, start+sizeof(B));
  #endif
  return r;
}



NOINLINE B nc_literal(B val) {
  HArr_p r = m_harr0p(1);
  r.a[0] = val;
  arr_shAtm((Arr*)r.c);
  return r.b;
}
static bool nc_up(u32 c) { return (c>='A' & c<='Z'); }
static bool nc_al(u32 c) { return (c>='a' & c<='z') | nc_up(c); }
static bool nc_num(u32 c) { return (c>='0' & c<='9'); }
B sys_c1(B, B);
NOINLINE B nc_tokenize(B prims, B sysvs, u32* chars, usz len, bool* hasBlock) {
  B r = emptyHVec();
  usz i = 0;
  usz pi = USZ_MAX;
  while (i < len) {
    u32 c = chars[i++];
    if (i==pi) thrF("Native compiler: Tokenizer stuck on \\u%xi / %i", c, c);
    B val;
    switch (c) {
      case U'∞': val = nc_literal(m_f64(1.0/0)); break;
      case U'π': val = nc_literal(m_f64(3.141592653589793)); break;
      case '@': val = nc_literal(m_c32(0)); break;
      case ' ': continue;
      case'0'...'9': case U'¯': { // numbers
        bool neg = c==U'¯';
        if (!neg) i--;
        else if (!nc_num(chars[i])) thrM("Native compiler: Standalone negative sign");
        f64 num = 0;
        while (nc_num(chars[i])) num = num*10 + (chars[i++]-'0');
        if (neg) num = -num;
        val = nc_literal(m_f64(num));
        break;
      }
      case '"': { // string literal
        usz i0 = i;
        while ('"'!=chars[i]) { i++; if (i>=len) thrM("Native compiler: Unclosed string literal"); }
        if (chars[i+1]=='"') thrM("Native compiler: No support for escaped quote characters in strings");
        usz ia = i-i0;
        u32* vp; val = nc_literal(m_c32arrv(&vp, ia));
        memcpy(vp, chars+i0, ia*sizeof(u32));
        i++;
        break;
      }
      case '\'': { // character literal
        if (i+1 >= len || chars[i+1] != '\'') thrM("Native compiler: Unclosed character literal");
        val = nc_literal(m_c32(chars[i]));
        i+= 2;
        break;
      }
      case '#': { // comments
        while (i<len && chars[i]!='\n') i++;
        val = m_c32(','); i++;
        break;
      }
      case U'𝕨': case U'𝕩': { // special names
        *hasBlock = true;
        val = m_hvec2(m_f64(3), m_c32vec((u32[1]){c}, 1));
        break;
      }
      case 'a'...'z': case 'A'...'Z': case U'•': { // names
        bool sys = c==U'•';
        usz i0 = sys? i : i-1;
        while (nc_al(chars[i]) | nc_num(chars[i])) i++;
        usz ia = i-i0;
        u32* np; B name = m_c32arrv(&np, ia);
        PLAINLOOP for (usz j = 0; j < ia; j++) np[j] = chars[i0+j] + (nc_up(chars[i0+j])? 32 : 0);
        if (sys) {
          B sysRes = C1(sys, m_hvec1(name));
          val = nc_literal(IGet(sysRes, 0)); // won't have the class the user entered but ¯\_(ツ)_/¯
          decG(sysRes);
        } else {
          val = m_hvec2(m_f64(nc_up(c)? 0 : 3), name);
        }
        break;
      }
      case U'⋄': case ',': case '\n': { // separators
        val = m_c32(',');
        break;
      }
      case '(': case ')': case '{': case '}': case U'⟨': case U'⟩': case U'←': case U'↩': case U'.': { // syntax to be parsed later
        val = m_c32(c);
        break;
      }
      default: { // primitives
        u32* primRepr = U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍⋈↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊";
        usz j = 0;
        while(primRepr[j]) {
          if (primRepr[j]==c) { val = nc_literal(IGet(prims, j)); goto add; }
          j++;
        }
        thrF("Native compiler: Can't tokenize \\u%xi / %i", c, c);
      }
    }
    add:
    nc_add(&r, val);
  }
  return r;
}

usz addObj(B* objs, B val) { // consumes val
  nc_add(objs, val);
  return IA(*objs)-1;
}

i32 nc_ty(B x) { return o2iG(IGetU(x, 0)); }

B nc_generate(B p1) { // consumes
  // printf("p1: "); printI(p1); printf("\n");
  
  usz p1ia = IA(p1);
  B listFinal;
  if (p1ia == 1) { // simple case of 1 element
    listFinal = p1;
    toFinal:;
    B elFinal = IGet(listFinal, 0);
    if(nc_ty(elFinal)>3) thrM("Native compiler: Unexpected assignment");
    decG(listFinal);
    return elFinal;
  }
  
  // merge 1-modifiers
  SGetU(p1)
  B p2 = emptyHVec();
  {
    usz i = 0;
    while (i < p1ia) {
      B e = incG(GetU(p1, i));
      u8 e0t = nc_ty(e);
      usz j = i+1;
      if (e0t==4 || e0t==5) goto add;
      if (!(e0t==0 || e0t==3)) thrM("Native compiler: Unexpected type in expression");
      while (j<p1ia) {
        B e1 = GetU(p1, j);
        u8 e1t = nc_ty(e1);
        if (e1t==1) {
          B bc = nc_emptyI32Vec();
          nc_ijoin(&bc, IGetU(e1, 1));
          nc_ijoin(&bc, IGetU(e,  1));
          nc_iadd(&bc, MD1C);
          decG(e);
          e = m_hvec2(m_f64(0), bc);
        } else if (e1t==2) {
          if (j+1 == p1ia) thrM("Native compiler: Expression ended in 2-modifier");
          B e2 = GetU(p1, ++j);
          u8 e2t = nc_ty(e2);
          if (!(e2t==0 || e2t==3)) thrM("Native compiler: Improper 2-modifier e2 operand");
          B bc = nc_emptyI32Vec();
          nc_ijoin(&bc, IGetU(e2, 1));
          nc_ijoin(&bc, IGetU(e1, 1));
          nc_ijoin(&bc, IGetU(e,  1));
          nc_iadd(&bc, MD2C);
          decG(e);
          e = m_hvec2(m_f64(0), bc);
        } else break;
        j++;
      }
      
      add: nc_add(&p2, e);
      i = j;
    }
  }
  decG(p1);
  // printf("p2: "); printI(p2); printf("\n");
  
  // parse as either tacit or explicit
  SGetU(p2)
  usz p2ia = IA(p2);
  if (p2ia==1) { listFinal=p2; goto toFinal; }
  B e = incG(GetU(p2, p2ia-1));
  usz i = p2ia-1; // pointer to last used thing
  u8 tyE = nc_ty(GetU(p2,p2ia-1));
  bool explicit = tyE==3;
  
  while(i>0) {
    B en1 = GetU(p2, i-1);
    u8 en1t = nc_ty(en1);
    // printf("e @ %d: ", i); printI(e); printf("\n");
    
    if (en1t==4 || en1t==5) { // assignment
      if (!explicit && i!=1) thrM("Native compiler: Assignment in the middle of tacit code");
      B bc = nc_emptyI32Vec();
      nc_ijoin(&bc, IGetU(e,   1));
      nc_ijoin(&bc, IGetU(en1, 1));
      if (tyE != o2iG(IGetU(en1, 2))) thrM("Native compiler: Wrong assignment class");
      nc_iadd(&bc, en1t==4? SETN : SETU);
      decG(e);
      e = m_hvec2(m_f64(explicit? 3 : 0), bc);
      i-= 1;
      continue;
    }
    if (en1t!=0) thrM("Native compiler: Expected but didn't get function");
    
    if (i>=2) { // dyadic call, fork
      B en2 = GetU(p2, i-2);
      if (nc_ty(en2)==3 || (!explicit && nc_ty(en2)==0)) {
        B bc = nc_emptyI32Vec();
        nc_ijoin(&bc, IGetU(e,   1));
        nc_ijoin(&bc, IGetU(en1, 1));
        nc_ijoin(&bc, IGetU(en2, 1));
        nc_iadd(&bc, explicit? FN2O : TR3O);
        decG(e);
        e = m_hvec2(m_f64(explicit? 3 : 0), bc);
        i-= 2;
        continue;
      }
    }
    
    // monadic call, atop
    B bc = nc_emptyI32Vec();
    nc_ijoin(&bc, IGetU(e,   1));
    nc_ijoin(&bc, IGetU(en1, 1));
    nc_iadd(&bc, explicit? FN1O : TR2D);
    decG(e);
    e = m_hvec2(m_f64(explicit? 3 : 0), bc);
    i-= 1;
  }
  // printf("e: "); printI(e); printf("\n");
  decG(p2);
  return e;
}

#if FAST_NATIVE_COMP
  #include "../utils/hash.h"
  typedef H_b2i** Vars;
  u32 nc_var(Vars vars, B name) { // doesn't consume
    bool had;
    u64 p = mk_b2i(vars, name, &had);
    if (had) return (*vars)->a[p].val;
    return (*vars)->a[p].val = (*vars)->pop-1;
  }
#else
  typedef B* Vars;
  u32 nc_var(Vars vars, B name) { // doesn't consume
    B o = *vars;
    usz ia = IA(o);
    SGetU(o)
    for (usz i = 0; i < ia; i++) if (nc_equal(GetU(o, i), name)) return i;
    nc_add(vars, incG(name));
    return ia;
  }
#endif

B nc_parseStatements(B tokens, usz i0, usz* i1, u32 close, B* objs, Vars vars) {
  SGetU(tokens)
  usz tia = IA(tokens);
  usz i = i0;
  
  B statements = emptyHVec();
  B parts = emptyHVec(); // list of lists; first element indicates class: 0:fn; 1:md1; 2:md2; 3:subject; 4:v←; 5:v↩
  
  while (true) {
    B ct;
    if (i==tia) ct = m_c32(0);
    else ct = GetU(tokens, i++);
    // printf("next token: "); printI(ct); printf("\n");
    
    if (isC32(ct)) {
      u32 ctc = o2cG(ct);
      if (ctc=='\0' || ctc==close || ctc==',') {
        if (IA(parts)>0) {
          nc_add(&statements, nc_generate(parts));
          parts = emptyHVec();
        }
        if (ctc==close) {
          decG(parts);
          break;
        }
        
        if (ctc=='\0') thrM("Native compiler: Unclosed");
        if (close==U')') thrM("Native compiler: Multiple statements in parens");
      } else if (ctc=='(' || ctc==U'⟨') {
        usz iM;
        B res = nc_parseStatements(tokens, i, &iM, ctc=='('? ')' : U'⟩', objs, vars);
        i = iM;
        nc_add(&parts, res);
      } else if (ctc==U'{' || ctc==U'}') {
        thrM("Native compiler: Nested blocks aren't supported");
      } else if (ctc==U'←' || ctc==U'↩') {
        thrM("Native compiler: Invalid assignment");
      } else if (ctc=='.') {
        if (IA(parts)==0 || i==i0+1) thrM("Native compiler: Expected value before '.'");
        
        B ns = nc_pop(&parts);
        if (nc_ty(ns) != 3) thrM("Native compiler: Expected subject before '.'");
        
        if (i==tia) thrM("Native compiler: Expression ended with '.'");
        B name = GetU(tokens, i++);
        if (!isArr(name) || RNK(name)!=1) thrM("Native compiler: Expected name to follow '.'");
        
        B bc = nc_emptyI32Vec();
        nc_ijoin(&bc, IGetU(ns, 1));
        decG(ns);
        nc_iadd(&bc, FLDG);
        nc_iadd(&bc, str2gid(IGetU(name,1)));
        nc_add(&parts, m_hvec2(IGet(name,0), bc));
      } else thrF("Native compiler: Unexpected character token \\u%xi / %i", ctc, ctc);
    } else if (isArr(ct)) {
      if (RNK(ct)==0) { // literal
        B val = IGetU(ct, 0);
        usz j = addObj(objs, inc(val));
        i32 type = isFun(val)? 0 : isMd1(val)? 1 : isMd2(val)? 2 : 3;
        nc_add(&parts, m_hvec2(m_f64(type), nc_ivec2(PUSH, j)));
      } else { // name
        u8 ty = nc_ty(ct);
        u8 rty = ty;
        assert(ty<=3);
        if (i<tia) {
          B ct2 = GetU(tokens, i);
          if (isC32(ct2) && (o2cG(ct2)==U'←' || o2cG(ct2)==U'↩')) {
            rty = o2cG(ct2)==U'←'? 4 : 5;
            i++;
          }
        }
        nc_add(&parts, m_hvec3(m_f64(rty), nc_ivec3(rty<=3? VARO : VARM, 0, nc_var(vars, IGetU(ct, 1))), m_f64(ty)));
      }
    } else thrF("Native compiler: Unexpected token %B", ct);
  }
  *i1 = i;
  
  // merge statements
  SGetU(statements)
  B final = nc_emptyI32Vec();
  usz segAm = IA(statements);
  for (usz i = 0; i < segAm; i++) {
    B segment = GetU(statements, i);
    if (i && close!=U'⟩') nc_iadd(&final, POPS);
    nc_ijoin(&final, IGetU(segment, 1));
  }
  
  u8 type0 = segAm==1? nc_ty(GetU(statements,0)) : 99;
  decG(statements);
  
  u8 type;
  if (close=='\0' || close=='}') {
    nc_iadd(&final, RETN);
    type = 0;
  } else if (close==U'⟩') {
    nc_iadd(&final, LSTO);
    nc_iadd(&final, segAm);
    type = 3;
  } else if (close==')') {
    type = type0;
  } else thrM("bad closing");
  return m_hvec2(m_f64(type), final);
}

NOINLINE usz nc_skipSeparators(B tokens, usz i) {
  while (i < IA(tokens) && isC32(IGetU(tokens,i)) && o2cG(IGetU(tokens,i))==',') i++;
  return i;
}

B nc_parseBlock(B tokens, usz i0, u32 end, bool isBlock, B* objs, u32* varCount) { // returns bytecode
  usz i1;
  #if FAST_NATIVE_COMP
  H_b2i* vars0 = m_b2i(32);
  #else
  B vars0 = emptyHVec();
  #endif
  Vars vars = &vars0;
  B toFree = emptyHVec();
  if (isBlock) {
    B t;
    t=m_c32vec(U"𝕤", 1); nc_var(vars, t); nc_add(&toFree, t);
    t=m_c32vec(U"𝕩", 1); nc_var(vars, t); nc_add(&toFree, t);
    t=m_c32vec(U"𝕨", 1); nc_var(vars, t); nc_add(&toFree, t);
  }
  B r0 = nc_parseStatements(tokens, i0, &i1, end, objs, vars);
  #if FAST_NATIVE_COMP
  *varCount = vars0->pop;
  free_b2i(vars0);
  #else
  *varCount = IA(vars0);
  decG(vars0);
  #endif
  i1 = nc_skipSeparators(tokens, i1);
  if (i1 != IA(tokens)) thrM("Native compiler: Code present after block end");
  B r = TO_GET(r0, 1);
  decG(toFree);
  return r;
}

B nativeComp_c2(B t, B w, B x) {
  SGetU(w)
  B prims = GetU(w,0);
  B sysvs = GetU(w,1);
  bool fnBlock = false;
  
  // tokenize
  usz xia = IA(x);
  u32* xBuf; B xBufO = m_c32arrv(&xBuf, xia+1);
  SGetU(x)
  for (usz i = 0; i < xia; i++) xBuf[i] = o2c(GetU(x, i));
  xBuf[xia] = 0;
  B tokens = nc_tokenize(prims, sysvs, xBuf, xia, &fnBlock);
  decG(xBufO);
  
  // parse
  B objs = emptyHVec();
  i32* bc0;
  B bytecode = m_i32arrv(&bc0, fnBlock? 3 : 0);
  usz i0 = nc_skipSeparators(tokens, 0);
  bool anyBlock = i0<IA(tokens) && isC32(IGetU(tokens,i0)) && o2cG(IGetU(tokens,i0))=='{';
  if (anyBlock) i0++;
  if (fnBlock) {
    bc0[0] = DFND;
    bc0[1] = 1;
    bc0[2] = RETN;
  }
  u32 varCount;
  if (fnBlock && !anyBlock) thrM("Native compiler: 𝕨/𝕩 outside block");
  B bc1 = nc_parseBlock(tokens, i0, anyBlock? '}' : '\0', fnBlock, &objs, &varCount);
  nc_ijoin(&bytecode, bc1);
  decG(bc1);
  decG(tokens);
  
  // build result
  B res;
  if (fnBlock) {
    res = m_hvec4(
      bytecode,
      objs,
      m_hvec2(nc_ivec3(0, 1, 0), nc_ivec3(0, 0, 1)),
      m_hvec2(nc_ivec2(0, 0), nc_ivec2(3, varCount))
    );
  } else {
    res = m_hvec4(
      bytecode,
      objs,
      m_hvec1(nc_ivec3(0, 1, 0)),
      m_hvec1(nc_ivec2(0, varCount))
    );
  }
  
  decG(w); decG(x);
  return res;
}

void nativeCompiler_init() {
  native_comp = m_nfn(registerNFn(m_c8vec_0("(native compiler)"), c1_bad, nativeComp_c2), bi_N);
  gc_add(native_comp);
}
