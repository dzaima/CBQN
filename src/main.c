#include "core.h"
#include "vm.h"
#include "ns.h"
#include "utils/utf.h"
#include "utils/talloc.h"
#include "utils/file.h"
#include "utils/time.h"
#include "utils/interrupt.h"

#if defined(_WIN32) || defined(_WIN64)
  #include "windows/getline.h"
#endif


#if __GNUC__ && __i386__ && !__clang__
  #warning "CBQN is known to miscompile on GCC for 32-bit x86 builds; using clang instead is suggested"
#endif

static B replPath;
static Scope* gsc;
static bool init = false;

static NOINLINE void repl_init() {
  if (init) return;
  cbqn_init();
  replPath = m_c8vec_0("."); gc_add(replPath);
  Body* body = m_nnsDesc();
  B ns = m_nns(body);
  gsc = ptr_inc(c(NS, ns)->sc); gc_add(tag(gsc,OBJ_TAG));
  ptr_dec(v(ns));
  init = true;
}

typedef struct { bool r; char* e; } IsCmdTmp;
static NOINLINE IsCmdTmp isCmd0(char* s, const char* cmd) {
  while (*cmd) {
    if (*cmd==' ' && *s==0) return (IsCmdTmp){.r=true, .e=s};
    if (*s!=*cmd || *s==0) return (IsCmdTmp){.r=false, .e=s};
    s++; cmd++;
  }
  return (IsCmdTmp){.r=true, .e=s};
}
static bool isCmd(char* s, char** e, const char* cmd) {
  IsCmdTmp t = isCmd0(s, cmd);
  *e = t.e;
  return t.r;
}


