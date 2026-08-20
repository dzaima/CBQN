#include "core.h"
#include "vm/vm.h"
#include "vm/load.h"
#include "core/ns.h"
#include "utils/utf.h"
#include "utils/talloc.h"
#include "utils/file.h"
#include "utils/time.h"
#include "utils/interrupt.h"
#include "utils/toplevel.h"
#include "builtins/ffi/ffi.h"

#if defined(_WIN32) || defined(_WIN64)
  #include "windows/getline.h"
  #include "windows/utf16.h"
#endif


#if __GNUC__ <= 14 && __i386__ && !__clang__
  #warning "CBQN is known to miscompile on GCC for 32-bit x86 builds; use clang or gcc-15 or newer"
#endif
#if USE_REPLXX_IO && !USE_REPLXX
  #error "Cannot use USE_REPLXX_IO without USE_REPLXX"
#endif

#define RUN_START Run curr_run_ = run_start();
#define RUN_END run_end(curr_run_)

STATIC_GLOBAL B replPath;
STATIC_GLOBAL B replName;
STATIC_GLOBAL Scope* gsc;
STATIC_GLOBAL bool repl_initialized = false;

static NOINLINE void repl_init() {
  if (repl_initialized) return;
  cbqn_init();
  replPath = gc_add(m_c8vec_0("."));
  replName = gc_add(m_c8vec_0("(REPL)"));
  Body* body = m_nnsDesc();
  B ns = m_nns(body);
  gsc = ptr_inc(c(NS, ns)->sc); gc_add(tag(gsc,OBJ_TAG));
  ptr_dec(v(ns));
  repl_initialized = true;
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

static NOINLINE u64 readu64part(char** p) { // writes NULL to *p on too-large or missing number
  char* c = *p;
  u64 am = 0;
  while (*c>='0' & *c<='9') {
    if (am >= U64_MAX/10) { *p = NULL; return 0; }
    am = am*10 + (*c - '0');
    c++;
  }
  *p = c==*p? NULL : c;
  return am;
}



#if USE_REPLXX
  #include <replxx.h>
  #include <errno.h>
  #include "utils/cstr.h"
  
  static i8 const themes[3][12][3] = {
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
  STATIC_GLOBAL ReplxxColor theme_replxx[12];
  
  STATIC_GLOBAL u32 replcfg_prefixChar = U'\\';
  STATIC_GLOBAL i32 replcfg_theme = 1;
  STATIC_GLOBAL bool replcfg_enableKeyboard = true;
  STATIC_GLOBAL B replcfg_path;
  
  
  INIT_GLOBAL u8 repl_historyMode = 2; // 0: don't load or save; 1: don't save; 2: load and save
  STATIC_GLOBAL char* repl_histfile = NULL;
  GLOBAL Replxx* global_replxx;
  
  static char* const command_completion[] = {
    ")ex ",
    ")r ",
    ")escaped ",
    ")profile ", ")profile@",
    ")t ", ")t:", ")time ", ")time:",
    ")mem", ")mem t", ")mem s", ")mem f", ")mem log",
    ")erase ",
    ")clearImportCache",
    ")kb",
    ")theme dark", ")theme light", ")theme none",
    ")exit", ")off",
    ")vars",
    ")gc", ")gc off", ")gc on", ")gc log",
    ")internalPrint ", ")heapdump",
#if NATIVE_COMPILER && !ONLY_NATIVE_COMP
    ")switchCompiler",
#endif
    ")e ", ")explain ",
  };
  
  NOINLINE void cfg_changed(void) {
    B s = emptyCVec();
    AFMT("theme=%i\nkeyboard=%i\nprefix=%i\n", replcfg_theme, replcfg_enableKeyboard, replcfg_prefixChar);
    if (CATCH) { freeThrown(); goto end; }
    path_wChars(incG(replcfg_path), s);
    popCatch();
    end: decG(s);
  }
  
  NOINLINE void cfg_set_theme(i32 num, bool writeCfg) {
    if (num>=3 || num<0) return;
    replcfg_theme = num;
    i8 const (*data)[3] = themes[num];
    for (int i = 0; i < 12; i++) {
      i8 v0 = data[i][0];
      i8 v1 = data[i][1];
      i8 v2 = data[i][2];
      theme_replxx[i] = v0>=0? replxx_color_rgb666(v0,v1,v2) : v2>=0? replxx_color_grayscale(v2) : REPLXX_COLOR_DEFAULT;
    }
    if (writeCfg) cfg_changed();
  }
  void cfg_set_keyboard(bool enable, bool writeCfg) {
    replcfg_enableKeyboard = enable;
    if (writeCfg) cfg_changed();
  }
  
  extern INIT_GLOBAL u32* const dsv_text[];
  STATIC_GLOBAL B sysvalNames, sysvalNamesNorm;
  
  NOINLINE void fill_color(ReplxxColor* cols, int s, int e, ReplxxColor col) {
    PLAINLOOP for (int i = s; i < e; i++) cols[i] = col;
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
  static u32* const chrs_fn = U"!+-×÷⋆*√⌊⌈∧∨¬|=≠≤<>≥≡≢⊣⊢⥊∾≍⋈↑↓↕⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔«»𝔽𝔾𝕎𝕏𝕊";
  static u32* const chrs_m1 = U"`˜˘¨⁼⌜´˝˙";
  static u32* const chrs_m2 = U"∘⊸⟜○⌾⎉⚇⍟⊘◶⎊";
  static u32* const chrs_lit = U"⟨⟩[]·@‿";
  static u32* const chrs_dmd = U"←↩,⋄→⇐";
  static u32* const chrs_blk = U"{}𝕨𝕩𝕗𝕘𝕣𝕤:?;";
  NOINLINE bool chr_in(u32 val, u32* chrs) {
    while(*chrs) if (val == *(chrs++)) return true;
    return false;
  }
  
  #define CATCH_OOM(X) if (CATCH) { freeThrown(); X; }
  void highlighter_replxx(const char* input, ReplxxColor* colors, int size, void* data) {
    CATCH_OOM(return)
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
    decG(charObj);
    popCatch();
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
    if (!*inp) return;
    
    CATCH_OOM(return)
    B inpB = toC32Any(utf8Decode0(inp));
    u32* chars = c32any_ptr(inpB);
    u32* ws;
    u32* we;
    i32 mode; // 0:var 1:sys 2:ns 3:cmd
    if (chars[0] == ')') {
      ws = chars;
      we = chars+IA(inpB);
      mode = 3;
    } else {
      not_cmd:;
      we = chars+IA(inpB);
      ws = rskip_name(chars, we);
      u32 last = ws>chars? *(ws-1) : 0;
      mode = last=='.'? 2 : last==U'•'? 1 : 0;
      
      if (mode==2) {
        u32* ws2 = ws;
        while (--ws2 > chars && *ws2=='.') {
          u32* ws3 = rskip_name(chars, ws2);
          if (ws2==ws3 || ws3==chars) break;
          ws2 = ws3;
          
          u32 last2 = *(ws2-1);
          if (last2==U'•') { ws=ws2; mode=1; break; }
        }
      }
      
      if (mode==1) ws--;
    }
    
    usz wl = *dist = we-ws;
    bool failedCmd = mode==3;
    if (wl>0) {
      usz wo = ws-chars;
      bool doUpper = false;
      B norm = mode==3? incG(inpB) : str_norm(ws, wl, &doUpper);
      B reg = mode==1? vec_slice(inpB, wo, wl) : bi_N;
      usz normLen = IA(norm);
      if (normLen>0) {
        B vars;
        
        switch (mode) {
          case 0: vars = listVars(gsc); break;
          case 1: vars = incG(sysvalNames); break;
          case 2: vars = allNsFields(); break;
          case 3: {
            usz n = sizeof(command_completion)/sizeof(char*);
            HArr_p vo = m_harr0v(n);
            for (ux i = 0; i < n; i++) vo.a[i] = m_c8vec_0(command_completion[i]);
            vars = vo.b;
          }
        }
        
        if (q_N(vars)) goto noVars;
        usz via=IA(vars); SGetU(vars)
        for (usz i = 0; i < via; i++) {
          i32 matchState=0; B match=bi_N; usz matchLen=0, skip=0;
          for (usz j = 0; j < (mode==1? 2 : 1); j++) {
            match = j? harr_ptr(sysvalNamesNorm)[i] : GetU(vars,i);
            bool doNorm = mode==1? j : true;
            matchLen = IA(match);
            usz wlen = doNorm? normLen : wl;
            if (wlen>=matchLen) continue;
            B slc = vec_slice(match, 0, wlen);
            bool matched = equal(slc, doNorm? norm : reg);
            decG(slc);
            if (matched) { matchState=doNorm?1:2; skip=wlen; break; }
          }
          
          if (matchState) {
            failedCmd = false;
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
      if (mode==1) decG(reg);
      if (failedCmd) goto not_cmd;
    }
    
    decG(inpB);
    popCatch();
  }
  void complete_replxx(const char* inp, replxx_completions* res, int* dist, void* data) {
    completion_impl(inp, res, false, dist);
  }
  void hint_replxx(const char* inp, replxx_hints* res, int* dist, ReplxxColor* c, void* data) {
    completion_impl(inp, res, true, dist);
  }
  static NOINLINE char* malloc_B(B x) { // toCStr but allocated by malloc; consumes x
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
  static NOINLINE void setState(TmpState s) { // consumes s.s
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
  STATIC_GLOBAL B b_pv;
  STATIC_GLOBAL int b_pp;
  STATIC_GLOBAL bool inBackslash;
  static void stopBackslash() { inBackslash = false; }
  NOINLINE void setPrev(B s, u64 pos) { // consumes
    decG(b_pv);
    b_pv = s;
    b_pp = pos;
  }
  NOINLINE void setPrevNow() {
    ReplxxState st;
    replxx_get_state(global_replxx, &st);
    setPrev(utf8Decode0(st.text), st.cursorPosition);
  }
  ReplxxActionResult enter_replxx(int code, void* data) {
    if (inBackslash) {
      setState(insertChar('\n', false));
      stopBackslash();
      setPrevNow();
      return REPLXX_ACTION_RESULT_CONTINUE;
    }
    return replxx_invoke(global_replxx, REPLXX_ACTION_COMMIT_LINE, 0);
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
  
  STATIC_GLOBAL B b_key, b_val;
  void modified_replxx(char** s_res, int* p_res, void* userData) {
    if (!replcfg_enableKeyboard) return;
    CATCH_OOM(return)
    TmpState t = getState();
    B s = t.s;
    u64 pos = t.pos;
    if (equal(b_pv, s) && pos==b_pp) goto end_withPrev; // sometimes this is called when nothing actually changed
    
    if (IA(b_pv)+1 != IA(s)  ||  b_pp+1 != pos) goto stop; // user did something other than type a single character
    
    if (inBackslash) {
      if (!slice_equal(b_pv, 0,    s, 0,   pos-1)) goto stop;
      if (!slice_equal(b_pv, b_pp, s, pos, IA(s)-pos)) goto stop;
      if (o2cG(IGet(s,pos-1)) == replcfg_prefixChar) goto stop; // always make sure that the prefix char typed in twice types in the prefix char
      usz mapPos = o2i(C1(pick, C2(indexOf, incG(b_key), IGet(s,pos-1))));
      if (mapPos==IA(b_key)) goto stop;
      
      TmpState t2 = insertChar(o2c(IGetU(b_val, mapPos)), true);
      *s_res = malloc_B(t2.s); // will be free()'d by replxx
      *p_res = t2.pos;
      goto stop;
    } else {
      if (o2cG(IGetU(s,b_pp))==replcfg_prefixChar) {
        inBackslash = true;
        *s_res = malloc_B(incG(b_pv)); // will be free()'d by replxx
        *p_res = b_pp;
        // restore state to previous one, and don't call setPrev as that'll set it to the updated one
        goto end_noPrev;
      }
      goto end_withPrev;
    }
    
    stop:
    stopBackslash();
    end_withPrev:
    setPrevNow();
    end_noPrev:
    decG(s);
    popCatch();
  }
  
  static NOINLINE B path_rel_dec(B base, B rel) {
    B res = path_rel(base, rel, "main.c");
    dec(base);
    return res;
  }
  static NOINLINE B get_config_path(bool inConfigDir, char* name) {
  #ifdef _WIN32
    (void) inConfigDir;
    WCHAR* local_app_data = _wgetenv(L"LOCALAPPDATA");
    B p;
    if (!local_app_data) {
      p = m_c8vec_0(".");
    } else {
      p = path_rel_dec(utf16Decode0(local_app_data), m_c8vec_0("CBQN"));
      // create the directory if it doesn't exist
      if (path_type(incG(p))==0 && !dir_create(p)) { decG(p); p = m_c8vec_0("."); }
    }
  #else
    char* history_dir = getenv("XDG_DATA_HOME");
    bool addConfig = false;
    if (!history_dir) { history_dir = getenv("HOME"); if (history_dir) addConfig = inConfigDir; }
    if (!history_dir) history_dir = ".";
    B p = utf8Decode0(history_dir);
    if (addConfig) p = path_rel_dec(p, m_c8vec_0(".config"));
  #endif
    return path_rel_dec(p, m_c8vec_0(name));
  }
  
  static void cbqn_init_replxx() {
    B cfg = get_config_path(true, "cbqn_repl.txt");
    replcfg_path = gc_add(cfg);
    
    if (path_type(incG(replcfg_path))==0) {
      cfg_set_theme(1, false);
      cfg_set_keyboard(true, false);
    } else {
      B lns = path_lines(incG(replcfg_path));
      SGetU(lns)
      for (usz i = 0; i < IA(lns); i++) {
        B ln = GetU(lns, i);
        char* s = toCStr(ln); char* e;
        if      (isCmd(s, &e, "theme="   )) replcfg_theme          = e[0]-'0';
        else if (isCmd(s, &e, "keyboard=")) replcfg_enableKeyboard = e[0]-'0';
        #if !NO_RYU
        else if (isCmd(s, &e, "prefix="  )) replcfg_prefixChar     = readu64part(&e);
        #endif
        freeCStr(s);
      }
      decG(lns);
      cfg_set_theme(replcfg_theme, false);
      cfg_set_keyboard(replcfg_enableKeyboard, false);
    }
    
    b_key = gc_add(m_c32vec_0(U"\"`1234567890-=~!@#$%^&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:|zxcvbnm,./ZXCVBNM<>? "));
    b_val = gc_add(m_c32vec_0( U"˙˜˘¨⁼⌜´˝7∞¯•÷×¬⎉⚇⍟◶⊘⎊⍎⍕⟨⟩√⋆⌽𝕨∊↑∧y⊔⊏⊐π←→↙𝕎⍷𝕣⍋YU⊑⊒⍳⊣⊢⍉𝕤↕𝕗𝕘⊸∘○⟜⋄↩↖𝕊D𝔽𝔾«J⌾»·|⥊𝕩↓∨⌊n≡∾≍≠⋈𝕏C⍒⌈N≢≤≥⇐‿"));
    sysvalNames = emptyHVec();
    sysvalNamesNorm = emptyHVec();
    u32* const* c = dsv_text;
    while (*c) { bool unused;
      B str = m_c32vec_0(*(c++));
      sysvalNames = vec_addN(sysvalNames, str);
      sysvalNamesNorm = vec_addN(sysvalNamesNorm, str_norm(c32any_ptr(str), IA(str), &unused));
    }
    gc_add(sysvalNames);
    gc_add(sysvalNamesNorm);
    gc_add_ref(&b_pv);
    
    global_replxx = replxx_init();
    b_pv = emptyCVec();
    b_pp = 0;
  }
  const char* cbqn_replxx_input(char* prefix) {
    setPrev(emptyCVec(), 0);
    replxx_enable_bracketed_paste(global_replxx);
    const char* r = replxx_input(global_replxx, prefix);
    replxx_disable_bracketed_paste(global_replxx);
    return r;
  }
#endif

B m_state(B path, B name, B args);
NOINLINE B gsc_exec_inplace(B src, char* name, B args) {
  Block* block = bqn_comp(src, m_state(incG(replPath), m_c8vec_0(name), args), def_re, gsc, COMP_UNK, true, false);
  ptr_dec(gsc->body); // redirect new errors to the newly executed code; initial scope had 0 vars, so this is safe
  gsc->body = ptr_inc(block->bodies[0]);
  B r = execBlockInplace(block, gsc);
  ptr_dec(block);
  return r;
}

bool profiler_alloc(void);
bool profiler_start(i32 mode, i64 hz);
bool profiler_stop(void);
void profiler_free(void);
void profiler_displayResults(FILE* f);
void clearImportCache(void);

#if NATIVE_COMPILER && !ONLY_NATIVE_COMP
void switchComp(void);
#endif

STATIC_GLOBAL B escape_parser;
static B simple_unescape(B x) {
  if (RARE(escape_parser.u == 0)) {
    escape_parser = bqn_exec(utf8Decode0("{m←\"Expected surrounding quotes\" ⋄ m!2≤≠𝕩 ⋄ m!\"\"\"\"\"\"≡0‿¯1⊏𝕩 ⋄ s←¬e←<`'\\'=𝕩 ⋄ i‿o←\"\\\"\"nr\"⋈\"\\\"\"\"∾@+10‿13 ⋄ 1↓¯1↓{n←i⊐𝕩 ⋄ \"Unknown escape\"!∧´n≠≠i ⋄ n⊏o}⌾((s/»e)⊸/) s/𝕩}"), bi_N);
    gc_add(escape_parser);
  }
  return c1(escape_parser, x);
}

static NOINLINE void printTime(f64 ns) {
  if      (ns<1e3) printf("%.5gns\n", ns);
  else if (ns<1e6) printf("%.4gus\n", ns/1e3);
  else if (ns<1e9) printf("%.4gms\n", ns/1e6);
  else             printf("%.5gs\n", ns/1e9);
}
static NOINLINE f64 timeBlockN(Block* block, i64 rep) {
  u64 sns = nsTime();
  for (i64 i = 0; i < rep; i++) dec(execBlockInplace(block, gsc));
  u64 ens = nsTime();
  return ens-sns;
}
#if !NO_RYU
bool ryu_s2d_n(u8* buffer, int len, f64* result);
#endif

#define DEFAULT_PROFILE_SAMPLERATE 5000
void heap_printInfoStr(char* str);
extern GLOBAL bool cfg_gcLog, cfg_memLog;
void cbqn_runLine0(char* ln, i64 read) {
  if (ln[0]==0 || read==0) return;
  
  B code;
  int output; // 0-no; 1-formatter; 2-internal
  int mode = 0; // 0: regular execution; 1: single timing; 2: many timings; 3: second-limited timing; 4: profile; 4: profile ip
  i64 timeRep = 0;
  f64 timeNanos = -1;
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
      mode = 1;
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "profile ") || isCmd(cmdS, &cmdE, "profile@")) {
      mode = 4;
      goto profile_init; profile_init:;
      char* cpos = cmdE;
      profile = '@'==*(cpos-1)? readu64part(&cpos) : DEFAULT_PROFILE_SAMPLERATE;
      if (cpos == NULL) { printf("Invalid profile sample rate value\n"); return; }
      code = utf8Decode0(cpos);
      output = 0;
#if PROFILE_IP
    } else if (isCmd(cmdS, &cmdE, "profileip ") || isCmd(cmdS, &cmdE, "profileip@")) {
      mode = 5;
      goto profile_init;
#endif
    } else if (isCmd(cmdS, &cmdE, "t:") || isCmd(cmdS, &cmdE, "time:")) {
      char* repE = cmdE;
      mode = 2;
      #if NO_RYU
        timeRep = readInt(&repE);
      #else
        while ((*repE>='0' & *repE<='9') || *repE=='.' || *repE=='e') repE++;
        ux chrs = repE-cmdE;
        if (chrs >= 40) { largeRep: printf("Timing repetition count too large\n"); return; }
        if (chrs == 0) { printf("Timing repetition count not provided\n"); return; }
        f64 am;
        if (!ryu_s2d_n((u8*)cmdE, chrs, &am)) { printf("Bad timing repetition count\n"); return; }
        if (am==0) { printf("Timing repetition count cannot be zero\n"); return; }
        if (*repE == 's') { repE++; mode=3; timeNanos=am*1e9; }
        else if (!q_fi64o(&timeRep, am)) { if (am>1e18) goto largeRep; else printf("Timing repetition count must be an integer\n"); return; }
      #endif
      code = utf8Decode0(repE);
      output = 0;
    } else if (isCmd(cmdS, &cmdE, "mem ")) {
      if (strcmp(cmdE,"log")==0) {
        cfg_memLog^= true;
        printf("Allocation logging %s\n", cfg_memLog? "enabled" : "disabled");
      } else {
        heap_printInfoStr(cmdE);
      }
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
              if (!gc_depth) gc_forceGC(true);
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
    } else if (isCmd(cmdS, &cmdE, "heapdump ")) {
      cbqn_heapDump(NULL);
      return;
#if USE_REPLXX
    } else if (isCmd(cmdS, &cmdE, "kb ")) {
      if (*cmdE == 0) {
        kb_toggle:
        cfg_set_keyboard(!replcfg_enableKeyboard, true);
        printf("Backslash input %s\n", replcfg_enableKeyboard? "enabled" : "disabled");
      } else {
        B a = utf8Decode0(cmdE);
        if (IA(a)==0) goto kb_toggle;
        if (IA(a)!=1) {
          printf("Invalid )kb usage: expected either no argument, or a single character\n");
        } else {
          u32 c = o2cG(IGetU(a,0));
          printf("Backslash input prefix character set to U+%04x\n", c);
          replcfg_prefixChar = c;
          cfg_changed();
        }
        decG(a);
      }
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
          printsB(rp[i]);
        }
        printf("\n");
      }
      return;
    } else if (isCmd(cmdS, &cmdE, "gc ")) {
      #if ENABLE_GC
        bool toplevel = true;
        if (0==*cmdE) {
          gc_now:;
          if (gc_depth!=0) {
            printf("GC is disabled, but forcibly GCing anyway\n");
            u64 stored_depth = gc_depth;
            gc_depth = 0;
            gc_forceGC(toplevel);
            gc_depth = stored_depth;
          } else {
            gc_forceGC(toplevel);
          }
        } else if (strcmp(cmdE,"nontop")==0) {
          toplevel = false;
          goto gc_now;
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
        } else if (strcmp(cmdE,"log")==0) {
          cfg_gcLog^= true;
          printf("GC logging %s\n", cfg_gcLog? "enabled" : "disabled");
        } else printf("Unknown GC command\n");
      #else
        printf("Macro ENABLE_GC was false at compile-time, cannot GC\n");
      #endif
      return;
    } else if (isCmd(cmdS, &cmdE, "internalPrint ")) {
      code = utf8Decode0(cmdE);
      output = 2;
#if NATIVE_COMPILER && !ONLY_NATIVE_COMP
    } else if (isCmd(cmdS, &cmdE, "switchCompiler")) {
      switchComp();
      return;
#endif
    } else if (isCmd(cmdS, &cmdE, "e ") || isCmd(cmdS, &cmdE, "explain ")) {
      B vars = listVars(gsc);
      if (q_N(vars)) vars = emptyHVec();
      HArr* expla = toHArr(bqn_explain(utf8Decode0(cmdE), vars));
      usz ia=PIA(expla);
      for(usz i=0; i<ia; i++) {
        printsB(expla->a[i]);
        printf("\n");
      }
      ptr_dec(expla);
      return;
    } else {
      printf("Unknown REPL command\n");
      return;
    }
  } else {
    code = utf8Decode0(ln);
    output = 1;
  }
  Block* block = bqn_comp(code, m_state(incG(replPath), incG(replName), emptySVec()), def_re, gsc, COMP_REPL, true, false);
  
  ptr_dec(gsc->body);
  gsc->body = ptr_inc(block->bodies[0]);
  
  B res;
  if (mode==1) {
    u64 sns = nsTime();
    res = execBlockInplace(block, gsc);
    u64 ens = nsTime();
    printTime(ens-sns);
  } else if (mode==2) {
    printTime(timeBlockN(block, timeRep) / timeRep);
    res = m_c32(0);
  } else if (mode==3) {
    f64 tns = 0; // total nanoseconds timed
    u64 rt = 0; // count of runs of those nanoseconds
    u64 r1 = 1; // run count to attempt next
    while (1) {
      // printf(N64d"\n", r1);
      f64 ct = timeBlockN(block, r1);
      tns+= ct;
      rt+= r1;
      timeNanos-= ct;
      if (timeNanos<=0) break;
      u64 r2 = timeNanos*r1/ct;
      if      (r2 <= r1/8) { r1/=8; if (!r1) r1=1; }
      else if (r2 >= r1*2) { r1*=2; }
      else r1 = r2;
    }
    printTime(tns / rt);
    res = m_c32(0);
  } else if (mode==4 || mode==5) {
    if (CATCH) { profiler_stop(); profiler_free(); rethrow(); }
    if (profiler_alloc() && profiler_start(mode==5? 2 : 1, profile)) {
      res = execBlockInplace(block, gsc);
      profiler_stop();
      profiler_displayResults(stderr);
      profiler_free();
    }
    popCatch();
  } else {
    res = execBlockInplace(block, gsc);
  }
  
  ptr_dec(block);
  
  if (output) {
    if (output!=2 && FORMATTER) {
      B resFmt = bqn_fmt(res);
      printsB(resFmt); dec(resFmt);
      printf("\n");
    } else {
      printI(res); printf("\n"); fflush(stdout);
      dec(res);
    }
  } else dec(res);
  
}

void cbqn_runLine(char* ln, i64 len) {
  #if DEBUG
  ux cfh = cfHeight();
  #endif
  Run e = run_start();
  if(CATCH) {
    cbqn_takeInterrupts(false);
    fprintf(stderr, "Error: "); printErrMsg(thrownMsg); fprintf(stderr, "\n");
    vm_pst(envCurr+1, envStart+envPrevHeight);
    freeThrown();
    #if HEAP_VERIFY
      cbqn_heapVerify();
    #endif
    run_end(e);
    return;
  }
  cbqn_takeInterrupts(true);
  cbqn_runLine0(ln, len);
  #if HEAP_VERIFY
    cbqn_heapVerify();
  #endif
  run_end(e);
  cbqn_takeInterrupts(false);
  popCatch();
  debug_assert(cfh == cfHeight());
}

#if WASM
void cbqn_evalSrc(char* src, i64 len) {
  Run e = run_start();
  B code = utf8Decode(src, len);
  B resFmt = bqn_fmt(bqn_exec(code, bi_N));
  printsB(resFmt); dec(resFmt);
  printf("\n");
  run_end(e);
}
#endif

#if TOPLEVEL_GC
  GLOBAL ux run_nesting;
  GLOBAL ux run_countdown = 1;
  void run_slow() {
    gc_maybeGC(true);
    // gc_maybeGC is mildly-expensive even when it doesn't GC (has to sum up 64 or 128 `u64`s) so don't do it frequently
    // primarily rely on non-toplevel GC setting countdown to 1
    run_countdown = 64;
  }
#endif

INIT_GLOBAL FILE* log_file;
STATIC_GLOBAL bool cfg_profileGlobal;

#if EMCC
int main() {
  repl_init();
}
#elif !CBQN_LIB

#if HAS_VERSION
  extern char* const cbqn_versionInfo;
#endif
extern GLOBAL bool cfg_fullStacktraces;
extern INIT_GLOBAL u64 cfg_randSeed, cfg_hashSeed;
extern GLOBAL u64 internalRandState;

INIT_GLOBAL char* cbqn_self = "CBQN";
#include <stdarg.h>
__attribute__((format(printf, 1, 2)))
static NOINLINE NORETURN void arg_bad(const char* format, ...) {
  va_list a;
  va_start(a, format);
  fprintf(stderr, "%s: ", cbqn_self);
  vfprintf(stderr, format, a);
  fprintf(stderr, "\n");
  exit(1);
}
static NOINLINE NORETURN void arg_badVal(const char* key) {
  arg_bad("Invalid value for -X%s", key);
}
static NOINLINE bool cmpkey(char* mem, ux len, const char* exp, u8 mode) { // mode: 0:no value; 1: needs value; 2: optional
  bool same = len==strlen(exp) && !memcmp(mem, exp, len);
  if (mode!=2 && same && (mem[len]!=0) != (mode==1)) arg_bad("Flag -X%s must%s be given a value", exp, mode==0? " not" : "");
  return same;
}
static NOINLINE u64 arg_u64(const char* key, const char* val0) {
  char* val = (char*) val0;
  u64 r = readu64part(&val);
  if (!val || *val) arg_badVal(key);
  return r;
}
static NOINLINE u64 arg_getSeed(const char* key, const char* val) {
  u64 r;
  if (val[0] == 'u' && !val[1]) {
    u64 tmp = internalRandState;
    internalRandState = nsTime();
    r = internalRand() & 0xffffffffffff;
    log_printf("Using: -X%s=" N64u "\n", key, r);
    internalRandState = tmp;
  } else {
    r = arg_u64(key, val);
  }
  return r;
}
static void run_x_arg(char* key, ux keyn, const char* val) {
  if (cmpkey(key, keyn, "help", 0)) {
    printf(
      "Note that behavior or availability of theese options may change between CBQN versions, or differ across OSes or architectures.\n"
      "Note that arguments are parsed and evaluated in order; certain options may be non-functional if executed after any BQN code is executed.\n"
      "Available -X options:\n"
      "-Xlog-file=<path>    write internal logging to a file instead of stderr\n"
      "-Xrand-seed=<n>      make •rand be `•MakeRand n`\n"
      "-Xhash-seed=<n>      seed for hashing\n"
      "-Xinternal-seed=<n>  seed for internal randomization (RANDOMIZE_HEURISTICS, -Xffi-debug)\n"
      "  (note: seed value can be set to `u`, which will generate and print a random value)\n"
      "-Xfull-stacktraces   don't truncate display of deep stacktraces\n"
      "-Xffi-debug          make wrong FFI pointer buffer usage fail early, at the cost of performance and never-released memory (~64B/pointer) from preserved bookkeeping\n"
      "-Xffi-debug=<end|start|rand>\n"
      "                     whether to precisely catch past-the-end or before-the-start out-of-bounds accesses (default: end)\n"
      "-Xprofile [=freq]    profile execution of everything; similar to `)profile`\n"
      "\n"
      "-Xrepl-history-path=<path>\n"
      "                     use the specified file path for loading/saving REPL history\n"
      "-Xrepl-history=<read-only|none>\n"
      "                     disable writing, or also reading, of REPL history\n"
      "\n"
      "-Xjit-disable        disable JIT compilation; will be slower than if CBQN was compiled with JIT disabled!\n"
      "-Xjit-perf-map       write out /tmp/perf-<pid>.map of JIT mappings for linux perf; makes JIT code never get freed\n"
      "-Xmem-log            log on new requested heap memory blocks; same as `)mem log`\n"
      "-Xgc-log             log on GC; same as `)gc log`\n"
      "-Xgc-disable         disable mark&sweep GC; same as `)gc disable`\n"
      "-Xrepl-init          initialize CBQN & the REPL, but don't execute anything yet\n"
    );
    exit(0);
  } else if (cmpkey(key, keyn, "jit-perf-map", 0)) {
    bool cfg_enablePerfMap(void);
    if (!cfg_enablePerfMap()) arg_bad("-Xjit-perf-map not available in this CBQN build");
  } else if (cmpkey(key, keyn, "internal-seed", 1)) {
    internalRandState = arg_getSeed("internal-seed", val);
  } else if (cmpkey(key, keyn, "profile", 2)) {
    u64 rate = val? arg_u64("profile", val) : DEFAULT_PROFILE_SAMPLERATE;
    repl_init();
    if (!profiler_alloc() || !profiler_start(1, rate)) exit(1);
    cfg_profileGlobal = true;
  } else if (cmpkey(key, keyn, "rand-seed", 1)) {
    cfg_randSeed = arg_getSeed("rand-seed", val);
  } else if (cmpkey(key, keyn, "hash-seed", 1)) {
    cfg_hashSeed = arg_getSeed("hash-seed", val);
  } else if (cmpkey(key, keyn, "jit-disable", 0)) {
    cfg_enableJit = false;
  } else if (cmpkey(key, keyn, "mem-log", 0)) {
    cfg_memLog = true;
  } else if (cmpkey(key, keyn, "log-file", 1)) {
    log_file = fopen(val, "wb");
    if (log_file == NULL) arg_bad("Could not open -Xlog-file path for writing");
  } else if (cmpkey(key, keyn, "repl-init", 0)) {
    repl_init();
  } else if (cmpkey(key, keyn, "ffi-debug", 2)) {
    u8 align = 1;
    if (val) {
      if (!strcmp(val, "end")) align = 1;
      else if (!strcmp(val, "start")) align = 2;
      else if (!strcmp(val, "rand")) align = 3;
      else arg_badVal("ffi-debug");
    }
    if (!set_ffi_check(2, align)) arg_bad("-Xffi-debug is not supported in this CBQN");
#if USE_REPLXX
  } else if (cmpkey(key, keyn, "repl-history", 1)) {
    if (!strcmp(val, "read-only")) repl_historyMode = 1;
    else if (!strcmp(val, "none")) repl_historyMode = 0;
    else arg_badVal("repl-history");
  } else if (cmpkey(key, keyn, "repl-history-path", 1)) {
    repl_histfile = strdup(val);
#endif
#if ENABLE_GC
  } else if (cmpkey(key, keyn, "gc-disable", 0)) {
    gc_disable();
  } else if (cmpkey(key, keyn, "full-stacktraces", 0)) {
    cfg_fullStacktraces = true;
  } else if (cmpkey(key, keyn, "gc-log", 0)) {
    cfg_gcLog = true;
#else
  } else if (cmpkey(key, keyn, "gc-disable", 0) || cmpkey(key, keyn, "gc-log", 0)) {
    arg_bad("GC not supported in this CBQN build");
#endif
  } else {
    arg_bad("Unknown -X option: -X%s", key);
  }
}

int main(int argc, char* argv[]) {
  #if USE_REPLXX_IO
    cbqn_init();
    cbqn_init_replxx();
  #endif
  
  bool startREPL = argc==1;
  bool silentREPL = false;
  cbqn_self = argv[0];
  log_file = stderr;
  char* mainFileToExecute;
  if (!startREPL) {
    i32 i = 1;
    while (i!=argc) {
      char* carg = argv[i];
      if (carg[0]!='-') break;
      i++;
      if (!carg[1]) { mainFileToExecute = NULL; goto execFile; }
      if (carg[1]=='-') {
        if (!carg[2]) goto endOfArgs;
        if (!strcmp(carg, "--help")) {
          print_help:
          printf(
          "Usage: %s [options] [file.bqn [arguments]]\n"
          "Options:\n"
          "  -f file    execute the contents of the file with all further arguments as •args\n"
          "  -e code    execute the argument as BQN\n"
          "  -p code    execute the argument as BQN and print its result pretty-printed\n"
          "  -o code    execute the argument as BQN and print its raw result\n"
          "  -M num     set maximum heap size to num megabytes\n"
          "  -r         start the REPL after executing all arguments\n"
          "  -s         start a silent REPL\n"
          "  -X         various internal/debugging options; see -Xhelp\n"
          "  --help     show this help text\n"
          "  --version  display CBQN version information\n"
          , cbqn_self);
          exit(0);
        } else if (!strcmp(carg, "--version")) {
          #if HAS_VERSION
            printf("%s", cbqn_versionInfo);
          #else
            printf("CBQN, unknown version\n");
          #endif
          exit(0);
        } else {
          arg_bad("Unknown option: %s", carg);
        }
      } else {
        carg++;
        char c;
        while ((c = *carg++)) {
          switch(c) { default: arg_bad("Unknown option: -%c", c);
            #define REQARG(X) ({ if(*carg) arg_bad("Value of -%s must be in a separate argument", #X); if (i==argc) arg_bad("-%s requires an argument", #X); argv[i++]; })
            case 'f': repl_init(); mainFileToExecute = REQARG(f); goto execFile;
            case 'e': { repl_init(); char* val = REQARG(e);
              RUN_START;
              dec(gsc_exec_inplace(utf8Decode0(val), "(-e)", emptySVec()));
              RUN_END;
              break;
            }
            case 'p': { repl_init(); char* val = REQARG(p);
              RUN_START;
              B r = gsc_exec_inplace(utf8Decode0(val), "(-p)", emptySVec());
              if (FORMATTER) { r = bqn_fmt(r); printsB(r); }
              else { printI(r); }
              dec(r);
              RUN_END;
              printf("\n");
              break;
            }
            case 'o': { repl_init(); char* val = REQARG(o);
              RUN_START;
              B r = gsc_exec_inplace(utf8Decode0(val), "(-o)", emptySVec());
              if (isAtm(r) || RNK(r)!=1) thrM("(-o): Value to print must be a string");
              printsB(r); decG(r);
              printf("\n");
              RUN_END;
              break;
            }
            case 'M': {
              char* str = REQARG(M);
              u64 am = 0;
              while (*str) {
                char c = *str++;
                if (c<'0' | c>'9') arg_bad("-M: Argument not a number");
                if (am>1ULL<<48) arg_bad("-M: Too large");
                am = am*10 + c-48;
              }
              mm_heapMax = am*1024*1024;
              break;
            }
            case 'h': goto print_help;
            case 'r': { startREPL=true;                  break; }
            case 's': { startREPL=true; silentREPL=true; break; }
            case 'X': goto arg_x;
          }
        }
      }
      if (0) { arg_x:;
        char* val = strchr(carg, '=');
        ux keyn = val==NULL? strlen(carg) : val-carg;
        run_x_arg(carg, keyn, val? val+1 : NULL);
      }
    }
    endOfArgs:;
    
    if (i<argc) {
      mainFileToExecute = argv[i++];
      execFile:;
      repl_init();
      B src = mainFileToExecute? utf8Decode0(mainFileToExecute) : utf8DecodeA(stream_bytes(stdin));
      B args;
      if (i==argc) {
        args = emptySVec();
      } else {
        M_HARR(ap, argc-i)
        for (usz j = 0; j < argc-i; j++) HARR_ADD(ap, j, utf8Decode0(argv[i+j]));
        args = withFill(HARR_FV(ap), emptyCVec());
      }
      
      RUN_START; // top-level in main(), don't need to catch
      
      dec(mainFileToExecute? bqn_execFile(src, args) : gsc_exec_inplace(src, "(stdin)", args));
      
      #if HEAP_VERIFY
        cbqn_heapVerify();
      #endif
      RUN_END;
    }
  }
  if (startREPL) {
    repl_init();
    #if USE_REPLXX
    if (!silentREPL) {
      #if !USE_REPLXX_IO
        cbqn_init_replxx();
      #endif
      
      if (repl_historyMode != 0) {
        if (repl_histfile == NULL) {
          B f = get_config_path(false, ".cbqn_repl_history");
          repl_histfile = toCStr(f);
          dec(f);
          gc_add(tag(TOBJ(repl_histfile), OBJ_TAG));
        }
        replxx_history_load(global_replxx, repl_histfile);
      }
      
      replxx_set_ignore_case(global_replxx, true);
      replxx_set_highlighter_callback(global_replxx, highlighter_replxx, NULL);
      replxx_set_hint_callback(global_replxx, hint_replxx, NULL);
      replxx_set_completion_callback(global_replxx, complete_replxx, NULL);
      replxx_bind_key(global_replxx, REPLXX_KEY_ENTER, enter_replxx, NULL);
      replxx_set_modify_callback(global_replxx, modified_replxx, NULL);
      replxx_bind_key_internal(global_replxx, REPLXX_KEY_CONTROL('N'), "history_next");
      replxx_bind_key_internal(global_replxx, REPLXX_KEY_CONTROL('P'), "history_previous");
      replxx_set_max_history_size(global_replxx, 50000);
      
      while(true) {
        const char* ln = cbqn_replxx_input("   ");
        if (ln==NULL) {
          if (errno==0) printf("\n");
          break;
        }
        replxx_history_add(global_replxx, ln);
        cbqn_runLine((char*)ln, strlen(ln));
        if (repl_historyMode == 2) replxx_history_save(global_replxx, repl_histfile);
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
        if (read<=0 || ln[0]==0) { if(!silentREPL) printf("\n"); break; }
        if (ln[read-1]==10) ln[--read] = 0;
        if (ln[read-1]==13) ln[--read] = 0;
        cbqn_runLine(ln, read);
        free(ln);
      }
    }
  }
  #if HEAP_VERIFY
    cbqn_heapVerify();
  #endif
  bqn_exit(0);
}
#endif

void before_exit(void) {
  if (cfg_profileGlobal) {
    profiler_stop();
    profiler_displayResults(log_file);
    profiler_free();
  }
  #if USE_REPLXX
    if (global_replxx!=NULL && repl_histfile!=NULL) {
      if (repl_historyMode == 2) replxx_history_save(global_replxx, repl_histfile);
      replxx_end(global_replxx);
      global_replxx = NULL;
    }
  #endif
}