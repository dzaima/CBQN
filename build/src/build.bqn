t0g ← •MonoTime@
#!build/obj2/for_build
# to emulate on makefile transition: NO_LDL → rm_lf=-ldl; no_fPIC → rm_f=-fPIC; something something PIE

⟨Spawn, WaitForOne⟩ ← ⟨⟩ •Import "fork.bqn"
⟨Serialize, Deserialize⟩ ← •Import "serialize.bqn"

# modes:
#  0: single string option
#  1: toggle
#  2: flag list
opts ← ⟨
  ⟨0, "j", "4"⟩       # number of parallel jobs
  ⟨1, "verbose", 0⟩   # log more things
  ⟨1, "rebuild", 0⟩   # forcibly rebuild everything
  # TODO clean
  
  ⟨0, "CC", @⟩ # the C compiler
  ⟨0, "LD", @⟩       # linker of the final binary; default CC, or CXX if REPLXX=1
  ⟨1, "color", 1⟩    # whether to enable colored diagnostics
  ⟨0, "os", @⟩       # target OS (linux, bsd, macos); used for .so vs .dylib, and changing linker flags for bsd
  ⟨0, "arch", @⟩     # target architecture (x86-64, aarch64, generic); used for Singeli target deciding
  
  ⟨0, "OUTPUT", ""⟩  # output location; default depends
  ⟨2, "f", ⟨⟩⟩       # C flags for CBQN files
  ⟨2, "CCFLAGS", ⟨⟩⟩ # flags for all C compiler & linker invocations
  ⟨2, "lf", ⟨⟩⟩      # linker flags
  ⟨2, "LDFLAGS", ⟨⟩⟩ # more linker flags (identical to lf)
  ⟨2, "rm_f", ⟨⟩⟩    # forcibly remove C compiler flag(s)
  ⟨2, "rm_lf", ⟨⟩⟩   # forcibly remove linker flag(s)
  
  # also define the default OUTPUT value (default default being "BQN")
  ⟨1, "shared", 0⟩   # build a shared library; OUTPUT = "libcbqn.[so|dylib]"
  ⟨1, "wasi", 0⟩     # build with WASI;        OUTPUT = "BQN.wasm"
  ⟨1, "emcc", 0⟩     # build with emscripten;  OUTPUT = ".", is used as a directory
  
  ⟨1, "FFI", @⟩       # enable FFI through libffi; default ≡ ¬wasi∨emcc
  ⟨1, "pkgconfig", 1⟩ # attempt to use pkg-config to find libffi inclusion flags
  ⟨1, "singeli", 0⟩   # enable Singeli
  ⟨1, "REPLXX", 0⟩    # enable replxx
  # TODO options for forcing around the bytecode/singeli/replxx directories
  
  # CBQN toggles
  ⟨1, "rtverify", 0⟩
  ⟨1, "heapverify", 0⟩
  
  ⟨1, "c", 0⟩      # disable some default flags
  ⟨1, "O3", @⟩     # -O3; default ≡ ¬c
  ⟨1, "g", @⟩      # -g; default ≡ debug
  ⟨1, "native", 0⟩ # -march=native & use native arch as lower & upper bound for Singeli
  ⟨1, "debug",  0⟩ # -DDEBUG & -g
  
  ⟨0, "CXX", "c++"⟩ # C++ compiler (for replxx)
  ⟨2, "REPLXX_FLAGS", ⟨"-std=c++11", "-Os"⟩⟩ # default replxx C++ build flags
⟩

Log ← •Out
_verboseLog ← {𝔽_𝕣:⊢}
onExitList ← ⟨⟩
OnExit ← {𝕊: {𝕏@}¨ onExitList}
_assert_ ← { 𝔾𝕩?𝕩; Log 𝕨𝔽𝕩 ⋄ OnExit@ ⋄ •Exit 1}