#if USE_REPLXX
  #include <replxx.h>
  #include <errno.h>
  #include "utils/calls.h"
  #include "utils/cstr.h"
  static Replxx* global_replxx;
  static char* global_histfile;
  
  static i8 themes[3][12][3] = {
    // {-1,-1,-1} for default/unchanged color, {-1,-1,n} for grayscale 0…23, else RGB 0…5
    { // 0: "none"
      {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1},
      {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1},
      {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1},
    },
    { // 1: "dark"
      [ 0] = {-1,-1,-1}, // default
      [ 1] = {-1,-1,11}, // comments
      [ 2] = {4,1,1},    // numbers
      [ 3] = {1,2,5},    // strings
      [ 4] = {-1,-1,-1}, // value names
      [ 5] = {1,4,1},    // functions
      [ 6] = {5,1,4},    // 1-modifiers
      [ 7] = {5,4,2},    // 2-modifiers
      [ 8] = {5,4,0},    // assignment (←↩→⇐), statement separators (,⋄)
      [ 9] = {4,3,5},    // ⟨⟩, [] ·, @, ‿
      [10] = {3,2,4},    // {}
      [11] = {5,0,0},    // error
    },
    { // 2: "light"
      [ 0] = {-1,-1,-1}, // default
      [ 1] = {-1,-1,11}, // comments
      [ 2] = {1,0,2},    // numbers
      [ 3] = {0,0,2},    // strings
      [ 4] = {-1,-1,-1}, // value names
      [ 5] = {0,1,0},    // functions
      [ 6] = {2,0,0},    // 1-modifiers
      [ 7] = {2,1,0},    // 2-modifiers
      [ 8] = {0,0,5},    // assignment (←↩→⇐), statement separators (,⋄)
      [ 9] = {0,1,1},    // ⟨⟩, [] ·, @, ‿
      [10] = {3,0,4},    // {}
      [11] = {5,0,0},    // error
    }
  };
  
  typedef i8 Theme[12][3];
  static ReplxxColor theme_replxx[12];
  
  static i32 cfg_theme = 1;
  static bool cfg_enableKeyboard = true;
  static B cfg_path;
  
  NOINLINE void cfg_changed() {
    B s = emptyCVec();
    AFMT("theme=%i\nkeyboard=%i\n", cfg_theme, cfg_enableKeyboard);
    if (CATCH) { freeThrown(); goto end; }
    path_wChars(incG(cfg_path), s);
    popCatch();
    end: decG(s);
  }
  
  NOINLINE void cfg_set_theme(i32 num, bool writeCfg) {
    if (num>=3) return;
    cfg_theme = num;
    i8 (*data)[3] = themes[num];
    for (int i = 0; i < 12; i++) {
      i8 v0 = data[i][0];
      i8 v1 = data[i][1];
      i8 v2 = data[i][2];
      theme_replxx[i] = v0>=0? replxx_color_rgb666(v0,v1,v2) : v2>=0? replxx_color_grayscale(v2) : REPLXX_COLOR_DEFAULT;
    }
    if (writeCfg) cfg_changed();
  }
  void cfg_set_keyboard(bool enable, bool writeCfg) {
    cfg_enableKeyboard = enable;
    if (writeCfg) cfg_changed();
  }
  
  extern u32* dsv_text[];
  static B sysvalNames, sysvalNamesNorm;
  
  NOINLINE void fill_color(ReplxxColor* cols, int s, int e, ReplxxColor col) {
    for (int i = s; i < e; i++) cols[i] = col;
  }
  static bool chr_nl(u32 c) { return c==10 || c==13; }
  static bool chr_dig(u32 c) { return c>='0' && c<='9'; }
  static bool chr_low(u32 c) { return c>='a' && c<='z'; }
  static bool chr_upp(u32 c) { return c>='A' && c<='Z'; }
  static u32 chr_to_low(u32 c) { return c-'A'+'a'; }
  static u32 chr_to_upp(u32 c) { return c-'a'+'A'; }
  static bool chr_num0(u32 c) { return chr_dig(c) || c==U'¯' || c==U'π' || c==U'∞'; }
  static bool chr_name0(u32 c) { return chr_low(c) || chr_upp(c) || c=='_'; }
  static bool chr_nameM(u32 c) { return chr_name0(c) || chr_num0(c); }
  static u32* chrs_fn = U"!+-×÷⋆*√⌊⌈∧∨¬|=≠≤<>≥≡≢⊣⊢⥊∾≍⋈↑↓↕⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔«»𝔽𝔾𝕎𝕏𝕊";
  static u32* chrs_m1 = U"`˜˘¨⁼⌜´˝˙";
  static u32* chrs_m2 = U"∘⊸⟜○⌾⎉⚇⍟⊘◶⎊";
  static u32* chrs_lit = U"⟨⟩[]·@‿";
  static u32* chrs_dmd = U"←↩,⋄→⇐";
  static u32* chrs_blk = U"{}𝕨𝕩𝕗𝕘𝕣𝕤:?;";
  NOINLINE bool chr_in(u32 val, u32* chrs) {
    while(*chrs) if (val == *(chrs++)) return true;
    return false;
  }
  
  void highlighter_replxx(const char* input, ReplxxColor* colors, int size, void* data) {
    B charObj = utf8Decode0(input);
    if (IA(charObj) != size) goto end; // don't want to kill the REPL if this happens, but gotta do _something_
    
    charObj = toC32Any(vec_addN(charObj, m_c32(0)));
    u32* chars = c32any_ptr(charObj);
    
    u32* badPrefix = U")escaped"; // )escaped has its own parsing setup, highlighting that would be very misleading
    for (usz i = 0; i < 8; i++) if (chars[i]!=badPrefix[i]) goto not_bad_prefix;
    goto end;
    not_bad_prefix:;
    
    ReplxxColor* theme = theme_replxx;
    #define SKIP(V) ({ u32 c; while(true) { c=chars[i]; if (c==0 || !(V)) break; i++; } }) // skips to first character that doesn't match V, or to the final null byte
    #define FILL(T) ({ assert(i<=size); fill_color(colors, i0, i, theme[T]); })
    #define SET1(T) ({ assert(i<=size); colors[i0] = theme[T]; })
    #define SKIP1 ({ if (i+1<=size) i++; })
    int i = 0;
    while (i < size) {
      int i0 = i;
      u32 c0 = chars[i++];
      bool sys = c0==U'•' && i<size;
      if (sys) c0 = chars[i++];
      
      if (!sys && c0=='_' && chars[i0+1]==U'𝕣') { bool m2 = chars[i0+2]=='_'; i+= m2?2:1; FILL(m2? 7 : 6); }
      else if (sys || chr_name0(c0)) {
        if (!chr_name0(c0)) { i--; SET1(11); }
        else { SKIP(chr_nameM(c));  FILL(c0=='_'? (i0+(sys?2:1)==i? 11 : chars[i-1]=='_'? 7 : 6)  :  (chr_upp(c0)? 5 : 4)); }
      }
      else if (c0=='#') { SKIP(!chr_nl(c));  FILL(1); }
      else if (c0=='"') { SKIP(c!='"'); SKIP1;  FILL(3); }
      else if (c0=='\'') { bool ok = i+2<=size && chars[i+1]=='\''; if(ok) i+=2;  FILL(ok? 3 : 11); }
      else if (chr_num0(c0)) { SKIP(chr_num0(c) || c=='e' || c=='E' || c=='.' || c=='_');  FILL(2); }
      else if (chr_in(c0, chrs_fn)) SET1(5); else if (chr_in(c0, chrs_lit)) SET1(9);
      else if (chr_in(c0, chrs_m1)) SET1(6); else if (chr_in(c0, chrs_dmd)) SET1(8);
      else if (chr_in(c0, chrs_m2)) SET1(7); else if (chr_in(c0, chrs_blk)) SET1(10);
      else SET1(0);
      if (i<=i0) break; // in case the above code fails to progress forward, at least don't get in an infinite loop
    }
    end:
    dec(charObj);
  }
  B allNsFields(void);
  B str_norm(u32* chars, usz len, bool* upper) {
    B norm = emptyCVec();
    bool hadLetter = false;
    bool hasLower = false;
    bool hasUpper = false;
    for (usz i = 0; i < len; i++) {
      u32 c = chars[i]; if (c=='_') continue;
      bool low = chr_low(c);
      bool upp = chr_upp(c);
      if (low) hasLower = true;
      if (hadLetter) hasUpper|= upp;
      if (upp) c = chr_to_low(c);
      hadLetter = upp | low;
      norm = vec_addN(norm, m_c32(c));
    }
    *upper = !hasLower && hasUpper;
    return norm;
  }
  NOINLINE B vec_slice(B x, usz s, usz len) {
    return taga(arr_shVec(TI(x,slice)(incG(x), s, len)));
  }
  NOINLINE u32* rskip_name(u32* start, u32* c) {
    while (c>start && chr_nameM(*(c-1))) c--;
    return c;
  }
  NOINLINE void completion_impl(const char* inp, void* res, bool hint, int* dist) {
    B inpB = toC32Any(utf8Decode0(inp));
    u32* chars = c32any_ptr(inpB);
    u32* we = chars+IA(inpB);
    u32* ws = rskip_name(chars, we);
    bool sysval = ws>chars && U'•' == *(ws-1);
    bool nsField = ws>chars && '.' == *(ws-1);
    if (nsField) {
      u32* ws2 = rskip_name(chars, ws-1);
      if (ws2>chars && U'•' == *(ws2-1)) { ws=ws2; sysval=true; nsField=false; }
    }
    if (sysval) ws--;
    
    usz wl = *dist = we-ws;
    if (wl>0) {
      usz wo = ws-chars;
      bool doUpper;
      B norm = str_norm(ws, wl, &doUpper);
      B reg = sysval? vec_slice(inpB, wo, wl) : bi_N;
      usz normLen = IA(norm);
      if (normLen>0) {
        B vars = sysval? incG(sysvalNames) : nsField? allNsFields() : listVars(gsc);
        if (q_N(vars)) goto noVars;
        usz via=IA(vars); SGetU(vars)
        for (usz i = 0; i < via; i++) {
          i32 matchState=0; B match=bi_N; usz matchLen=0, skip=0;
          for (usz j = 0; j < (sysval? 2 : 1); j++) {
            match = j? harr_ptr(sysvalNamesNorm)[i] : GetU(vars,i);
            bool doNorm = sysval? j : true;
            matchLen = IA(match);
            usz wlen = doNorm? normLen : wl;
            if (wlen>=matchLen) continue;
            B slc = vec_slice(match, 0, wlen);
            bool matched = equal(slc, doNorm? norm : reg);
            decG(slc);
            if (matched) { matchState=doNorm?1:2; skip=wlen; break; }
          }
          
          if (matchState) {
            usz ext = matchLen-skip;
            assert(ext<100);
            u32* rp; B r = m_c32arrv(&rp, wl + ext);
            COPY_TO(rp, el_c32, 0, inpB, wo, wl);
            if (doUpper && matchState!=2) {
              SGetU(match)
              for (usz i = 0; i < ext; i++) {
                u32 cc=o2cG(GetU(match,i+skip));
                rp[wl+i] = chr_low(cc)? chr_to_upp(cc) : cc;
              }
            } else {
              COPY_TO(rp, el_c32, wl, match, skip, ext);
            }
            char* str = toCStr(r);
            if (hint) replxx_add_hint(res, str);
            else replxx_add_completion(res, str);
            freeCStr(str);
            decG(r);
          }
        }
        decG(vars);
        noVars:;
      }
      decG(norm);
      if (sysval) decG(reg);
    }
    
    dec(inpB);
  }
  void complete_replxx(const char* inp, replxx_completions* res, int* dist, void* data) {
    completion_impl(inp, res, false, dist);
  }
  void hint_replxx(const char* inp, replxx_hints* res, int* dist, ReplxxColor* c, void* data) {
    completion_impl(inp, res, true, dist);
  }
  static NOINLINE char* malloc_B(B x) { // toCStr but allocated by malloc; frees x
    u64 len1 = utf8lenB(x);
    char* s1 = malloc(len1+1);
    toUTF8(x, s1);
    decG(x);
    s1[len1] = '\0';
    return s1;
  }
  typedef struct {
    B s; u64 pos;
  } TmpState;
  
  static NOINLINE TmpState getState() {
    ReplxxState st;
    replxx_get_state(global_replxx, &st);
    return (TmpState){.s = utf8Decode0(st.text), .pos = st.cursorPosition};
  }
  static NOINLINE void setState(TmpState s) { // frees s.s
    char* r = malloc_B(s.s);
    ReplxxState st = (ReplxxState){.text = r, .cursorPosition = s.pos};
    replxx_set_state(global_replxx, &st);
    free(r);
  }
  static NOINLINE TmpState insertChar(u32 p, bool replace) {
    TmpState t = getState();
    u64 pos = t.pos;
    B s0 = t.s;
    if (pos==0) replace = false;
    
    B s = vec_slice(s0, 0, pos-(replace? 1 : 0));
    ACHR(p);
    AJOIN(vec_slice(s0, pos, IA(s0)-pos));
    decG(s0);
    
    return (TmpState){.s = s, .pos = replace? pos : pos+1};
  }
  static B b_pv;
  static int b_pp;
  static bool inBackslash() { return b_pv.u!=0; }
  static void stopBackslash() { decG(b_pv); b_pv.u = 0; }
  ReplxxActionResult backslash_replxx(int code, void* data) {
    if (inBackslash()) {
      setState(insertChar('\\', false));
      stopBackslash();
    } else if (!cfg_enableKeyboard) {
      setState(insertChar('\\', false));
    } else {
      ReplxxState st;
      replxx_get_state(global_replxx, &st);
      b_pv = utf8Decode0(st.text);
      b_pp = st.cursorPosition;
    }
    return REPLXX_ACTION_RESULT_CONTINUE;
  }
  ReplxxActionResult enter_replxx(int code, void* data) {
    if (inBackslash()) {
      setState(insertChar('\n', false));
      stopBackslash();
      return REPLXX_ACTION_RESULT_CONTINUE;
    }
    TmpState s = getState();
    s.pos = IA(s.s);
    setState(s);
    return REPLXX_ACTION_RESULT_RETURN;
  }
  static NOINLINE bool slice_equal(B a, usz as, B b, usz bs, usz l) {
    B ac = vec_slice(a, as, l);
    B bc = vec_slice(b, bs, l);
    bool r = equal(ac, bc);
    decG(ac); decG(bc);
    return r;
  }
  
  B indexOf_c2(B, B, B);
  B pick_c1(B, B);
  
  static B b_key, b_val;
  void modified_replxx(char** s_res, int* p_res, void* userData) {
    if (!cfg_enableKeyboard) return;
    if (!inBackslash()) return;
    
    TmpState t = getState();
    B s = t.s;
    u64 pos = t.pos;
    if (IA(b_pv)+1 != IA(s)  ||  b_pp+1 != pos) goto stop;
    if (!slice_equal(b_pv, 0,    s, 0,   pos-1)) goto stop;
    if (!slice_equal(b_pv, b_pp, s, pos, IA(s)-pos)) goto stop;
    usz mapPos = o2i(pick_c1(m_f64(0), indexOf_c2(m_f64(0), incG(b_key), IGet(s,pos-1))));
    if (mapPos==IA(b_key)) goto stop;
    
    TmpState t2 = insertChar(o2c(IGetU(b_val, mapPos)), true);
    *s_res = malloc_B(t2.s); // will be free()'d by replxx
    *p_res = t2.pos;
    
    stop: decG(s);
    stopBackslash();
  }
  
  
  void before_exit() {
    if (global_replxx!=NULL && global_histfile!=NULL) {
      replxx_history_save(global_replxx, global_histfile);
      replxx_end(global_replxx);
      global_replxx = NULL;
    }
  }
  
  static NOINLINE B path_rel_dec(B base, B rel) {
    B res = path_rel(base, rel);
    dec(base);
    return res;
  }
  static NOINLINE B get_config_path(bool inConfigDir, char* name) {
    char* history_dir = getenv("XDG_DATA_HOME");
    bool addConfig = false;
    if (!history_dir) { history_dir = getenv("HOME"); if (history_dir) addConfig = inConfigDir; }
    if (!history_dir) history_dir = ".";
    B p = utf8Decode0(history_dir);
    if (addConfig) p = path_rel_dec(p, m_c8vec_0(".config"));
    return path_rel_dec(p, m_c8vec_0(name));
  }
  
  static void replxx_gcFn() {
    mm_visit(b_pv);
  }
  static void cbqn_init_replxx() {
    B cfg = get_config_path(true, "cbqn_repl.txt");
    cfg_path = cfg; gc_add(cfg);
    
    if (path_type(incG(cfg_path))==0) {
      cfg_set_theme(1, false);
      cfg_set_keyboard(true, false);
    } else {
      B lns = path_lines(incG(cfg_path));
      SGetU(lns)
      for (usz i = 0; i < IA(lns); i++) {
        B ln = GetU(lns, i);
        char* s = toCStr(ln); char* e;
        if      (isCmd(s, &e, "theme="   )) cfg_theme          = e[0]-'0';
        else if (isCmd(s, &e, "keyboard=")) cfg_enableKeyboard = e[0]-'0';
        freeCStr(s);
      }
      decG(lns);
      cfg_set_theme(cfg_theme, false);
      cfg_set_keyboard(cfg_enableKeyboard, false);
    }
    
    gc_add(b_key = m_c32vec_0(U"\"`1234567890-=~!@#$%^&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:|zxcvbnm,./ZXCVBNM<>? "));
    gc_add(b_val = m_c32vec_0( U"˙˜˘¨⁼⌜´˝7∞¯•÷×¬⎉⚇⍟◶⊘⎊⍎⍕⟨⟩√⋆⌽𝕨∊↑∧y⊔⊏⊐π←→↙𝕎⍷𝕣⍋YU⊑⊒⍳⊣⊢⍉𝕤↕𝕗𝕘⊸∘○⟜⋄↩↖𝕊D𝔽𝔾«J⌾»·|⥊𝕩↓∨⌊n≡∾≍≠⋈𝕏C⍒⌈N≢≤≥⇐‿"));
    sysvalNames = emptyHVec();
    sysvalNamesNorm = emptyHVec();
    u32** c = dsv_text;
    while (*c) { bool unused;
      B str = m_c32vec_0(*(c++));
      sysvalNames = vec_addN(sysvalNames, str);
      sysvalNamesNorm = vec_addN(sysvalNamesNorm, str_norm(c32any_ptr(str), IA(str), &unused));
    }
    gc_add(sysvalNames);
    gc_add(sysvalNamesNorm);
    gc_addFn(replxx_gcFn);
  }
