t0g ← •MonoTime@
#!build/obj2/for_build
# todo:
#   clean
#   forcing bytecode/singeli/replxx directories to whatever's applicable

⟨Spawn, WaitForOne⟩ ← ⟨⟩ •Import "fork.bqn"
⟨Serialize, Deserialize⟩ ← •Import "serialize.bqn"

# modes:
#  0: single string option
#  1: toggle
#  2: flag list
opts ← ⟨
  # makefile compatibility options:
  ⟨1, "from-makefile", 0, @⟩
  ⟨2, "LD_LIBS", @, @⟩
  ⟨1, "NO_LDL",  0, @⟩
  ⟨1, "no_fPIC", 0, @⟩
  
  ⟨0, "j",       @, "Number of parallel jobs"⟩
  ⟨1, "verbose", 0, "Log more things"⟩
  ⟨1, "rebuild", 0, "Forcibly rebuild everything"⟩
  ⟨1, "notui",   0, "disable live-updating status display"∾@+10⟩
  ⟨1, "rebuild-singeli", 0, @⟩
  
  ⟨0, "CC",    @, "The used C compiler"⟩
  ⟨0, "LD",    @, "Linker of the final binary; defaults to CC, or CXX if REPLXX=1"⟩
  ⟨1, "color", @, "Whether to enable colored diagnostics; default based on notui"⟩
  ⟨0, "os",    @, "Target OS (linux, bsd, macos)"⟩ # used for .so vs .dylib, and changing linker flags for bsd
  ⟨0, "arch",  @, "Target architecture (x86-64, aarch64, generic)"⟩ # used for selecting Singeli target
  ⟨1, "pie",   @, "Position-independent executable; default based on OS & arch"∾@+10⟩
  
  ⟨0, "OUTPUT",  "", "Output location; defaults to ./BQN for regular builds"⟩
  ⟨2, "f",       ⟨⟩, "C flags for CBQN files"⟩
  ⟨2, "CCFLAGS", ⟨⟩, "flags for all C compiler & linker invocations"⟩
  ⟨2, "lf",      ⟨⟩, "linker flags"⟩
  ⟨2, "sf",      ⟨⟩, "Singeli flags"⟩
  ⟨2, "LDFLAGS", ⟨⟩, @⟩
  ⟨2, "rm_f",    ⟨⟩, "forcibly remove C compiler flag(s)"⟩
  ⟨2, "rm_lf",   ⟨⟩, "forcibly remove linker flag(s)"⟩
  ⟨0, "v",       @,  "Version to report by --version; 'v=' to use git commit"∾@+10⟩
  
  ⟨1, "shared", 0, "Build a shared library; default output depends on 'os' setting"⟩
  ⟨1, "wasi",   0, "Build with WASI; default output name is BQN.wasm"⟩
  ⟨1, "emcc",   0, "Build with emscripten; outputs two files - BQN.wasm and BQN.js"⟩
  ⟨1, "clangd", 0, "Don't build, instead generate a compile_commands.json for clangd"∾@+10⟩
  
  ⟨1, "FFI",       @, "Enable FFI through libffi; On by default, except for WASM builds"⟩
  ⟨1, "pkgconfig", 1, "Attempt to use pkg-config to find libffi flags"⟩
  ⟨1, "singeli",   0, "Enable compilation with Singeli"⟩
  ⟨1, "replxx",    0, "Enable REPLXX"∾@+10⟩
  
  ⟨1, "c",      0, "Disable some default flags"⟩
  ⟨1, "O3",     @, "Use '-O3'; On by default unless 'c=1'"⟩
  ⟨1, "native", @, "Use '-march=native'"∾@+10⟩
  
  ⟨1, "debug",      0, "Debug: '-DDEBUG -g'; enables various sanity checks"⟩
  ⟨1, "g",          @, "Debug: '-g'; enable debug symbols"⟩
  ⟨1, "rtverify",   0, "Debug: compare builtin results with the self-hosted runtime"⟩
  ⟨1, "heapverify", 0, "Debug: check reference count correctness"∾@+10⟩
  
  ⟨0, "CXX", "c++", "C++ compiler (for replxx)"⟩
  ⟨2, "REPLXX_FLAGS", ⟨"-std=c++11", "-Os"⟩, "default replxx C++ build flags"⟩
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
  
  treq ← (args≢¨<"v=") ∧ '='=¯1⊑¨args
  compat ← ⊑(<"from-makefile") ∊ args
  {
    compat? args (¬treq)⊸/ ↩; # makefile will leave things like "shared=" if the envvar was empty; build/build by default sees this as "shared=1", but we want to ignore it
    {𝕊: "Error: Trailing '=' for an option"} _assert_¬ ∨´treq
  }
  
  [ot, on0, od, oh] ← ⍉>opts
  on ← Lowercase¨ on0
  
  {𝕊:
    •Out "Usage: path/to/cbqn/build/build [options]
  Options are specified as arguments of 'key=value'. Keys are case-insensitive.
  Toggles without a value implicitly enable it; e.g. 'singeli' is 'singeli=1'.
  For flag lists, 'k=v' will split on spaces, i.e. 'f=-a -b' adds two flags,
  whereas 'f+=-a -b' adds one flag that contains a space.
Options:
"∾∾oh {"  "∾𝕩∾"  "∾𝕨∾@+10}¨⟜((⌈´≠¨)⊸(↑¨))○((@≢¨oh)⊸/) on0
    •Exit 0
  }⍟⊢ ∨´ "help"‿"h"‿"?"∊'-'⊸≠⊸/¨ •args
  
  ot1 ← 1=ot
  ot2 ← 2=ot
  s1 ← ⊑∘⊐⟜'='¨ args # start of =
  hs ← s1<≠¨args     # has value
  fl ← hs∧'+'=(0⌈s1-1)⊑¨args # is appended flag, i.e. "f+=a b c"
  s0 ← s1-fl         # end of key
  ks ← s0↑¨args      # keys
  ks Lowercase¨↩     # lowercase keys
  vs ← (1+s1)↓¨args  # values
  {𝕊: ∾⟨"Unknown option -- '",⊑(¬ks∊on)/ks, "'; see --help for options"⟩}_assert_(∧´) ks∊on
  
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
TrySH ← {1⊑•SH⎊1‿𝕨‿"" 𝕩}