SepArgs ← {' ' ((¬-˜⊢×·+`»⊸>)∘≠⊔⊢) 𝕩}
Lowercase ← {𝕩 - (-´"Aa")×(𝕩≥'A')∧𝕩≤'Z'}

getOpt ← {
  args ← ×∘≠¨⊸/ •args
  [ot, on0, od] ← ⍉>opts
  on ← Lowercase¨ on0
  ot1 ← 1=ot
  ot2 ← 2=ot
  
  s1 ← ⊑∘⊐⟜'='¨ args # start of =
  hs ← s1<≠¨args     # has value
  fl ← hs∧'+'=(0⌈s1-1)⊑¨args # is appended flag, i.e. "f+=a b c"
  s0 ← s1-fl         # end of key
  ks ← s0↑¨args      # keys
  ks Lowercase¨↩     # lowercase keys
  vs ← (1+s1)↓¨args  # values
  {𝕊: "Unknown option -- "∾⊑(¬ks∊on)/ks}_assert_(∧´) ks∊on
  
  ci ← on⊐ks # index in option specification
  {𝕊: ∾⟨"Error: Unexpected '+=' for '",ks⊑˜⊑/¬𝕩,"'"⟩}_assert_(∧´) (ci⊏ot2)∨¬fl
  
  Gr ← {(≠ot)↑ci⊔𝕩}
  gvs ← Gr vs # values for each option
  {𝕊: ∾⟨"Error: Multiple values given for '",on⊑˜⊑/¬𝕩,"'"⟩}_assert_(∧´) ot2∨1≥≠¨gvs
  
  {𝕊: ∾⟨"Error: Expected value to be provided for ",⊑𝕩/ks⟩}_assert_(¬∨´) (¬ci⊏ot1) ∧ ¬hs
  
  # f='a b' f+='c d' → "a" "b" "c d"
  gvs ↩ (Gr fl) (∾{𝕨?⋈𝕩;×≠𝕩?SepArgs 𝕩;⋈⟨⟩}¨)¨⌾(ot2⊸/) gvs
  # map toggles to their actual values, error on invalid
  gvs ↩ on {𝕨{𝕊: ∾⟨"Error: Invalid value for '",𝕨,"'"⟩}_assert_⊢ ∧´𝕩∊""‿"0"‿"1" ⋄ ≠◶⊢‿(⋈"0"≢⊑) 𝕩}¨⌾(ot1⊸/) gvs
  
  gvs ↩ ((¬ot2) ⋈∘⊢⍟⊣¨ od) {×≠𝕩?𝕩;𝕨}¨ gvs # map in defaults
  
  gvs ↩ (¬ot2) ⋈⁼∘⊢⍟⊣¨ gvs # disclose options with only one expected result
  
  # •Show [ks, fl, hs, ci, vs]
  # •Show {𝕩/˜×(≠1⊑⊢)˘𝕩} ⍉[on, gvs]
  {(⊑on⊐<Lowercase 𝕩)⊑gvs}
}

SH ← {
  c‿o‿e ← •SH⎊{𝕊: 𝕩 {𝕊: ∾⟨"Error: Failed to spawn ",⊑𝕨⟩}_assert_⊢ 0} 𝕩
  Log⍟(×≠) e
  𝕩 {𝕊: ∾⟨"Error: ",⊑𝕨," exited with error code ",•Repr 𝕩⟩}_assert_(0=⊢) c
  o
}

rootDir ← •file.Parent⍟2 •file.path
AtRoot ← rootDir⊸•file.At

po ← { # parsed options
  ⟨
    verbose, j, rebuildAll, output,
    os, arch, native,
    emcc, wasm, replxx, singeli,
    bytecodeDir, replxxDir, singeliDir,
    CBQNc, REPLXXc, Linker
  ⟩⇐
  
  custom ← GetOpt "c"
  DOpt ← {@⊸≡◶⟨⊢,𝕨⟩ GetOpt 𝕩}
  j ⇐ •BQN GetOpt "j"
  rebuildAll ⇐ GetOpt "rebuild"
  {𝕊: _verboseLog ↩ {Log𝕨𝔽𝕩⋄𝕩}}⍟⊢ verbose⇐GetOpt "verbose"
  
  Max1 ← {𝕩 {𝕊: a‿b←2↑𝕩/𝕨 ⋄ ∾⟨"Error: '",a,"' and '",b,"' cannot both be enabled"⟩}_assert_(1≥+´) GetOpt¨ 𝕩}
  Max1 "REPLXX"‿"shared"‿"wasi"‿"emcc"
  
  
  shared ← GetOpt "shared"
  native ⇐ GetOpt "native"
  emcc ⇐ GetOpt "emcc"
  wasi ← GetOpt "wasi"
  wasm ⇐ wasi∨emcc
  
  cc ← {emcc? "emcc"; "clang"} DOpt "CC"
  cxx ← GetOpt "CXX"
  ffi ← (¬wasm) DOpt "FFI"
  debug ← GetOpt "debug"
  singeli ⇐ GetOpt "singeli"
  replxx ⇐ GetOpt "REPLXX"
  
  uname ← ⊢◶""‿{𝕊: Lowercase 1⊑•SH⎊1‿""‿"" "uname"‿"-sm"} ∧´ (@≡GetOpt)¨ "os"‿"arch"
  InUname ← {𝕊: ∨´ 𝕩⍷uname}
  
  os ⇐ Lowercase {InUname"linux"? "linux"; InUname"darwin"? "macos"; InUname"bsd"? "bsd"; "linux"} DOpt "os"
  linux‿bsd‿macos ← os⊸≡¨ {𝕊: "Error: Unsupported OS; options:"∾1↓∾", "⊸∾¨𝕩}_assert_(⊑os<⊸∊⊢) "linux"‿"bsd"‿"macos"
  
  arch ⇐ {'-'¨⌾(('_'=𝕩)⊸/)𝕩} Lowercase {InUname"x86_64"? "x86-64"; ∨´InUname¨"aarch64"‿"arm64"? "aarch64"; "generic"} DOpt "arch"
  {𝕊: "Error: Unsupported arch; options:"∾1↓∾", "⊸∾¨𝕩}_assert_(⊑arch  <⊸∊⊢) "x86-64"‿"aarch64"‿"generic"
  
  {𝕊: "Error: Cannot use Singeli on x86-64 without 'native'"}_assert_¬ (arch≡"x86-64")∧singeli∧¬native
  {𝕊: "Error: Cannot use Singeli on generic arch"}_assert_¬ singeli ∧ arch≡"generic"
  
  output ⇐ GetOpt "OUTPUT"
  output ↩ •wdpath•file.At {𝕊:
    wasi? "BQN.wasm";
    emcc? ".";
    shared? "libcbqn." ∾ macos⊑"so"‿"dylib";
    "BQN"
  }⍟(""≡⊢) output
  
  exportSymbols ← ffi∨shared
  
  GetLibs ← { 𝕊:
    getLibs ↩ {
      ¬ffi? ⟨⟨⟩,⟨⟩⟩;
      GetOpt "pkgconfig"? 0=⊑ •SH⎊1 "pkg-config"‿"--exists"‿"libffi"?
        {SepArgs ¯1↓SH⟨"pkg-config",𝕩,"libffi"⟩}¨ ⟨"--cflags", "--libs"⟩;
      ⟨⟨⟩, ⟨"-lffi"⟩⟩
    }
  }
  
  SubmoduleDir ← "build"⊸•file.At⍟(@≢⊢) {𝕨≡0?@; •file.Exists r←AtRoot "build"•file.At 𝕩∾"Local"? r; 𝕩∾"Submodule"}
  LogDir ← {𝕩≡@? 𝕨∾": not used"; 𝕨∾" directory: "∾AtRoot 𝕩}_verboseLog
  bytecodeDir ⇐ 1       SubmoduleDir "bytecode" ⋄ "Bytecode" LogDir bytecodeDir
  replxxDir   ⇐ replxx  SubmoduleDir "replxx"   ⋄ "REPLXX" LogDir replxxDir
  singeliDir  ⇐ singeli SubmoduleDir "singeli"  ⋄ "Singeli" LogDir singeliDir
  {𝕊: "Output location: "∾𝕩}_verboseLog output
  
  CBQNc ⇐ { 𝕊:
    isClang ← ∨´"clang"⍷SH cc‿"--version" # TODO exclude from hash?
    args ← ⟨
      cc,
      "-std=gnu11",
      "-Wall", "-Wno-unused-function",
      "-fms-extensions", "-ffp-contract=off", "-fno-math-errno", "-fvisibility=hidden",
      "-DBYTECODE_DIR="∾•file.Name bytecodeDir,
      "-DSINGELI="∾•Repr singeli,
      "-DFFI="∾•Repr 2×ffi
    ⟩
    args∾↩ isClang⊑⟨
      ⟨"-Wno-parentheses"⟩
      ⟨"-Wno-microsoft-anon-tag", "-Wno-bitwise-instead-of-logical", "-Wno-unknown-warning-option"⟩
    ⟩
    args∾↩ 0⊑GetLibs@
    
    args∾↩ GetOpt "f"
    args∾↩ GetOpt "CCFLAGS"
    args∾↩ (singeli∧arch≡ "x86-64") / ⟨"-DSINGELI_X86_64"⟩
    args∾↩ (singeli∧arch≡"aarch64") / ⟨"-DSINGELI_NEON"⟩
    args∾↩ (               wasm) / ⟨"-DWASM"⟩
    args∾↩ (               wasi) / ⟨"-DWASI", "-DNO_MMAP", "-DCATCH_ERRORS=0", "-D_WASI_EMULATED_MMAN", "--target=wasm32-wasi"⟩
    args∾↩ (               emcc) / ⟨"-DEMCC", "-O3"⟩
    args∾↩ (             replxx) / ⟨"-DUSE_REPLXX", "-I"∾replxxDir∾"/include"⟩ # TODO maybe move to main.c only, and have it be in its own separate cache dir, so that adding replxx doesn't recompile everything?
    args∾↩ (     debug DOpt "g") / ⟨"-g"⟩
    args∾↩ ((¬custom) DOpt "O3") / ⟨"-O3"⟩
    args∾↩ (             native) / ⟨"-march=native"⟩
    args∾↩ (              debug) / ⟨"-DDEBUG"⟩
    args∾↩ (     GetOpt "color") / ⟨isClang⊑"-fdiagnostics-color=always"‿"-fcolor-diagnostics"⟩ # TODO exclude from hash?
    args∾↩ (  GetOpt "rtverify") / ⟨"-DRT_VERIFY", "-DEEQUAL_NEGZERO"⟩
    args∾↩ (GetOpt "heapverify") / ⟨"-DHEAP_VERIFY"⟩
    args∾↩ (      exportSymbols) / ⟨"-DCBQN_EXPORT"⟩
    args∾↩ (             shared) / ⟨"-DCBQN_SHARED", "-shared", "-fPIC"⟩
    args ↩ args (¬∘∊/⊣) GetOpt "rm_f"
    {"CBQN C compiler: "∾•Repr 𝕩} _verboseLog args
    cbqnc ↩ args
  }
  
  REPLXXc ⇐ { 𝕊:
    args ← ⟨cxx, "-DREPLXX_STATIC=1", "-I"∾replxxDir∾"/include"⟩
    args∾↩ GetOpt "REPLXX_FLAGS"
    {"REPLXX C++ compiler: "∾•Repr 𝕩} _verboseLog args
    replxxc ↩ args
  }
  
  Linker ⇐ { 𝕊:
    args ← ⟨{replxx? cxx; cc} DOpt "LD", "-lm"⟩
    args∾↩ 1⊑GetLibs@
    args∾↩ (¬bsd) / ⟨"-ldl"⟩
    args∾↩ GetOpt "lf"
    args∾↩ GetOpt "LDFLAGS"
    args∾↩ (                  wasi) / ⟨"-lwasi-emulated-mman", "--target=wasm32-wasi", "-Wl,-z,stack-size=8388608", "-Wl,--initial-memory=67108864"⟩
    args∾↩ (                  emcc) / ⟨"-s", "EXPORTED_FUNCTIONS=_main,_cbqn_runLine,_cbqn_evalSrc", "-s", "EXPORTED_RUNTIME_METHODS=ccall,cwrap", "-s", "ALLOW_MEMORY_GROWTH=1"⟩
    args∾↩ (         exportSymbols) / ⟨"-rdynamic"⟩
    args∾↩ ( linux ∧ arch≡"x86-64") / ⟨"-no-pie"⟩
    args ↩ args (¬∘∊/⊣) GetOpt "rm_lf"
    {"linker: "∾•Repr 𝕩} _verboseLog args
    linker ↩ args
  }
  
  {𝕊: CBQNc@ ⋄ REPLXXc⍟replxx @ ⋄ Linker@}⍟⊢ verbose
}

Hash ← {(32↑∾"0a"+↕¨10‿26)⊏˜{𝕨+2×𝕩}˝5‿⌊⥊32‿1•bit._cast∾(↕4)•Hash¨<𝕩}

MkDir ← {•file.Exists𝕩?@; •file.CreateDir𝕩}

allObjDir ← "build/obj2"
MkDir AtRoot allObjDir

isFileTheSame ← {
  ks‿vs ← ⟨⟩‿⟨⟩
  {
    i ← ⊑ks⊐<𝕨
    i<≠ks? 𝕩 ≡ i⊑vs;
    ks∾↩ <𝕨
    vs∾↩ <t←•file.Modified 𝕨
    𝕩 ≡ t
  }
}
updateSubmodule ← {
  done ← ⟨⟩
  {
    𝕩≡@? @;
    "Local"≡¯5↑𝕩? @;
    ⊑𝕩<⊸∊done? @;
    done∾↩ <𝕩
    SH ⟨"git", "-C", rootDir, "submodule", "update", "--init", AtRoot 𝕩⟩
  }
}

# gets/creates a directory of cacheable objects; key is the unique identifier of when it can be reused
GetCache ← { 𝕊 basename‿desc‿key:
  hash ⇐ Hash key
  folder ⇐ allObjDir•file.At basename∾"-"∾hash
  MkDir AtRoot folder
  
  File ⇐ folder⊸•file.At
  {𝕊: desc∾": "∾AtRoot𝕩} _verboseLog folder
  
  dataPath ← AtRoot File "data"
  prevKs‿prevVs ← •file.Exists◶⟨⟨⟩‿⟨⟩, {Deserialize ⟨8‿'c',8⟩•bit._cast •FBytes 𝕩}⟩ dataPath
  
  IsUpToDate ⇐ { 𝕊:
    i ← ⊑prevKs⊐<𝕩
    i<≠prevKs?
      {@:0; ∧´ AtRoot⊸IsFileTheSame´¨𝕩} i⊑prevVs;
    0
  }
  newKs‿newVs ← ⟨⟩‿⟨⟩
  Update ⇐ {key𝕊data: newKs∾↩ <key ⋄ newVs∾↩ <data} # data≡@ means failed to build
  onExitList∾↩ {{ 𝕊:
    jKs ← newKs∾prevKs
    jVs ← newVs∾prevVs
    dataPath •FBytes ⟨8,8‿'c'⟩•bit._cast Serialize (<∊jKs) /¨ jKs‿jVs
  }⍟{𝕊: 0≠≠newKs}}
}

ruleKs‿ruleVs ← ⟨⟩‿⟨⟩
AddRule ← { 𝕊 cache‿cacheKey‿dst‿GetCMD‿disp‿customDeps:
  ruleKs∾↩ <dst
  ruleVs∾↩ {
    cache ⇐ cache
    cacheKey ⇐ cacheKey
    disp ⇐ disp
    CMD ⇐ GetCMD
    ruleDeps ⇐ customDeps
  }
}

# dependency resolution & thread management
Run ← { 𝕊:
  ruleDeps ← {𝕩.ruleDeps}¨ ruleVs
  ruleSrcs0 ← ruleKs⊐∾ruleDeps
  ! ∧´ ruleSrcs0<≠ruleKs
  ruleSrcs ← ((≠∾˜≠¨/↕∘≠)ruleDeps) ⊔ ruleSrcs0
  req ← ⟨⟩
  ruleN ← 0¨ ruleKs # number of children (i.e. how many have this in their ruleP list)
  ruleP ← ⟨⟩¨ ruleKs # parent rules (i.e. which ones require this)
  rebuildAll ← po.rebuildAll
  
  Require ← {
    v ← 𝕩⊑ruleVs
    rebuild ← rebuildAll
    chi ← 𝕩⊑ruleSrcs
    chr ← Require¨ chi
    rebuild∨↩ ∨´ chr
    # rebuild∨↩ ¬•file.Exists AtRoot 𝕩⊑ruleKs # not really needed unless someone deletes a specific file without deleting the data file
    rebuild∨↩ ¬v.cache.IsUpToDate v.cacheKey
    {
      ruleN (+´chr)⊸+⌾(𝕩⊸⊑)↩
      ruleP ∾⟜𝕩¨⌾((chr/chi)⊸⊏)↩
      req∾↩ 𝕩
    }⍟rebuild 𝕩
    rebuild
  }
  anyRebuilt ← Require ⊑ruleKs⊐<𝕩
  
  left ← (req⊏ruleN=0)/req
  
  RequestJob ← { 𝕊:
    0=≠left? @;
    (left↓˜↩ ¯1) ⊢ ¯1⊑left
  }
  
  FinishJob ← { i𝕊tb:
    v ← i⊑ruleVs
    Log⍟(×≠) 1⊑tb
    ⊢◶⟨
      { 𝕊:
        v.cacheKey v.cache.Update @
      }
      { 𝕊:
        v.cacheKey v.cache.Update 3⊑tb
        ps ← i⊑ruleP
        ruleN -⟜1⌾(ps⊸⊏)↩
        left∾↩ (0=ps⊏ruleN)/ps
      } 
    ⟩ ⊑tb
    ⊑tb
  }
  
  threads ← Spawn∘(•file.At "runner.bqn")¨ ↕(≠req)⌊po.j
  work ← ⟨⟩
  free ← threads
  Ts ← {𝕩.t}¨
  
  storedOut ← ⟨⟩
  Log ↩ {storedOut∾↩ <𝕩∾@+10}
  # Log ↩ {𝕊:1}
  currLive ← ⟨⟩
  e ← @+27
  UpdateLive ← {
    •term.OutRaw (∾(0⌈¯1+≠currLive)⥊<(e∾(@+13)∾"[0K"∾e∾"[1F")) ∾ (•ToUTF8 ∾storedOut) ∾ 1↓∾((@+10)∾•ToUTF8)¨ 𝕩
    •term.Flush @
    currLive ↩ 𝕩
    storedOut ↩ ⟨⟩
  }
  FmtTime ← {{(""≡◶⊢‿"0" ¯1↓𝕩)∾'.'∾¯1↑𝕩} •Repr ⌊0.5+ 10×𝕩}
  onExitList∾↩ {{𝕊: UpdateLive ⟨⟩}}
  
  tmap ← ⟨⟩ # threads in the order they're displayed on-screen (dynamically calculated so that if there's only ever only one job in parallel, there are no pointless empty lines)
  doneCount ← 0
  stopping ← 0
  Fail ← {stopping↩1 ⋄ Log 𝕩}
  DoneLine ← {𝕊: ∾⟨•Repr doneCount, "/", •Repr ≠req⟩}
  { 𝕊:
    {𝕊:
      t ← ¯1⊑free ⋄ free↓˜↩ ¯1
      
      i ← RequestJob@ ⋄ v←i⊑ruleVs
      t.Request v.CMD@
      work∾↩ {t⇐t, i⇐i, v⇐v, t0⇐•MonoTime@}
      
    }•_while_{𝕊: ∧´0<≠¨ left‿free}⍟¬ stopping
    
    t1 ← •MonoTime@
    tmap ↩ ⍷tmap∾Ts work
    UpdateLive (<DoneLine@)∾{
      𝕩≡≠work? "";
      o ← 𝕩⊑work
      ∾⟨"[", FmtTime t1-o.t0, "] ", o.v.disp⟩
    }¨ (Ts work) ⊐tmap
    
    dm ← 0.1 WaitForOne Ts work
    lm ← ¬dm
    {𝕊: Fail "Error: Fork died" ⋄ dm‿lm∧↩<dm≠¯1}⍟⊢ ∨´dm=¯1
    done ← dm/work
    work ↩ lm/work
    {𝕩.i FinishJob 𝕩.t.Take@? doneCount+↩1; Fail "Error: During '"∾𝕩.v.disp∾"'"}¨ done
    free∾↩ Ts done
  }•_while_{𝕊: (0<≠work) ∨ (¬stopping)∧0<≠left}@
  
  Log ∾⟨DoneLine@, " in ", FmtTime t0g-˜•MonoTime@, "s", stopping/"; failed to build"⟩
  
  ¬stopping
}


MakeCCInv ← { 𝕊 GetArgs‿Init‿cache‿id‿src‿customDeps: # src should be CBQN-base-dir-relative, so that cache doesn't store 
  key ⇐ id
  dst ⇐ cache.File key∾".o"
  GetCMD ← { 𝕊:
    Init @
    dep ← AtRoot cache.File id∾".d"
    ⟨"sh", rootDir, (GetArgs@)∾⟨"-MT", "o", "-MMD", "-MF", dep, "-o", AtRoot dst, "-c", src⟩, dep⟩
  }
  AddRule ⟨cache, key, dst, GetCMD, •file.Name src, customDeps⟩
}

MakeSingeliInv ← { 𝕊 args‿Init‿cache‿id‿src‿customDeps: # src should be CBQN-base-dir-relative, so that cache doesn't store 
  key ⇐ id
  dst ⇐ cache.File key∾".c"
  GetCMD ← { 𝕊:
    Init @
    dep ← AtRoot cache.File id∾".d"
    ⟨"singeli", rootDir, AtRoot dst, AtRoot po.singeliDir, args, AtRoot src, dep⟩
  }
  AddRule ⟨cache, key, dst, GetCMD, •file.Name src, customDeps⟩
}

MakeLinkerInv ← { 𝕊 GetArgs‿cache‿name‿srcs:
  dst ⇐ cache.File name
  GetCMD ← { 𝕊:
    args ← GetArgs@
    ⟨"sh", rootDir, ⟨⊑args, "-o", dst⟩∾srcs∾1↓args, @⟩
  }
  AddRule ⟨cache, name, dst, GetCMD, "link", srcs⟩
}



# actual CBQN/Singeli/REPLXX definitions
cachedBin‿linkerCache ← {
  Shorten ← {r ← {𝕩↓˜¯1-⊑'.'⊐˜⌽𝕩}¨ •file.Name¨ 𝕩 ⋄ ! ∧´ ∊r ⋄ r}
  cbqnSrc ← ∾{⌽(⊑𝕩)⊸•file.At¨ 1↓𝕩}¨ ⌽⟨
    ⟨"src/builtins/", "arithd.c", "arithm.c", "cmp.c", "sfns.c", "squeeze.c", "select.c", "slash.c", "group.c", "sort.c", "search.c", "selfsearch.c", "fold.c", "scan.c", "md1.c", "md2.c", "fns.c", "sysfn.c", "internal.c", "inverse.c"⟩
    ⟨"src/core/", "tyarr.c", "harr.c", "fillarr.c", "stuff.c", "derv.c", "mm.c", "heap.c"⟩
    ⟨"src/", "load.c", "main.c", "rtwrap.c", "vm.c", "ns.c", "nfns.c", "ffi.c"⟩
    ⟨"src/jit/", "nvm.c"⟩
    ⟨"src/utils/", "utf.c", "hash.c", "file.c", "mut.c", "each.c", "bits.c"⟩
  ⟩
  singeliMap ← {po.arch≡"aarch64"? 𝕩/˜(1⊑¨𝕩)∊"cmp"‿"bits"‿"equal"; 𝕩} ⟨
    "src/core/stuff.c"‿"equal",        "src/utils/mut.c"‿"copy",       "src/utils/bits.c"‿"bits"
    "src/builtins/arithd.c"‿"dyarith", "src/builtins/cmp.c"‿"cmp",     "src/builtins/squeeze.c"‿"squeeze"
    "src/builtins/select.c"‿"select",  "src/builtins/fold.c"‿"fold",   "src/builtins/scan.c"‿"scan"
    "src/builtins/scan.c"‿"neq",       "src/builtins/slash.c"‿"slash", "src/builtins/slash.c"‿"constrep"
  ⟩
  objs ← ⟨⟩
  
  
  
  replxxCache ← {
    ¬po.replxx? @;
    replxxCache ← GetCache ⟨"replxx", "REPLXX object file location", po.REPLXXc@⟩
    
    replxxSrc ← (po.replxxDir•file.At"src")⊸•file.At¨⟨"ConvertUTF.cpp", "wcwidth.cpp", "conversion.cxx", "escape.cxx", "history.cxx", "prompt.cxx", "replxx.cxx", "replxx_impl.cxx", "terminal.cxx", "util.cxx", "windows.cxx"⟩
    objs∾↩ {𝕩.dst}¨ (Shorten replxxSrc) {MakeCCInv ⟨po.REPLXXc, ⊢, replxxCache, 𝕨, 𝕩, ⟨⟩⟩}¨ replxxSrc
    
    replxxCache
  }
  
  singeliObjs ← @
  singeliCache ← {
    ¬po.singeli? @;
    singeliCache ← GetCache ⟨"singeli", "Singeli generated code location", ⟨po.native, po.arch⟩⟩
    
    # genArithTables
    ga ← "src/singeli/src/genArithTables.bqn"
    gaDefs ← singeliCache.File "arDefs.singeli"
    gaTables ← singeliCache.File "arTables.c"
    AddRule ⟨
      singeliCache, "genArithTables",
      gaDefs, # am cheating and only using arDefs.singeli as destination; ¯\_(ツ)_/¯
      {𝕊: ⟨"runbqn", rootdir, AtRoot ga, AtRoot¨ gaDefs‿gaTables, ⟨ga⟩⟩},
      •file.Name ga, ⟨⟩
    ⟩
    
    singeliArgs ← ⟨"-l", "gen="∾singeliCache.folder⟩∾{
      po.native? ⟨⟩;
      "-a" ⋈ {"x86-64":"X86_64"; "aarch64":"AARCH64"} po.arch
    }
    singeliObjs ↩ {𝕩.dst}¨ {MakeSingeliInv ⟨singeliArgs, {𝕊:UpdateSubmodule po.singeliDir}, singeliCache, 𝕩, "src/singeli/src/"•file.At 𝕩∾".singeli", (𝕩≡"dyarith")/⟨gaDefs⟩⟩}¨ 1⊑¨singeliMap
    
    singeliCache
  }
  
  cbqnCache ← {
    cbqnCache ← GetCache ⟨"cbqn", "CBQN object file location", ⟨po.CBQNc@, {po.singeli? singeliCache.hash; @}⟩⟩
    ruleDeps ← {
      ¬po.singeli? ⟨⟩¨ cbqnSrc;
      ((≠cbqnSrc) ∾˜ cbqnSrc⊐⊑¨singeliMap) ⊔ singeliObjs
    }
    
    singeliArgs ← {po.singeli? ⟨"-DSINGELI_DIR="∾•file.Name singeliCache.folder⟩; ⟨⟩}
    objs∾↩ {𝕩.dst}¨ {a‿b‿c: MakeCCInv ⟨po.CBQNc∾singeliArgs˙, {𝕊:UpdateSubmodule po.bytecodeDir ⋄ UpdateSubmodule po.replxxDir}, cbqnCache, a, b, c⟩}¨ <˘⍉[Shorten cbqnSrc, cbqnSrc, ruleDeps] # updates replxx because needs replxx.h
    
    cbqnCache
  }
  
  linkerCache ← GetCache ⟨"linker", "linker cached result location", ⟨
    po.Linker@,
    cbqnCache.hash,
    {po.replxx? replxxCache.hash; @}
    {po.singeli? singeliCache.hash; @}
  ⟩⟩
  res ← MakeLinkerInv ⟨po.Linker, linkerCache, {po.emcc? "BQN.js"; "res"}, objs⟩
  
  res.dst ⋈ linkerCache
}

outPath ← •wdpath •file.At po.output
success ← Run cachedBin
{ 𝕊:
  po.emcc?
    SH ⟨"cp", AtRoot cachedBin,                   outPath •file.At •file.Name cachedBin⟩
    SH ⟨"cp", AtRoot linkerCache.File "BQN.wasm", outPath •file.At "BQN.wasm"⟩
  ;
    SH ⟨"cp", AtRoot cachedBin, outPath⟩
}⍟⊢ success

OnExit@
•Exit ¬success