#else
  void before_exit() { }
#endif


static NOINLINE B gsc_exec_inline(B src, B path, B args) {
  Block* block = bqn_compSc(src, path, args, gsc, true);
  ptr_dec(gsc->body); // redirect new errors to the newly executed code; initial scope had 0 vars, so this is safe
  gsc->body = ptr_inc(block->bodies[0]);
  B r = execBlockInline(block, gsc);
  ptr_dec(block);
  return r;
}

bool profiler_alloc(void);
bool profiler_start(i64 hz);
bool profiler_stop(void);
void profiler_free(void);
void profiler_displayResults(void);
void clearImportCache(void);

static B escape_parser;
static B simple_unescape(B x) {
  if (RARE(escape_parser.u==0)) {
    escape_parser = bqn_exec(utf8Decode0("{m←\"Expected surrounding quotes\" ⋄ m!2≤≠𝕩 ⋄ m!\"\"\"\"\"\"≡0‿¯1⊏𝕩 ⋄ s←¬e←<`'\\'=𝕩 ⋄ i‿o←\"\\\"\"nr\"⋈\"\\\"\"\"∾@+10‿13 ⋄ 1↓¯1↓{n←i⊐𝕩 ⋄ \"Unknown escape\"!∧´n≠≠i ⋄ n⊏o}⌾((s/»e)⊸/) s/𝕩}"), bi_N, bi_N);
    gc_add(escape_parser);
  }
  return c1(escape_parser, x);
}