rootDir ← •file.Parent⍟2 •file.path
AtRoot ← rootDir⊸•file.At

po ← { # parsed options
  ⟨
    verbose, J, rebuildAll, rebuildSingeli, tui, output, clangd, versionName,
    os, arch, native,
    emcc, wasm, replxx, singeli,
    ffi, o3,
    bytecodeDir, replxxDir, singeliDir,
    CBQNc, ccColor, singeliFlags, REPLXXc, Linker
  ⟩⇐
  
  compat ← GetOpt "from-makefile"
  custom ← GetOpt "c"
  clangd ← GetOpt "clangd"
  DOpt ← {@⊸≡◶⟨⊢,𝕨⟩ GetOpt 𝕩}
  J ⇐ {𝕊: v←GetOpt "j" ⋄ v≢@? •ParseFloat v; •ParseFloat⎊4 "4" TrySH ⟨"nproc"⟩}
  rebuildAll ⇐ GetOpt "rebuild"
  rebuildSingeli ⇐ GetOpt "rebuild-singeli"
  tui ⇐ ¬GetOpt "notui"
  {𝕊: _verboseLog ↩ {Log𝕨𝔽𝕩⋄𝕩}}⍟⊢ verbose⇐GetOpt "verbose"
  
  Max1 ← {𝕩 {𝕊: a‿b←2↑𝕩/𝕨 ⋄ ∾⟨"Error: '",a,"' and '",b,"' cannot both be enabled"⟩}_assert_(1≥+´) GetOpt¨ 𝕩}
  Max1 "REPLXX"‿"shared"‿"wasi"‿"emcc"
  
  
  shared ← GetOpt "shared"
  native ⇐ 0 DOpt "native"
  emcc ⇐ GetOpt "emcc"
  wasi ← GetOpt "wasi"
  versionName ← {compat ∧ shared? @; "0"⊸≡◶⊢‿@ GetOpt "v"}
  {𝕊: "Error: Specifying version for shared build is useless"}_assert_¬ shared ∧ versionName≢@
  wasm ⇐ wasi∨emcc
  
  cc ← {emcc? "emcc"; "clang"} DOpt "CC"
  cxx ← GetOpt "CXX"
  debug ← GetOpt "debug"
  o3 ⇐ (¬custom) DOpt "O3"
  
  uname ← ⊢◶""‿{𝕊: Lowercase "" TrySH "uname"‿"-sm"} ∧´ (@≡GetOpt)¨ "os"‿"arch"
  InUname ← {𝕊: ∨´ 𝕩⍷uname}
  
  os ⇐ Lowercase {InUname"linux"? "linux"; InUname"darwin"? "macos"; InUname"bsd"? "bsd"; "linux"} DOpt "os"
  linux‿bsd‿macos‿windows ← os⊸≡¨ {𝕊: "Error: Unsupported OS; options:"∾1↓∾", "⊸∾¨𝕩}_assert_(⊑os<⊸∊⊢) "linux"‿"bsd"‿"macos"‿"windows"
  
  arch ⇐ {'-'¨⌾(('_'=𝕩)⊸/)𝕩} Lowercase {InUname"x86_64"? "x86-64"; ∨´InUname¨"aarch64"‿"arm64"? "aarch64"; "generic"} DOpt "arch"
  {𝕊: "Error: Unsupported arch; options:"∾1↓∾", "⊸∾¨𝕩}_assert_(⊑arch  <⊸∊⊢) "x86-64"‿"aarch64"‿"generic"
  
  ffi ⇐ ((¬windows)∧¬wasm) DOpt "FFI"
  singeli ⇐ GetOpt "singeli"
  replxx ⇐ GetOpt "REPLXX"
  
  {𝕊: "Error: Cannot use Singeli on x86-64 without explicit 'native'"}_assert_¬ (arch≡"x86-64") ∧ singeli ∧ @≡GetOpt"native"
  {𝕊: "Error: Cannot use Singeli on generic arch"}_assert_¬ singeli ∧ arch≡"generic"
  
  pie ⇐ {(¬shared)∧linux∧arch≡"x86-64"? 0; 1} DOpt "pie"
  
  output ⇐ GetOpt "OUTPUT"
  output ↩ •wdpath•file.At {𝕊:
    wasi? "BQN.wasm";
    emcc? ".";
    shared? {macos? "libcbqn.dylib"; windows? "cbqn.dll"; "libcbqn.so"};
    windows? "BQN.exe";
    "BQN"
  }⍟(""≡⊢) output
  
  exportSymbols ← ffi∨shared
  
  defLibs ← @≡GetOpt "LD_LIBS"
  GetLibs ← { 𝕊:
    getLibs ↩ ⟨⟩⌾(1⊸⊑)⍟(¬defLibs) {
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
  
  ccClang ← {clangd? 1; ∨´"clang"⍷SH cc‿"--version"}
  ccColor ⇐ ((¬clangd) ∧ tui DOpt "color") / ⟨ccClang⊑"-fdiagnostics-color=always"‿"-fcolor-diagnostics"⟩
  
  cbqnc ← {
    args ← ⟨
      cc,
      "-std=gnu11",
      "-Wall", "-Wno-unused-function",
      "-fms-extensions", "-ffp-contract=off", "-fno-math-errno", "-fvisibility=hidden", "-fno-strict-aliasing",
      "-DBYTECODE_DIR="∾•file.Name bytecodeDir,
      "-DSINGELI="∾•Repr singeli,
      "-DFFI="∾•Repr 2×ffi
    ⟩
    args∾↩ ccClang⊑⟨
      ⟨"-Wno-parentheses"⟩
      ⟨"-Wno-microsoft-anon-tag", "-Wno-bitwise-instead-of-logical", "-Wno-unknown-warning-option"⟩
    ⟩
    args∾↩ 0⊑GetLibs@
    
    
    args∾↩ GetOpt "f"
    args∾↩ GetOpt "CCFLAGS"
    args∾↩ (singeli∧arch≡ "x86-64") / ⟨"-DSINGELI_X86_64"⟩
    args∾↩ (singeli∧arch≡"aarch64") / ⟨"-DSINGELI_NEON"⟩
    args∾↩ (              wasm) / ⟨"-DWASM"⟩
    args∾↩ (              wasi) / ⟨"-DWASI", "-DNO_MMAP", "-DCATCH_ERRORS=0", "-D_WASI_EMULATED_MMAN", "--target=wasm32-wasi"⟩
    args∾↩ (              emcc) / ⟨"-DEMCC", "-O3"⟩
    args∾↩ (            replxx) / ⟨"-DUSE_REPLXX", "-DREPLXX_STATIC=1", "-I"∾replxxDir∾"/include"⟩ # TODO maybe move to main.c only, and have it be in its own separate cache dir, so that adding replxx doesn't recompile everything?
    args∾↩ (    debug DOpt "g") / ⟨"-g"⟩
    args∾↩ (                o3) / ⟨"-O3"⟩
    args∾↩ (            native) / ⟨"-march=native"⟩
    args∾↩ (             debug) / ⟨"-DDEBUG"⟩
    args∾↩ ( GetOpt "rtverify") / ⟨"-DRT_VERIFY", "-DEEQUAL_NEGZERO"⟩
    args∾↩ (GetOpt"heapverify") / ⟨"-DHEAP_VERIFY"⟩
    args∾↩ (     exportSymbols) / ⟨"-DCBQN_EXPORT"⟩
    args∾↩ (              ¬pie) / ⟨"-fno-pie"⟩
    args∾↩ (     pie ∧ ¬shared) / ⟨"-fPIE"⟩
    args∾↩ (            shared) / ⟨"-DCBQN_SHARED"⟩ ∾ (¬GetOpt"no_fPIC")/⟨"-fPIC"⟩
    args∾↩ (           windows) / ⟨"-DNO_MMAP"⟩
    args∾↩ (  replxx ∧ windows) / ⟨"-DUSE_REPLXX_IO"⟩
    args∾↩ (     @≢versionName) / ⟨"-DHAS_VERSION"⟩
    args ↩ args (¬∘∊/⊣) GetOpt "rm_f"
    {"CBQN C compiler: "∾•Repr 𝕩} _verboseLog args
    args
  }
  
  REPLXXc ⇐ { 𝕊:
    args ← ⟨cxx, "-DREPLXX_STATIC=1", "-I"∾replxxDir∾"/include"⟩
    args∾↩ GetOpt "REPLXX_FLAGS"
    args∾↩ (         ¬pie) / ⟨"-fno-pie"⟩
    args∾↩ (pie ∧ ¬shared) / ⟨"-fPIE"⟩
    args∾↩ (       shared) / (¬GetOpt"no_fPIC")/⟨"-fPIC"⟩
    {"REPLXX C++ compiler: "∾•Repr 𝕩} _verboseLog args
    replxxc ↩ args
  }
  
  singeliFlags ← GetOpt "sf"
  
  Linker ⇐ { 𝕊:
    rdynamic ← exportSymbols ∧ ¬windows
    args ← ⟨{replxx? cxx; cc} DOpt "LD"⟩
    args∾↩ defLibs / ⟨"-lm"⟩
    args∾↩ ⟨⟩ DOpt "LD_LIBS"
    args∾↩ ((¬GetOpt "NO_LDL") ∧ defLibs ∧ ffi ∧ ¬bsd) / ⟨"-ldl"⟩
    args∾↩ 1⊑GetLibs@
    args∾↩ GetOpt "lf"
    args∾↩ GetOpt "CCFLAGS"
    args∾↩ GetOpt "LDFLAGS"
    args∾↩ (         wasi) / ⟨"-lwasi-emulated-mman", "--target=wasm32-wasi", "-Wl,-z,stack-size=8388608", "-Wl,--initial-memory=67108864"⟩
    args∾↩ (         emcc) / ⟨"-s", "EXPORTED_FUNCTIONS=_main,_cbqn_runLine,_cbqn_evalSrc", "-s", "EXPORTED_RUNTIME_METHODS=ccall,cwrap", "-s", "ALLOW_MEMORY_GROWTH=1"⟩
    args∾↩ (     rdynamic) / ⟨"-rdynamic"⟩
    args∾↩ (         ¬pie) / ⟨"-no-pie"⟩
    args∾↩ (pie ∧ ¬shared) / ⟨"-fPIE", "-pie"⟩
    args∾↩ (       shared) / ⟨"-shared"⟩
    args∾↩ (      windows) / ⟨"-lpthread"⟩
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
    vs∾↩ <t←•file.Modified⎊"unknown" 𝕨
    𝕩 ≡ t
  }
}
GitCmd ← {
  ¯1↓SH ⟨"git", "-C", rootDir⟩∾𝕩
}
updateSubmodule ← {
  done ← ⟨⟩
  {
    𝕩≡@? @;
    "Local"≡¯5↑𝕩? @;
    ⊑𝕩<⊸∊done? @;
    done∾↩ <𝕩
    GitCmd ⟨"submodule", "update", "--init", AtRoot 𝕩⟩
  }
}

# gets/creates a directory of cacheable objects; key is the unique identifier of when it can be reused
GetCache ← { 𝕊 basename‿desc‿key:
  folderHash ⇐ Hash key
  folder ⇐ allObjDir•file.At basename∾"-"∾folderHash
  MkDir AtRoot folder
  
  File ⇐ folder⊸•file.At
  {𝕊: desc∾": "∾AtRoot𝕩} _verboseLog folder
  
  dataPath ← AtRoot File "data"
  dataVersion ← 0
  prevKs‿prevVs ← {a‿b: a≡dataVersion? b; ⟨⟩‿⟨⟩} •file.Exists◶⟨@, {Deserialize ⟨8‿'c',8⟩•bit._cast •FBytes 𝕩}⟩ dataPath
  
  Find ⇐ {
    i ← ⊑prevKs⊐<𝕩.cacheKey
    i<≠prevKs? i⊑prevVs;
    @
  }
  IsUpToDate ⇐ {
    𝕊:
      𝕩.found0≢@?
      fileInfo‿depInfo ← 𝕩.found0
      fileInfo≢@?
      ∧´ AtRoot⊸IsFileTheSame´¨ fileInfo?
      depInfo≡𝕩.DepHash@
    ;
      0
  }
  newKs‿newVs ← ⟨⟩‿⟨⟩
  finalNewData ← @
  FinalData ⇐ { 𝕊:
    finalNewData≢@?finalNewData;
    jKs ← prevKs∾˜ {𝕩.cacheKey}¨ newKs
    jVs ← prevVs∾˜ newKs {𝕨.SetVal 𝕩}¨ newVs
    finalNewData ↩ Serialize dataVersion⋈ (<∊jKs) /¨ jKs‿jVs
  }
  Update ⇐ {key𝕊data: # data≡@ means failed to build
    !finalNewData≡@
    newKs∾↩ <key
    newVs∾↩ <data
  }
  onExitList∾↩ {{𝕊: dataPath •FBytes ⟨8,8‿'c'⟩•bit._cast FinalData@}⍟{𝕊: 0≠≠newKs}}
}

ruleKs‿ruleVs ← ⟨⟩‿⟨⟩

# dependency resolution & thread management
Run ← { 𝕊:
  ruleDeps ← {{𝕩.dst}¨ 𝕩.ruleDeps}¨ ruleVs
  ruleSrcs0 ← ruleKs⊐∾ruleDeps
  ! ∧´ ruleSrcs0<≠ruleKs
  ruleSrcs ← ((≠∾˜≠¨/↕∘≠)ruleDeps) ⊔ ruleSrcs0
  req ← ⟨⟩
  ruleN ← 0¨ ruleKs # number of children (i.e. how many have this in their ruleP list)
  ruleP ← ⟨⟩¨ ruleKs # parent rules (i.e. which ones require this)
  
  Require ← {
    v ← 𝕩⊑ruleVs
    rebuild ← po.rebuildAll
    {𝕊: rebuild∨↩ ".singeli"≡¯8↑v.disp}⍟⊢ po.rebuildSingeli
    chi ← 𝕩⊑ruleSrcs
    chr ← Require¨ chi
    rebuild∨↩ ∨´ chr
    # rebuild∨↩ ¬•file.Exists AtRoot 𝕩⊑ruleKs # not really needed unless someone deletes a specific file without deleting the data file
    rebuild∨↩ ¬v.cache.IsUpToDate v
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
        v v.cache.Update @
      }
      { 𝕊:
        v v.cache.Update 3⊑tb
        ps ← i⊑ruleP
        ruleN -⟜1⌾(ps⊸⊏)↩
        left∾↩ (0=ps⊏ruleN)/ps
      } 
    ⟩ ⊑tb
    ⊑tb
  }
  
  threads ← Spawn∘(•file.At "runner.bqn")¨ ↕{0:0; 𝕩⌊po.J@} ≠req
  work ← ⟨⟩
  free ← threads
  Ts ← {𝕩.t}¨
  
  updateLive ← { 𝕊:
    storedOut ← ⟨⟩
    Log ↩ {storedOut∾↩ <𝕩∾@+10}
    currLive ← ⟨⟩
    e ← @+27
    {
      •term.OutRaw (¯4↓∾(≠currLive)⥊<(e∾(@+13)∾"[0K"∾e∾"[1F")) ∾ (•ToUTF8 ∾storedOut) ∾ 1↓∾((@+10)∾•ToUTF8)¨ 𝕩
      •term.Flush @
      currLive ↩ 𝕩
      storedOut ↩ ⟨⟩
    }
  }⍟po.tui 0
  FmtTime ← {{(""≡◶⊢‿"0" ¯1↓𝕩)∾'.'∾¯1↑𝕩} •Repr ⌊0.5+ 10×𝕩}
  onExitList∾↩ {{𝕊: UpdateLive ⟨⟩}}
  
  tmap ← ⟨⟩ # threads in the order they're displayed on-screen (dynamically calculated so that if there's only ever only one job in parallel, there are no pointless empty lines)
  doneCount ← 0
  stopping ← 0
  Fail ← {stopping↩1 ⋄ Log 𝕩}
  nextRedraw ← ¯∞
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
    FileLine ← {∾⟨"[", FmtTime t1-𝕩.t0, "] ", 𝕩.v.disp⟩}
    { 𝕊:
      nextRedraw ↩ t1+0.1
      UpdateLive (<DoneLine@)∾{
        𝕩≡≠work? "";
        FileLine 𝕩⊑work
      }¨ (Ts work) ⊐tmap
    }⍟⊢ t1>nextRedraw-0.02
    
    dm ← (0.1⌊nextRedraw-t1) WaitForOne Ts work
    lm ← ¬dm
    {𝕊: Fail "Error: Fork died" ⋄ dm‿lm∧↩<dm≠¯1}⍟⊢ ∨´dm=¯1
    done ← dm/work
    work ↩ lm/work
    {
      𝕩.i FinishJob 𝕩.t.Take@?
        {𝕊: Log FileLine 𝕩 ⋄ •term.Flush@}⍟(¬po.tui) 𝕩
        doneCount+↩1;
      Fail "Error: During '"∾𝕩.v.disp∾"'"
    }¨ done
    free∾↩ Ts done
  }•_while_{𝕊: (0<≠work) ∨ (¬stopping)∧0<≠left}@
  
  Log ∾⟨DoneLine@, " in ", FmtTime t0g-˜•MonoTime@, "s", stopping/"; failed to build"⟩
  
  ¬stopping
}


AddRule ← { 𝕊 cache‿cacheKey‿dst‿GetCMD‿disp‿deps:
  ruleKs∾↩ <dst
  setFound ← @
  res ← {
    dst ⇐ dst
    cache ⇐ cache
    cacheKey ⇐ cacheKey
    found0 ⇐ 0
    SetFound ↩ {found0↩𝕩}
    DepHash ⇐ {𝕊: Hash {𝕩.found1≡@? 𝕩.found0; 𝕩.found1}¨ ruleDeps} # todo instead of conditionally choosing found1 or found2, decide statically; and properly cache
    found1 ⇐ @
    SetVal ⇐ {found1 ↩ 𝕩⋈DepHash@}
    disp ⇐ disp
    CMD ⇐ GetCMD
    ruleDeps ⇐ deps
  }
  SetFound cache.Find res
  ruleVs∾↩ res
  res
}

clangd ← {¬po.clangd? {_add⇐{𝕨𝔽_𝕣𝕩:@} ⋄ Files⇐⊣}; ⟨AtRoot⟩ •Import "clangd.bqn"}

MakeCCInv ← { 𝕊 GetArgs‿Init‿cache‿id‿src‿customDeps: # src should be CBQN-base-dir-relative, so that cache doesn't store full paths
  GetArgs clangd._add src
  dst ← cache.File id∾".o"
  GetCMD ← { 𝕊:
    Init @
    dep ← AtRoot cache.File id∾".d"
    ⟨"sh", rootDir, (GetArgs@)∾⟨"-MT", "o", "-MMD", "-MF", dep, "-o", AtRoot dst, "-c", src⟩, dep⟩
  }
  AddRule ⟨cache, id, dst, GetCMD, •file.Name src, customDeps⟩
}