static NOINLINE i64 readInt(char** p) {
  char* c = *p;
  i64 am = 0;
  while (*c>='0' & *c<='9') {
    am = am*10 + (*c - '0');
    c++;
  }
  *p = c;
  return am;
}
void cbqn_runLine0(char* ln, i64 read) {
  if (ln[0]==0 || read==0) return;
  
  B code;
  int output; // 0-no; 1-formatter; 2-internal
  i32 time = 0;
  i64 profile = -1;
  if (ln[0] == ')') {
    char* cmdS = ln+1;
    char* cmdE;
    if (isCmd(cmdS, &cmdE, "ex ")) {
      B path = utf8Decode0(cmdE);
      code = path_chars(path);
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "r ")) {
      code = utf8Decode0(cmdE);
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "escaped ")) {
      B u = simple_unescape(utf8Decode0(cmdE));
      u64 len = utf8lenB(u);
      TALLOC(char, ascii, len+1);
      toUTF8(u, ascii);
      dec(u);
      ascii[len] = '\0';
      cbqn_runLine0(ascii, len+1);
      TFREE(ascii);
      return;
    } else if (isCmd(cmdS, &cmdE, "t ") || isCmd(cmdS, &cmdE, "time ")) {
      code = utf8Decode0(cmdE);
      time = -1;
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "profile ") || isCmd(cmdS, &cmdE, "profile@")) {
      char* cpos = cmdE;
      profile = '@'==*(cpos-1)? readInt(&cpos) : 5000;
      if (profile==0) { printf("Cannot profile with 0hz sampling frequency\n"); return; }
      if (profile>999999) { printf("Cannot profile with >999999hz frequency\n"); return; }
      code = utf8Decode0(cpos);
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "t:") || isCmd(cmdS, &cmdE, "time:")) {
      char* repE = cmdE;
      i64 am = readInt(&repE);
      if (repE==cmdE) { printf("time command not given repetition count\n"); return; }
      if (am==0) { printf("repetition count was zero\n"); return; }
      code = utf8Decode0(repE);
      time = am;
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "mem ")) {
      bool sizes = 0;
      bool types = 0;
      bool freed = 0;
      char c;
      while ((c=*(cmdE++)) != 0) {
        if (c=='t') types=1;
        if (c=='s') sizes=1;
        if (c=='f') freed=1;
      }
      heap_printInfo(sizes, types, freed);
      return;
    } else if (isCmd(cmdS, &cmdE, "erase ")) {
      char* name = cmdE;
      i64 len = strlen(name);
      ScopeExt* e = gsc->ext;
      if (e!=NULL) {
        i32 am = e->varAm;
        for (i32 i = 0; i < am; i++) {
          B c = e->vars[i+am];
          if (IA(c) != len) continue;
          SGetU(c)
          bool ok = true;
          for (i32 j = 0; j < len; j++) ok&= o2cG(GetU(c, j))==name[j];
          if (ok) {
            B val = e->vars[i];
            e->vars[i] = bi_noVar;
            dec(val);
            #if ENABLE_GC
              if (!gc_depth) gc_forceGC();
            #endif
            return;
          }
        }
      }
      printf("No such variable found\n");
      return;
    } else if (isCmd(cmdS, &cmdE, "clearImportCache ")) {
      clearImportCache();
      return;
#if USE_REPLXX
    } else if (isCmd(cmdS, &cmdE, "kb ")) {
      cfg_set_keyboard(!cfg_enableKeyboard, true);
      printf("Backslash input %s\n", cfg_enableKeyboard? "enabled" : "disabled");
      return;
    } else if (isCmd(cmdS, &cmdE, "theme ")) {
      if      (strcmp(cmdE,"dark" )==0) cfg_set_theme(1, true);
      else if (strcmp(cmdE,"light")==0) cfg_set_theme(2, true);
      else if (strcmp(cmdE,"none" )==0) cfg_set_theme(0, true);
      else printf("Unknown theme\n");
      return;
#endif
    } else if (isCmd(cmdS, &cmdE, "exit") || isCmd(cmdS, &cmdE, "off")) {
      bqn_exit(0);
    } else if (isCmd(cmdS, &cmdE, "vars")) {
      B r = listVars(gsc);
      if (q_N(r)) {
        printf("Couldn't list variables\n");
      } else {
        usz ia = IA(r);
        B* rp = harr_ptr(r);
        for (usz i = 0; i < ia; i++) {
          if (i!=0) printf(", ");
          printRaw(rp[i]);
        }
        putchar('\n');
      }
      return;
    } else if (isCmd(cmdS, &cmdE, "gc ")) {
      #if ENABLE_GC
        if (0==*cmdE) {
          if (gc_depth!=0) {
            printf("GC is disabled, but forcibly GCing anyway\n");
            gc_enable();
            gc_forceGC();
            gc_disable();
          } else {
            gc_forceGC();
          }
        } else if (strcmp(cmdE,"on")==0) {
          if (gc_depth==0) printf("GC already on\n");
          else if (gc_depth>1) printf("GC cannot be enabled\n");
          else {
            gc_enable();
            printf("GC enabled\n");
          }
        } else if (strcmp(cmdE,"off")==0) {
          if (gc_depth>0) printf("GC already off\n");
          else {
            gc_disable();
            printf("GC disabled\n");
          }
        } else printf("Unknown GC command\n");
      #else
        printf("Macro ENABLE_GC was false at compile-time, cannot GC\n");
      #endif
      return;
    } else if (isCmd(cmdS, &cmdE, "internalPrint ")) {
      code = utf8Decode0(cmdE);
      output = 2;
    } else if (isCmd(cmdS, &cmdE, "e ") || isCmd(cmdS, &cmdE, "explain ")) {
      B expl = bqn_explain(utf8Decode0(cmdE), replPath);
      HArr* expla = toHArr(expl);
      usz ia=PIA(expla);
      for(usz i=0; i<ia; i++) {
        printRaw(expla->a[i]);
        putchar('\n');
      }
      dec(expl);
      return;
    } else {
      printf("Unknown REPL command\n");
      return;
    }
  } else {
    code = utf8Decode0(ln);
    output = 1;
  }
  Block* block = bqn_compSc(code, incG(replPath), emptySVec(), gsc, true);
  
  ptr_dec(gsc->body);
  gsc->body = ptr_inc(block->bodies[0]);
  
  B res;
  if (time) {
    f64 t;
    if (time==-1) {
      u64 sns = nsTime();
      res = execBlockInline(block, gsc);
      u64 ens = nsTime();
      t = ens-sns;
    } else {
      u64 sns = nsTime();
      for (i64 i = 0; i < time; i++) dec(execBlockInline(block, gsc));
      u64 ens = nsTime();
      t = (ens-sns)*1.0 / time;
      res = m_c32(0);
    }
    if      (t<1e3) printf("%.5gns\n", t);
    else if (t<1e6) printf("%.4gus\n", t/1e3);
    else if (t<1e9) printf("%.4gms\n", t/1e6);
    else            printf("%.5gs\n", t/1e9);
  } else if (profile>0) {
    if (CATCH) { profiler_stop(); profiler_free(); rethrow(); }
    if (profiler_alloc() && profiler_start(profile)) {
      res = execBlockInline(block, gsc);
      profiler_stop();
      profiler_displayResults();
      profiler_free();
    }
    popCatch();
  } else res = execBlockInline(block, gsc);
  ptr_dec(block);
  
  if (output) {
    if (output!=2 && FORMATTER) {
      B resFmt = bqn_fmt(res);
      printRaw(resFmt); dec(resFmt);
      putchar('\n');
    } else {
      print(res); putchar('\n'); fflush(stdout);
      dec(res);
    }
  } else dec(res);
  
}