MakeSingeliInv ← { 𝕊 args‿Init‿cache‿id‿src‿customDeps: # src should be CBQN-base-dir-relative, so that cache doesn't store full paths
  dst ← cache.File id∾".c"
  GetCMD ← { 𝕊:
    Init @
    dep ← AtRoot cache.File id∾".d"
    ⟨"singeli", rootDir, AtRoot dst, AtRoot po.singeliDir, args, AtRoot src, dep⟩
  }
  AddRule ⟨cache, id, dst, GetCMD, •file.Name src, customDeps⟩
}

MakeLinkerInv ← { 𝕊 GetArgs‿cache‿name‿srcs:
  dst ← cache.File name
  GetCMD ← { 𝕊:
    args ← GetArgs@
    ⟨"sh", rootDir, ⟨⊑args, "-o", dst⟩∾({𝕩.dst}¨ srcs)∾1↓args, @⟩
  }
  AddRule ⟨cache, name, dst, GetCMD, "link", srcs⟩
}



# actual CBQN/Singeli/REPLXX definitions
cachedBin‿linkerCache ← {
  Shorten ← {po.clangd? 𝕩; r ← {𝕩↓˜¯1-⊑'.'⊐˜⌽𝕩}¨ •file.Name¨ 𝕩 ⋄ ! ∧´ ∊r ⋄ r}
  cbqnSrc ← ∾{⌽(⊑𝕩)⊸•file.At¨ 1↓𝕩}¨ ⌽⟨
    ⟨"src/builtins/", "arithd.c", "arithm.c", "cmp.c", "sfns.c", "squeeze.c", "select.c", "slash.c", "group.c", "sort.c", "search.c", "selfsearch.c", "transpose.c", "fold.c", "scan.c", "md1.c", "md2.c", "fns.c", "sysfn.c", "internal.c", "inverse.c"⟩
    ⟨"src/core/", "tyarr.c", "harr.c", "fillarr.c", "stuff.c", "derv.c", "mm.c", "heap.c"⟩
    ⟨"src/", "load.c", "main.c", "rtwrap.c", "vm.c", "ns.c", "nfns.c", "ffi.c"⟩
    ⟨"src/jit/", "nvm.c"⟩
    ⟨"src/utils/", "ryu.c", "utf.c", "hash.c", "file.c", "mut.c", "each.c", "bits.c"⟩
  ⟩
  cbqnSrc ↩ cbqnSrc clangd.Files "src"
  singeliMap ← 1↓¨ (∨´¨ (⊑po.arch) = ⊑¨)⊸/ ⟨
    "xa"‿"src/builtins/arithm.c"‿"monarith",
    "xa"‿"src/core/stuff.c"‿"equal",        "xa"‿"src/utils/mut.c"‿"copy",       "xa"‿"src/utils/bits.c"‿"bits"
    "xa"‿"src/builtins/arithd.c"‿"dyarith", "xa"‿"src/builtins/cmp.c"‿"cmp",     "xa"‿"src/builtins/squeeze.c"‿"squeeze"
    "x."‿"src/builtins/select.c"‿"select",  "x."‿"src/builtins/fold.c"‿"fold",   "x."‿"src/builtins/scan.c"‿"scan"
    "x."‿"src/builtins/scan.c"‿"neq",       "x."‿"src/builtins/slash.c"‿"slash", "x."‿"src/builtins/slash.c"‿"constrep"
    "xa"‿"src/builtins/transpose.c"‿"transpose"
  ⟩
  objs ← ⟨⟩
  
  
  
  replxxCache ← {
    ¬po.replxx? @;
    replxxCache ← GetCache ⟨"replxx", "REPLXX object file location", po.REPLXXc@⟩
    
    replxxSrc ← (po.replxxDir•file.At"src")⊸•file.At¨⟨"ConvertUTF.cpp", "wcwidth.cpp", "conversion.cxx", "escape.cxx", "history.cxx", "prompt.cxx", "replxx.cxx", "replxx_impl.cxx", "terminal.cxx", "util.cxx", "windows.cxx"⟩
    objs∾↩ (Shorten replxxSrc) {MakeCCInv ⟨po.REPLXXc, ⊢, replxxCache, 𝕨, 𝕩, ⟨⟩⟩}¨ replxxSrc
    
    replxxCache
  }
  
  singeliObjs ← @
  singeliCache ← {
    ¬po.singeli? @;
    singeliCache ← GetCache ⟨"singeli", "Singeli generated code location", ⟨po.native, po.arch, po.singeliFlags, po.singeliDir⟩⟩
    
    # genArithTables
    ga ← "src/singeli/src/genArithTables.bqn"
    gaDefs ← singeliCache.File "arDefs.singeli"
    gaTables ← singeliCache.File "arTables.c"
    gaRule ← AddRule ⟨
      singeliCache, "genArithTables",
      gaDefs, # am cheating and only using arDefs.singeli as destination; ¯\_(ツ)_/¯
      {𝕊: ⟨"runbqn", rootdir, AtRoot ga, AtRoot¨ gaDefs‿gaTables, ⟨ga⟩⟩},
      •file.Name ga, ⟨⟩
    ⟩
    
    singeliArgs ← po.singeliFlags∾⟨"-l", "gen="∾AtRoot singeliCache.folder⟩∾{
      po.native? ⟨⟩;
      "-a" ⋈ {"x86-64":"X86_64"; "aarch64":"AARCH64"} po.arch
    }
    singeliObjs ↩ {MakeSingeliInv ⟨singeliArgs, {𝕊:UpdateSubmodule po.singeliDir}, singeliCache, 𝕩, "src/singeli/src/"•file.At 𝕩∾".singeli", (𝕩≡"dyarith")/⟨gaRule⟩⟩}¨ 1⊑¨singeliMap
    
    singeliCache
  }
  
  cbqnCache ← {
    cbqnCache ← GetCache ⟨"cbqn", "CBQN object file location", ⟨po.CBQNc@, {po.singeli? singeliCache.folderHash; @}⟩⟩
    ruleDeps ← {
      ¬po.singeli? ⟨⟩¨ cbqnSrc;
      ((≠cbqnSrc) ∾˜ cbqnSrc⊐⊑¨singeliMap) ⊔ singeliObjs
    }
    
    singeliArgs ← {po.singeli? ⟨"-DSINGELI_DIR="∾•file.Name singeliCache.folder⟩; ⟨⟩}
    objs∾↩ {a‿b‿c: MakeCCInv ⟨po.CBQNc∾po.ccColor∾singeliArgs˙, {𝕊:UpdateSubmodule po.bytecodeDir ⋄ UpdateSubmodule po.replxxDir}, cbqnCache, a, b, c⟩}¨ <˘⍉[Shorten cbqnSrc, cbqnSrc, ruleDeps] # updates replxx because needs replxx.h
    
    cbqnCache
  }
  
  linkerDeps ← @⊸≢¨⊸/ ⟨cbqnCache, replxxCache, singeliCache⟩
  linkerCache ← GetCache ⟨"linker", "linker cached result location", ⟨po.Linker@, {𝕩.folderHash}¨ linkerDeps⟩⟩
  {
    po.versionName≡@? @;
    srcFile ← linkerCache.File "versionInfo.c"
    src ← "char* cbqn_versionString = """∾(∾∾⟜"\n"¨ ×∘≠¨⊸/ ⟨
      "CBQN "∾{po.versionName≡""? "on commit "∾(GitCmd ⟨"rev-parse", "HEAD"⟩)∾{""≡GitCmd ⟨"status", "--porcelain", "src"⟩? ""; "-dirty"}; po.versionName}
      {"built with "⊸∾⍟(×≠) ¯2↓∾∾⟜", "¨ 𝕩} po.ffi‿po.singeli‿po.replxx‿(¬po.o3) / ⟨"FFI", "singeli "∾{po.native? "native"; po.arch}, "replxx", "optimizations disabled"⟩
      # "build invocation: "⊸∾⍟(×≠) ¯1↓∾∾⟜' '¨ •args
    ⟩)∾""";"∾@+10
    {
      •file.Exists AtRoot srcFile? src≡•FChars AtRoot srcFile? @; # don't update
      (AtRoot srcFile) •FChars src
    }
    objs∾↩ ⋈ MakeCCInv ⟨po.CBQNc, ⊢, linkerCache, "versionInfo", srcFile, ⟨⟩⟩
  }
  
  res ← MakeLinkerInv ⟨po.Linker, linkerCache, {po.emcc? "BQN.js"; "windows"≡po.os? "res.exe"; "res"}, objs⟩
  
  res.dst ⋈ linkerCache
}

outPath ← •wdpath •file.At po.output
{
  po.clangd? clangd.Finish@;
  # else, regular build
  success ← Run cachedBin
  { 𝕊:
    po.emcc?
      SH ⟨"cp", AtRoot cachedBin,                   outPath •file.At •file.Name cachedBin⟩
      SH ⟨"cp", AtRoot linkerCache.File "BQN.wasm", outPath •file.At "BQN.wasm"⟩
    ;
      SH ⟨"cp", "-f", AtRoot cachedBin, outPath⟩
  }⍟⊢ success
  OnExit@
  •Exit ¬success
}