void cbqn_runLine(char* ln, i64 len) {
  if(CATCH) {
    cbqn_takeInterrupts(false);
    fprintf(stderr, "Error: "); printErrMsg(thrownMsg); fputc('\n', stderr);
    vm_pst(envCurr+1, envStart+envPrevHeight);
    freeThrown();
    #ifdef HEAP_VERIFY
      heapVerify();
    #endif
    gc_maybeGC();
    return;
  }
  cbqn_takeInterrupts(true);
  cbqn_runLine0(ln, len);
  #ifdef HEAP_VERIFY
    heapVerify();
  #endif
  gc_maybeGC();
  cbqn_takeInterrupts(false);
  popCatch();
}

#if WASM
void cbqn_evalSrc(char* src, i64 len) {
  B code = utf8Decode(src, len);
  B res = bqn_exec(code, bi_N, bi_N);
  
  B resFmt = bqn_fmt(res);
  printRaw(resFmt); dec(resFmt);
  putchar('\n');
}
#endif

#if EMCC
int main() {
  repl_init();
}
#elif !CBQN_SHARED
int main(int argc, char* argv[]) {
  repl_init();
  bool startREPL = argc==1;
  bool silentREPL = false;
  bool execStdin = false;
  if (!startREPL) {
    i32 i = 1;
    while (i!=argc) {
      char* carg = argv[i];
      if (carg[0]!='-') break;
      if (carg[1]==0) { execStdin=true; i++; break; }
      i++;
      if (carg[1]=='-') {
        if (!strcmp(carg, "--help")) {
          print_help:
          printf(
          "Usage: %s [options] [file.bqn [arguments]]\n"
          "Options:\n"
          "-f file: execute the contents of the file with all further arguments as •args\n"
          "-e code: execute the argument as BQN\n"
          "-p code: execute the argument as BQN and print its result pretty-printed\n"
          "-o code: execute the argument as BQN and print its raw result\n"
          "-M num : set maximum heap size to num megabytes\n"
          "-r     : start the REPL after executing all arguments\n"
          "-s     : start a silent REPL\n"
          , argv[0]);
          exit(0);
        } else {
          printf("%s: Unknown option: %s\n", argv[0], carg);
          exit(1);
        }
      } else {
        carg++;
        char c;
        while ((c = *carg++)) {
          switch(c) { default: fprintf(stderr, "%s: Unknown option: -%c\n", argv[0], c); exit(1);
            #define REQARG(X) if(*carg) { fprintf(stderr, "%s: -%s must end the option\n", argv[0], #X); exit(1); } if (i==argc) { fprintf(stderr, "%s: -%s requires an argument\n", argv[0], #X); exit(1); }
            case 'f': repl_init(); REQARG(f); goto execFile;
            case 'e': { repl_init(); REQARG(e);
              dec(gsc_exec_inline(utf8Decode0(argv[i++]), m_c8vec_0("(-e)"), emptySVec()));
              break;
            }
            case 'L': { repl_init(); break; } // just initialize. mostly for perf testing
            case 'p': { repl_init(); REQARG(p);
              B r = bqn_fmt(gsc_exec_inline(utf8Decode0(argv[i++]), m_c8vec_0("(-p)"), emptySVec()));
              printRaw(r); dec(r);
              printf("\n");
              break;
            }
            case 'o': { repl_init(); REQARG(o);
              B r = gsc_exec_inline(utf8Decode0(argv[i++]), m_c8vec_0("(-o)"), emptySVec());
              printRaw(r); dec(r);
              printf("\n");
              break;
            }
            case 'M': { REQARG(M);
              char* str = argv[i++];
              u64 am = 0;
              while (*str) {
                char c = *str++;
                if (c<'0' | c>'9') { printf("%s: -M: Argument not a number\n", argv[0]); exit(1); }
                if (am>1ULL<<48) { printf("%s: -M: Too large\n", argv[0]); exit(1); }
                am = am*10 + c-48;
              }
              mm_heapMax = am*1024*1024;
              break;
            }
            #ifdef PERF_TEST
            case 'R': { repl_init(); REQARG(R);
              B path = utf8Decode0(argv[i++]);
              B lines = path_lines(path);
              usz ia = IA(lines);
              SGet(lines)
              for (u64 i = 0; i < ia; i++) {
                dec(gsc_exec_inline(Get(lines, i), incG(replPath), emptySVec()));
              }
              break;
            }
            #endif
            case 'h': goto print_help;
            case 'r': { startREPL=true;                  break; }
            case 's': { startREPL=true; silentREPL=true; break; }
          }
        }
      }
    }
    execFile:
    if (i!=argc || execStdin) {
      repl_init();
      B src;
      if (!execStdin) src = utf8Decode0(argv[i++]);
      B args;
      if (i==argc) {
        args = emptySVec();
      } else {
        M_HARR(ap, argc-i)
        for (usz j = 0; j < argc-i; j++) HARR_ADD(ap, j, utf8Decode0(argv[i+j]));
        args = HARR_FV(ap);
      }
      
      B execRes;
      if (execStdin) {
        execRes = gsc_exec_inline(utf8DecodeA(stream_bytes(stdin)), m_c8vec_0("(-)"), args);
      } else {
        execRes = bqn_execFile(src, args);
      }
      dec(execRes);
      #ifdef HEAP_VERIFY
        heapVerify();
      #endif
      gc_forceGC();
    }
  }
  if (startREPL) {
    repl_init();
    #if USE_REPLXX
    if (!silentREPL) {
      cbqn_init_replxx();
      Replxx* replxx = replxx_init();
      
      B f = get_config_path(false, ".cbqn_repl_history");
      char* histfile = toCStr(f);
      dec(f);
      gc_add(tag(TOBJ(histfile), OBJ_TAG));
      replxx_history_load(replxx, histfile);
      global_replxx = replxx;
      global_histfile = histfile;
      
      replxx_set_ignore_case(replxx, true);
      replxx_set_highlighter_callback(replxx, highlighter_replxx, NULL);
      replxx_set_hint_callback(replxx, hint_replxx, NULL);
      replxx_set_completion_callback(replxx, complete_replxx, NULL);
      replxx_enable_bracketed_paste(replxx);
      replxx_bind_key(replxx, '\\', backslash_replxx, NULL);
      replxx_bind_key(replxx, REPLXX_KEY_ENTER, enter_replxx, NULL);
      replxx_set_modify_callback(replxx, modified_replxx, NULL);
      replxx_bind_key_internal(replxx, REPLXX_KEY_CONTROL('N'), "history_next");
      replxx_bind_key_internal(replxx, REPLXX_KEY_CONTROL('P'), "history_previous");
      
      while(true) {
        const char* ln = replxx_input(replxx, "   ");
        if (ln==NULL) {
          if (errno==0) printf("\n");
          break;
        }
        replxx_history_add(replxx, ln);
        cbqn_runLine((char*)ln, strlen(ln));
        replxx_history_save(replxx, histfile);
      }
    }
    else
    #endif
    {
      while (true) {
        if (!silentREPL) {
          printf("   ");
          fflush(stdout);
        }
        char* ln = NULL;
        size_t gl = 0;
        i64 read = getline(&ln, &gl, stdin);
        if (read<=0 || ln[0]==0) { if(!silentREPL) putchar('\n'); break; }
        if (ln[read-1]==10) ln[--read] = 0;
        if (ln[read-1]==13) ln[--read] = 0;
        cbqn_runLine(ln, read);
        free(ln);
      }
    }
  }
  #ifdef HEAP_VERIFY
    heapVerify();
  #endif
  bqn_exit(0);
  #undef INIT
}
#endif
