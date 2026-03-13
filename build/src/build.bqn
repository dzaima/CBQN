t0g ← •MonoTime@
# todo: forcing bytecode/singeli/replxx directories to whatever's applicable

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
  
  ⟨0, "v",       "", "Version string to report by --version; v=0 to disable"⟩
  ⟨0, "j",        @, "Number of parallel jobs"⟩
  ⟨1, "verbose",  0, "Log more things"⟩
  ⟨2, "rebuild", ⟨⟩, "Forcibly rebuild cbqn/replxx/singeli/linker"⟩
  ⟨2, "clean",   ⟨⟩, "Clean cbqn/replxx/singeli/linker and exit"⟩
  ⟨1, "dry-run",  0, "Don't run/modify/delete anything (except submodules)"⟩
  ⟨1,"stored-warn",0,"Print stored warnings from cached build steps"⟩
  ⟨1, "notui",    @, "Disable live-updating status display"⟩
  ⟨1, "nogit",    0, "Error if something attempts to use git"⟩
  ⟨1,"keep-going",0, "Continue building things even after others fail to"⟩
  ⟨1, "quiet",    0, "Don't output final status line (or any, combined with notui)"∾@+10⟩
  
  ⟨0, "CC",    @, "The used C compiler"⟩
  ⟨0, "LD",    @, "Linker of the final binary; defaults to CC, or CXX if REPLXX=1"⟩
  ⟨1, "color", @, "Whether to enable colored diagnostics; default based on notui"⟩
  ⟨0, "os",    @, "Target OS (linux, bsd, macos)"⟩ # used for .so vs .dylib, and changing linker flags for bsd
  ⟨0, "arch",  @, "Target architecture (x86-64, aarch64, generic)"⟩ # used for selecting Singeli target
  ⟨1, "target-from-cc",0, "Detect arch, OS, and target-supported extensions from CC"⟩
  ⟨1, "pie",   @, "Position-independent executable; default based on OS & arch"∾@+10⟩
  
  ⟨0, "OUTPUT",  "", "Output location; defaults to ./BQN for regular builds"⟩
  ⟨2, "f",       ⟨⟩, "C flags for CBQN files"⟩
  ⟨2, "CCFLAGS", ⟨⟩, "Flags for all CC/CXX invocations (both object file & linking)"⟩
  ⟨2, "CXXFLAGS",⟨⟩, "Flags for C++ object file creation invocations"⟩
  ⟨2, "lf",      ⟨⟩, "Linker flags"⟩
  ⟨2, "sf",      ⟨⟩, "Singeli flags"⟩
  ⟨2, "LDFLAGS", ⟨⟩, @⟩
  ⟨2, "rm_f",    ⟨⟩, "Forcibly remove C compiler flag(s)"⟩
  ⟨2, "rm_lf",   ⟨⟩, "Forcibly remove linker flag(s)"∾@+10⟩
  
  ⟨1, "shared", 0, "Build a shared library"⟩
  ⟨1, "static-lib", 0, "Build a static library"⟩
  ⟨1, "static-bin", 0, "Statically link the executable"⟩
  ⟨1, "wasi",   0, "Build with WASI; default output name is BQN.wasm"⟩
  ⟨1, "emcc",   0, "Build with emscripten; OUTPUT should be a folder"⟩
  ⟨1, "clangd", 0, "Generate a compile_commands.json for clangd"∾@+10⟩
  
  ⟨1, "FFI",       @, "Enable FFI through libffi; off by default for Windows"⟩
  ⟨1, "pkgconfig", 1, "Whether to attempt to use pkg-config to find libffi flags"⟩
  ⟨1, "singeli",   0, "Enable Singeli usage"⟩
  ⟨1, "replxx",    0, "Enable REPLXX"∾@+10⟩
  
  ⟨0, "usz",    @, "Size of array length fields in bits; 32 or 64 (default: 64)"⟩
  ⟨1, "c",      0, "Disable some default flags"⟩
  ⟨1, "O3",     @, "Use '-O3'; On by default unless 'c=1'"⟩
  ⟨1, "native", @, "Use '-march=native'"⟩
  ⟨2, "has",   ⟨⟩, "Assume specific arch features; options: AVX2, BMI2, slow-pdep"∾@+10⟩
  
  ⟨1, "debug",      0, "Debug: '-DDEBUG -g'; enables various sanity checks"⟩
  ⟨1, "g",          @, "Debug: '-g'; enable debug symbols"⟩
  ⟨1, "rtverify",   0, "Debug: compare builtin results with the self-hosted runtime"⟩
  ⟨1, "heapverify", 0, "Debug: check reference count correctness"⟩
  ⟨1, "ubsan",      0, "Debug: enable compiler undefined behavior sanitizer"∾@+10⟩
  
  ⟨0, "CXX", "c++", "C++ compiler (for replxx)"⟩
  ⟨2, "REPLXX_FLAGS", ⟨"-std=c++11", "-Os"⟩, "default replxx C++ build flags"⟩
⟩

Log ← •Out
_verboseLog ← {𝔽_𝕣:⊢}
onExitList ← ⟨⟩
OnExit ← {𝕊: {𝕏@}¨ onExitList}
_assert_ ← { 𝔾𝕩?𝕩; Log 𝕨𝔽𝕩 ⋄ OnExit@ ⋄ •Exit 1}

Split ← ((¬-˜⊢×·+`»⊸>)∘≠⊔⊢)
SepArgs ← ' '⊸Split
Lowercase ← {𝕩 - (-´"Aa")×(𝕩≥'A')∧𝕩≤'Z'}
Uppercase ← {𝕩 - (-´"aA")×(𝕩≥'a')∧𝕩≤'z'}

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

SH ← { # if 𝕨≢@, returns 𝕨 if failed to spawn
  defres ← 𝕨 ⊣ @
  c‿o‿e ← •SH⎊{defres≢@? ¯1‿defres‿@; (∾⟨"Error: Failed to spawn ",⊑𝕩⟩)_assert_⊢ 0} 𝕩
  {
    c≡¯1? o;
    Log⍟(×≠) e
    𝕩 {𝕊: ∾⟨"Error: ",⊑𝕨," exited with error code ",•Repr 𝕩⟩}_assert_(0=⊢) c
    o
  }𝕩
}
TrySH ← {1⊑•SH⎊1‿𝕨‿"" 𝕩}

nogit←1
GitCmd ← {
  𝕨 {"Error: git needed for "∾𝕨∾", but nogit=1 specified"}_assert_¬ nogit
  ¯1↓SH ⟨"git", "-C", rootDir⟩∾𝕩
}
updateSubmodule ← {
  done ← ⟨⟩
  {
    𝕩≡@? @;
    "Local"≡¯5↑𝕩? @;
    ⊑𝕩<⊸∊done? @;
    done∾↩ <𝕩
    ("updating "∾𝕩) GitCmd ⟨"submodule", "update", "--init", AtRoot 𝕩⟩
  }
}

rootDir ← •file.Parent⍟2 •file.path
AtRoot ← rootDir⊸•file.At
allCaches ← ⟨"cbqn", "singeli", "replxx", "linker"⟩

po ← { # parsed options
  ⟨
    verbose, J, clean, rebuild, keepGoing, warnStored, tui, loud, output, clangd, versionName,
    os, arch, native, has,
    emcc, wasm, replxx, singeli, staticLib,
    usz, ffi, o3, debug, rtverify, heapverify,
    bytecodeDir, replxxDir, singeliDir,
    CBQNc, ccColor, singeliFlags, singeliCFlags, singeliArch, REPLXXc, Linker
  ⟩⇐
  
  compat ← GetOpt "from-makefile"
  compatTgt ← compat/"target_"
  
  custom ← GetOpt "c"
  clangd ← GetOpt "clangd"
  DOpt ← {@⊸≡◶⟨⊢,𝕨⟩ GetOpt 𝕩}
  J ⇐ {𝕊: v←GetOpt "j" ⋄ v≢@? •ParseFloat v; •ParseFloat⎊4 ¯1↓"4" TrySH ⟨"nproc"⟩}
  tui ⇐ @⊸≡◶¬‿{𝕊: f←@•FFI"i32"‿"isatty"‿">i32" ⋄ F 1} GetOpt "notui"
  warnStored ⇐ GetOpt "stored-warn"
  keepGoing ⇐ GetOpt "keep-going"
  loud ⇐ ¬GetOpt "quiet"
  nogit ↩ GetOpt "nogit"
  truecc ⇐ GetOpt "target-from-cc"
  
  dry ⇐ GetOpt "dry-run"
  CacheList ← {
    vs ← ∾','Split⟜Lowercase¨ GetOpt 𝕩
    {
      vs≡⟨"all"⟩? allCaches;
      𝕩 {∾⟨"Error: Invalid target in ",𝕨," list: ",⊑𝕩, " (valid options: ", 2↓∾", "⊸∾¨ allCaches, ")"⟩}_assert_(0=≠) vs (¬∘∊/⊣) allCaches
      vs
    }𝕩
  }
  clean   ⇐ CacheList "clean"
  rebuild ⇐ CacheList "rebuild"
  
  {𝕊: _verboseLog ↩ {Log𝕨𝔽𝕩⋄𝕩}}⍟⊢ verbose⇐GetOpt "verbose"
  
  ErrBoth ← {a‿b: ∾⟨"Error: '",a,"' and '",b,"' cannot both be enabled"⟩}
  Max1 ← {𝕩 ErrBoth∘(2↑/˜) _assert_(1≥+´) GetOpt¨ 𝕩}
  NotWith ← {𝕩 {ErrBoth ⊑¨ 𝕩/¨𝕨}_assert_(¬·∧´∨´¨) GetOpt¨¨ 𝕩}∘⋈
  
  Max1 "REPLXX"‿"wasi"‿"emcc"
  Max1 "shared"‿"static-lib"
  "static-lib"‿"shared" NotWith "REPLXX"‿"emcc"
  
  
  sharedLib ← GetOpt "shared"
  staticLib ⇐ GetOpt "static-lib"
  staticBin ⇐ GetOpt "static-bin"
  lib ← staticLib ∨ sharedLib
  native ⇐ 0 DOpt "native"
  emcc ⇐ GetOpt "emcc"
  wasi ← GetOpt "wasi"
  versionName ← "0"⊸≡◶⊢‿@ GetOpt "v"
  wasm ⇐ wasi∨emcc
  "Error: Static linking for wasm is pointless"_assert_¬ staticBin ∧ wasm
  
  usz ⇐ •ParseFloat {wasm? "32"; "64"} DOpt "usz"
  {𝕊: "Error: usz can only be 32 or 64"}_assert_⊢ ⊑usz∊32‿64
  
  cc ← {emcc? "emcc"; "clang"} DOpt "CC"
  ccClang ← {
    clangd? 1;
    TryVersion ← {𝕩 SH cc‿"--version"}
    vs ← TryVersion {@≡GetOpt"CC"? ¯1; @}
    
    {𝕊: Log⍟loud "Note: default CC '"∾cc∾"' wasn't found; using 'cc'; add 'CC=cc' to hide" ⋄ cc↩"cc" ⋄ vs↩TryVersion@}⍟⊢ vs≡¯1
    ∨´"clang"⍷vs
  }
  
  cxx ← GetOpt "CXX"
  debug ⇐ GetOpt "debug"
  rtverify ⇐ GetOpt "rtverify"
  heapverify ⇐ GetOpt "heapverify"
  ubsan ⇐ GetOpt "ubsan"
  o3 ⇐ (¬custom) DOpt "O3"
  
  allFlags ← GetOpt "CCFLAGS"
  minCCFlags ← ∾⟨allFlags, GetOpt "f", native/⟨"-march=native"⟩⟩
  ubsanFlags ← ubsan / ⟨"-fsanitize=undefined", "-fno-sanitize=vla-bound"⟩
  
  CDefines ← {𝕊: cDefines ↩ (@+10) Split SH ⟨cc⟩ ∾ minCCFlags ∾ ⟨"-dM", "-E", "-"⟩}
  
  Uname ← {𝕊: uname ↩ Lowercase "" TrySH "uname"‿"-sm"}
  InUname ← {𝕊: ∨´ 𝕩⍷Uname@}
  
  singeli ⇐ GetOpt "singeli"
  replxx ⇐ GetOpt "REPLXX"
  
  SubmoduleDir ← "build"⊸•file.At⍟(@≢⊢) {𝕨≡0?@; •file.Exists r←AtRoot "build"•file.At 𝕩∾"Local"? r; 𝕩∾"Submodule"}
  LogDir ← {𝕩≡@? 𝕨∾": not used"; 𝕨∾" directory: "∾AtRoot 𝕩}_verboseLog
  bytecodeDir ⇐ 1       SubmoduleDir "bytecode" ⋄ "Bytecode" LogDir bytecodeDir
  replxxDir   ⇐ replxx  SubmoduleDir "replxx"   ⋄ "REPLXX" LogDir replxxDir
  singeliDir  ⇐ singeli SubmoduleDir "singeli"  ⋄ "Singeli" LogDir singeliDir
  
  arch ⇐ {'-'¨⌾(('_'=𝕩)⊸/)𝕩} Lowercase {truecc∨wasm? "generic"; InUname"x86_64"? "x86-64"; ∨´InUname¨"aarch64"‿"arm64"? "aarch64"; "generic"} DOpt "arch"
  {𝕊: "Error: Unsupported arch; options:"∾1↓∾", "⊸∾¨𝕩}_assert_(⊑arch<⊸∊⊢) "x86-64"‿"aarch64"‿"riscv64"‿"generic"
  arch {"riscv64": "RV64"; "generic": "NONE"; Uppercase {'_'¨⌾(('-'=𝕩)⊸/)𝕩} 𝕩}↩ # translate to Singeli
  
  has ⇐ ∾ ','Split¨Lowercase GetOpt "has"
  has ↩ ∧⍷has ∾ (⊑"slow-pdep"<⊸∊ has) / ⟨"bmi2"⟩
  {𝕊: "Error: Invalid 'has' value; options:"∾1↓∾", "⊸∾¨𝕩}_assert_(∧´has∊⊢) "ssse3"‿"sse4.1"‿"sse4.2"‿"avx2"‿"bmi2"‿"pclmul"‿"slow-pdep"
  "Error: Cannot use 'has=...' if Singeli isn't enabled"_assert_⊢ singeli ∨ ⟨⟩≡has
  
  {𝕊: has∾↩ (∨´(("#define __tune_znver"⊸∾∾⟜"__ 1")¨ "12")∊CDefines@) / ⟨"slow-pdep"⟩}⍟⊢ truecc
  
  singeliArch ⇐ @
  singeliCFlags ⇐ ⟨⟩
  SingeliArchInfo ⇐ ! # takes arg of infer
  
  {
    ¬singeli? @;
    UpdateSubmodule singeliDir
    SingeliUtil ← {𝕨 •Import AtRoot singeliDir •file.At 𝕩}
    SingeliArchInfo ↩ {⟨{native∧¬truecc? ⟨⟩; singeliArch}, 𝕩⟩ SingeliUtil "arch.bqn"}
    singeliArch ↩ {
      truecc ∧ @≡GetOpt"arch"?
        archutil ← SingeliUtil "archutil.bqn"
        foundArch ← archutil.ParseCDefines CDefines@
        arch ↩ ⊑foundArch
        foundArch
      ;
        Uppercase ⟨arch⟩
    }
    {
      arch≡"X86_64"?
        singeliArch∾↩ Uppercase ∊⟜"ssse3"‿"sse4.1"‿"sse4.2"‿"avx2"‿"bmi2"‿"pclmul"⊸/ has
        singeliCFlags ↩ (SingeliArchInfo 1).GetCFlags @
        {𝕊: Log "Note: inferred the following C flags:"∾∾' '⊸∾¨𝕩}⍟(loud ∧ ×∘≠) singeliCFlags (¬∘∊/⊣) (SingeliArchInfo 0).GetCFlags @
      ;@
    }
  }
  
  {𝕊: "Error: Cannot have 'has' options on architecture '"∾arch∾"'; remove 'has=...' or use '"∾compatTgt∾"arch=x86-64'"}_assert_¬ (arch≢"X86_64") ∧ 0≠≠has
  
  os ⇐ GetOpt "os"
  {
    os≢@?
      os Lowercase ↩;
    ¬truecc?
      os ↩ {InUname"linux"? "linux"; InUname"darwin"? "macos"; InUname"bsd"? "bsd"; "linux"};
    osKeys‿osVals ← <˘⍉>⟨
      ⟨"_WIN32", "windows"⟩
      ⟨"__FreeBSD__", "bsd"⟩
      ⟨"__OpenBSD__", "bsd"⟩
      ⟨"__APPLE__", "macos"⟩
      ⟨"__MACH__", "macos"⟩
      ⟨"__linux", "linux"⟩
    ⟩
    osOpts ← ⍷ osVals /˜ osKeys∊{𝕩↑˜⊑𝕩⊐' '}¨ 8↓¨ ("#define "≡8↑⊢)¨⊸/ CDefines@
    {𝕊: "Error: Couldn't detect OS from C-defined macros; add explicit '"∾compatTgt∾"os=...'"} _assert_⊢ 1=≠osOpts
    os ↩ ⋈⁼ osOpts
  }
  linux‿bsd‿macos‿windows ← os⊸≡¨ {𝕊: "Error: Unsupported OS; options:"∾1↓∾", "⊸∾¨𝕩}_assert_(⊑os<⊸∊⊢) "linux"‿"bsd"‿"macos"‿"windows"
  
  {𝕊: ∾⟨"Target: ", os, " ", Lowercase arch, 1↓ ∾", "⊸∾¨ has∾native/⋈"native"⟩} _verboseLog@
  
  output ⇐ GetOpt "OUTPUT"
  output ↩ •wdpath•file.At {𝕊:
    wasi? "BQN.wasm";
    emcc? ".";
    staticLib? "libcbqn.a";
    sharedLib? {macos? "libcbqn.dylib"; windows? "cbqn.dll"; "libcbqn.so"};
    windows? "BQN.exe";
    "BQN"
  }⍟(""≡⊢) output
  {𝕊: "Output location: "∾𝕩}_verboseLog output
  
  pie ⇐ {(¬sharedLib∨staticLib)∧linux∧arch≡"X86_64"? 0; 1} DOpt "pie"
  {𝕊: pie ↩ 0}⍟⊢ wasm
  
  ffi ⇐ ((¬windows)∧¬wasm) DOpt "FFI"
  
  exportSymbols ← ffi∨lib∨wasm
  
  defLibs ← @≡GetOpt "LD_LIBS"
  GetLibs ← { 𝕊:
    getLibs ↩ ⟨⟩⌾(1⊸⊑)⍟(¬defLibs) {
      ¬ffi? ⟨⟨⟩,⟨⟩⟩;
      GetOpt "pkgconfig"? 0=⊑ •SH⎊1 "pkg-config"‿"--exists"‿"libffi"?
        {SepArgs ¯1↓SH⟨"pkg-config",𝕩,"libffi"⟩}¨ ⟨"--cflags", "--libs"⟩;
      ⟨⟨⟩, ⟨"-lffi"⟩⟩
    }
  }
  
  ccColor ⇐ ((¬clangd) ∧ tui DOpt "color") / ⟨ccClang⊑"-fdiagnostics-color=always"‿"-fcolor-diagnostics"⟩
  debuginfo ← debug DOpt "g"
  
  cbqnc ← {
    args ← ⟨
      cc,
      "-std=gnu11",
      "-Wall", "-Wno-unused-function",
      "-fms-extensions", "-ffp-contract=off", "-fno-math-errno", "-fvisibility=hidden", "-fno-strict-aliasing",
      "-iquote", "src",
      "-DBYTECODE_DIR="∾•file.Name bytecodeDir,
      "-DSINGELI="∾•Repr singeli,
      "-DFFI="∾•Repr ffi
    ⟩
    args∾↩ ccClang⊑⟨
      ⟨"-Wno-parentheses"⟩ # gcc
      ⟨ # clang
        "-Wno-microsoft-anon-tag", "-Wno-bitwise-instead-of-logical", "-Wno-unknown-warning-option"
        "-Wno-pass-failed" # vfor
      ⟩ ∾ (¬wasm)/⟨"-mllvm", "--x86-cmov-converter=0"⟩ # thing that converts `cmov`s to branching sometimes (?), but we don't want that
    ⟩
    args∾↩ 0⊑GetLibs@
    
    args∾↩ minCCFlags
    args∾↩ ubsanFlags
    args∾↩ (singeli ∧ arch≡ "X86_64") / ⟨"-DSINGELI_X86_64"⟩ ∾ {singeli? ⟨"-mavx2"⟩ ∊ singeliCFlags; 0} / ⋈"-DSINGELI_AVX2"
    args∾↩ (singeli ∧ arch≡"AARCH64") / ⟨"-DSINGELI_NEON"⟩
    args∾↩ (singeli ∧ ⊑arch<⊸∊"X86_64"‿"AARCH64") / ⟨"-DSINGELI_SIMD"⟩
    args∾↩ (            wasm) / ⟨"-DWASM"⟩
    args∾↩ (            wasi) / ⟨"-DWASI", "-DNO_MMAP", "-DUSE_SETJMP=0", "-D_WASI_EMULATED_MMAN", "--target=wasm32-wasi"⟩
    args∾↩ (            emcc) / ⟨"-DEMCC", "-O3"⟩
    args∾↩ (          replxx) / ⟨"-DUSE_REPLXX", "-DREPLXX_STATIC=1", "-I"∾replxxDir∾"/include"⟩ # TODO maybe move to main.c only, and have it be in its own separate cache dir, so that adding replxx doesn't recompile everything?
    args∾↩ (       debuginfo) / ⟨"-g"⟩
    args∾↩ (              o3) / ⟨"-O3"⟩
    args∾↩ (           debug) / ⟨"-DDEBUG"⟩
    args∾↩ (        rtverify) / ⟨"-DRT_VERIFY"⟩
    args∾↩ (      heapverify) / ⟨"-DHEAP_VERIFY", "-DUSE_SETJMP=0"⟩
    args∾↩ (   exportSymbols) / ⟨"-DCBQN_EXPORT"⟩
    args∾↩ (            ¬pie) / ⟨"-fno-pie"⟩
    args∾↩ (pie ∧ ¬sharedLib) / ⟨"-fPIE"⟩
    args∾↩ (             lib) / ⟨"-DCBQN_LIB"⟩
    args∾↩ (       sharedLib) / (¬GetOpt"no_fPIC")/⟨"-fPIC"⟩
    args∾↩ (         windows) / ⟨"-DNO_MMAP"⟩
    args∾↩ (replxx ∧ windows) / ⟨"-DUSE_REPLXX_IO"⟩
    args∾↩ (   @≢versionName) / ⟨"-DHAS_VERSION"⟩
    args∾↩ (          usz≡64) / ⟨"-DUSZ_64"⟩
    args∾↩ (⊑"slow-pdep"<⊸∊ has) / ⟨"-DSLOW_PDEP"⟩
    args∾↩ (         ¬native) / singeliCFlags
    args ↩ args (¬∘∊/⊣) GetOpt "rm_f"
    {"CBQN C compiler: "∾•Repr 𝕩} _verboseLog args
    args
  }
  
  REPLXXc ⇐ { 𝕊:
    args ← ⟨cxx, "-DREPLXX_STATIC=1", "-I"∾replxxDir∾"/include"⟩
    args∾↩ GetOpt "REPLXX_FLAGS"
    args∾↩ GetOpt "CXXFLAGS"
    args∾↩ (            ¬pie) / ⟨"-fno-pie"⟩
    args∾↩ allFlags
    args∾↩ (pie ∧ ¬sharedLib) / ⟨"-fPIE"⟩
    args∾↩ (       sharedLib) / (¬GetOpt"no_fPIC")/⟨"-fPIC"⟩
    {"REPLXX C++ compiler: "∾•Repr 𝕩} _verboseLog args
    replxxc ↩ args
  }
  
  singeliFlags ⇐ GetOpt "sf"
  
  linker ⇐ {
    bin ← @
    args ← ⟨⟩
    Cmd ← !
    {
      staticLib?
        bin ↩ "ar" DOpt "LD"
        args∾↩ GetOpt "lf"
        args∾↩ GetOpt "LDFLAGS"
        args∾↩ ⟨"-rcs"⟩
        Cmd ↩ {srcs‿dst: ⟨bin⟩∾args∾⟨dst⟩∾srcs}
      ;
        rdynamic ← (¬wasi) ∧ exportSymbols ∧ ¬windows
        bin ↩ {replxx? cxx; cc} DOpt "LD"
        args∾↩ emcc / ⟨
          "-sEXPORTED_FUNCTIONS=_main,_cbqn_runLine,_cbqn_evalSrc,_malloc,_free"
          "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,HEAPU8"
          "-sALLOW_MEMORY_GROWTH=1"
          "-sSTACK_SIZE="∾•Repr 1024×1024
        ⟩ ∾ debug / "-sASSERTIONS"‿"-sSTACK_OVERFLOW_CHECK=2"
        args∾↩ ubsanFlags
        args∾↩ defLibs / ⟨"-lm"⟩
        args∾↩ ⟨⟩ DOpt "LD_LIBS"
        args∾↩ ((¬GetOpt "NO_LDL") ∧ defLibs ∧ ffi ∧ ¬bsd) / ⟨"-ldl"⟩
        args∾↩ 1⊑GetLibs@
        args∾↩ GetOpt "lf"
        args∾↩ allFlags
        args∾↩ GetOpt "LDFLAGS"
        args∾↩ (           wasi) / ⟨"-lwasi-emulated-mman", "--target=wasm32-wasi", "-Wl,-z,stack-size=8388608", "-Wl,--initial-memory=67108864"⟩
        args∾↩ (sharedLib∧ wasi) / ⟨"-mexec-model=reactor", "-Wl,--export-dynamic", "-Wl,--export=malloc", "-Wl,--export=free"⟩
        args∾↩ (sharedLib∧¬wasi) / ⟨"-shared"⟩
        args∾↩ (sharedLib∧macos) / ⟨"-install_name", "@rpath/libcbqn.dylib"⟩
        args∾↩ (       rdynamic) / ⟨"-rdynamic"⟩
        args∾↩ ((¬wasm)∧   ¬pie) / ⟨"-no-pie"⟩
        args∾↩ (pie∧ ¬sharedLib) / ⟨"-fPIE", "-pie"⟩
        args∾↩ (        windows) / ⟨"-lpthread"⟩
        args∾↩ (      staticBin) / ⟨"-static"⟩
        args∾↩ ( debuginfo∧emcc) / ⟨"-gsource-map"⟩
        args ↩ args (¬∘∊/⊣) GetOpt "rm_lf"
        Cmd ↩ {srcs‿dst: ⟨bin, "-o", dst⟩∾srcs∾args}
    }
    {"key":bin‿args; srcs‿dst: Cmd srcs‿dst}
  }
  {"linker: "∾•Repr Linker ⟨"src1"‿"src2"‿"...", "dst"⟩} _verboseLog@
  
  {𝕊: CBQNc@ ⋄ REPLXXc⍟replxx @}⍟⊢ verbose
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

# gets/creates a directory of cacheable objects; key is the unique identifier of when it can be reused
madeCaches ← ⟨⟩
GetCache ← {madeCaches∾↩𝕩⋄𝕩} { 𝕊 basename‿desc‿key:
  folderHash ⇐ Hash key
  folder ⇐ allObjDir•file.At basename∾"-"∾folderHash
  MkDir AtRoot folder
  
  name ⇐ basename
  ! ⊑(<name) ∊ allCaches
  
  File ⇐ folder⊸•file.At
  {𝕊: desc∾": "∾AtRoot𝕩} _verboseLog folder
  
  dataPath ← AtRoot File "data"
  dataVersion ← 1
  prevKs‿prevVs ← {a‿b: a≡dataVersion? b; ⟨⟩‿⟨⟩} •file.Exists◶⟨@, {Deserialize ⟨8‿'c',8⟩•bit._cast •FBytes 𝕩}⟩ dataPath
  
  Find ⇐ {
    i ← ⊑prevKs⊐<𝕩.cacheKey
    i<≠prevKs? i⊑prevVs;
    @
  }
  IsUpToDate ⇐ {
    𝕊:
      𝕩.found0≢@?
      info‿depInfo ← 𝕩.found0
      info≢@?
      ⟨fileInfo,·⟩ ← info
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
  onExitList∾↩ {{𝕊: dataPath •FBytes ⟨8,8‿'c'⟩•bit._cast FinalData@}⍟{𝕊: (0≠≠newKs) ∧ ¬po.dry}}
}

ruleKs‿ruleVs ← ⟨⟩‿⟨⟩
anyStoredWarnings ← 0

# dependency resolution & thread management
Run ← { 𝕊:
  0≢≠po.clean?
    {
      Log⍟(po.verbose∨po.dry) ∾⟨"recursively deleting ", AtRoot 𝕩.folder⟩
      SH⍟(¬po.dry) ⟨"rm", "-r", "--", AtRoot 𝕩.folder⟩
    }¨ (⊢ /˜ {𝕩.name}¨ ∊ po.clean˙) madeCaches
    •Exit 0
    ;
  ruleDeps ← {{𝕩.dst}¨ 𝕩.ruleDeps}¨ ruleVs
  ruleSrcs0 ← ruleKs⊐∾ruleDeps
  ! ∧´ ruleSrcs0<≠ruleKs
  ruleSrcs ← ((≠∾˜≠¨/↕∘≠)ruleDeps) ⊔ ruleSrcs0
  req ← ⟨⟩
  ruleN ← 0¨ ruleKs # number of children (i.e. how many have this in their ruleP list)
  ruleP ← ⟨⟩¨ ruleKs # parent rules (i.e. which ones require this)
  
  Require ← {
    v ← 𝕩⊑ruleVs
    # rebuild ← po.rebuildAll
    # {𝕊: rebuild∨↩ ".singeli"≡¯8↑v.disp}⍟⊢ po.rebuildSingeli
    rebuild ← ⊑(<v.cache.name)∊po.rebuild
    chi ← 𝕩⊑ruleSrcs
    chr ← Require¨ chi
    rebuild∨↩ ∨´ chr
    # rebuild∨↩ ¬•file.Exists AtRoot 𝕩⊑ruleKs # not really needed unless someone deletes a specific file without deleting the data file
    rebuild∨↩ ¬v.cache.IsUpToDate v
    rebuild◶⟨
      {𝕊: {
        po.warnStored? Log ∾⟨"Previous warning from ",v.disp,":"⟩ ⋄ Log 𝕩;
        anyStoredWarnings ↩ 1
      }⍟(×≠) 1⊑0⊑v.found0}
      {
        ruleN (+´chr)⊸+⌾(𝕩⊸⊑)↩
        ruleP ∾⟜𝕩¨⌾((chr/chi)⊸⊏)↩
        req∾↩ 𝕩
      }
    ⟩ 𝕩
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
        v v.cache.Update ⟨3⊑tb, 1⊑tb⟩
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
      •term.OutRaw (¯4↓∾(≠currLive)⥊<((@+13)∾e∾"[0K"∾e∾"[1F")) ∾ (•ToUTF8 ∾storedOut) ∾ 1↓∾((@+10)∾•ToUTF8)¨ 𝕩
      •term.Flush @
      currLive ↩ 𝕩
      storedOut ↩ ⟨⟩
    }
  }⍟po.tui 0
  FmtTime ← {{(""≡◶⊢‿"0" ¯1↓𝕩)∾'.'∾¯1↑𝕩} •Repr ⌊0.5+ 10×𝕩}
  onExitList∾↩ {{𝕊: UpdateLive ⟨⟩}}
  
  tmap ← ⟨⟩ # threads in the order they're displayed on-screen (dynamically calculated so that if there's only ever only one job in parallel, there are no pointless empty lines)
  doneCount ← 0
  failed ← 0
  Fail ← {failed↩1 ⋄ Log 𝕩}
  nextRedraw ← ¯∞
  DoneLine ← {𝕊: ∾⟨•Repr doneCount, "/", •Repr ≠req⟩}
  { 𝕊:
    {𝕊:
      t ← ¯1⊑free ⋄ free↓˜↩ ¯1
      
      i ← RequestJob@ ⋄ v←i⊑ruleVs
      t.Request "dry"⊸⋈⍟po.dry v.CMD@
      work∾↩ {t⇐t, i⇐i, v⇐v, t0⇐•MonoTime@}
      
    }•_while_{𝕊: ∧´0<≠¨ left‿free}⍟⊢ po.keepGoing ∨ ¬failed
    
    t1 ← •MonoTime@
    tmap ↩ ⍷tmap∾Ts work
    FileLine ← {∾⟨"[", FmtTime t1-𝕩.t0, "] ", 𝕩.v.disp⟩}
    { 𝕊:
      nextRedraw ↩ t1+0.1
      UpdateLive (<DoneLine@)∾{
        𝕩≡≠work? "";
        FileLine 𝕩⊑work
      }¨ (Ts work)⊐tmap
    }⍟⊢ t1>nextRedraw-0.02
    
    dm ← (0.1⌊nextRedraw-t1) WaitForOne Ts work
    lm ← ¬dm
    {𝕊: Fail ∾⟨"Error: Fork processing ", •Repr (⊑(dm=¯1)/work).v.disp, " died"⟩ ⋄ dm‿lm∧↩<dm≠¯1}⍟⊢ ∨´dm=¯1
    done ← dm/work
    work ↩ lm/work
    {
      𝕩.i FinishJob 𝕩.t.Take@?
        {𝕊: Log FileLine 𝕩 ⋄ •term.Flush@}⍟(po.loud∧¬po.tui) 𝕩
        doneCount+↩1;
      Fail "Error: During '"∾𝕩.v.disp∾"'"
    }¨ done
    free∾↩ Ts done
  }•_while_{𝕊: (0<≠work) ∨ (¬failed)∧0<≠left}@
  
  Log⍟po.loud ∾⟨DoneLine@, " in ", FmtTime t0g-˜•MonoTime@, "s", failed/"; failed to build", anyStoredWarnings/"; cached build steps had warnings - view with stored-warn=1"⟩
  
  ¬failed
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

clangd ← {¬po.clangd? {_addC⇐{𝕨𝔽_𝕣𝕩:@} ⋄ AddSingeli⇐{𝕊:@} ⋄ SingeliInit⇐⊢ ⋄ CFiles⇐⊣}; ⟨AtRoot⟩ •Import "clangd.bqn"}

MakeCCInv ← { 𝕊 GetArgs‿Init‿cache‿id‿src‿customDeps: # src should be CBQN-base-dir-relative, so that cache doesn't store full paths
  GetArgs clangd._addC src
  dst ← cache.File id∾".o"
  GetCMD ← { 𝕊:
    Init @
    dep ← AtRoot cache.File id∾".d"
    ⟨"sh", rootDir, (GetArgs@)∾⟨"-MT", "o", "-MMD", "-MF", dep, "-o", AtRoot dst, "-c", src⟩, dep⟩
  }
  AddRule ⟨cache, id, dst, GetCMD, •file.Name src, customDeps⟩
}

MakeSingeliInv ← { 𝕊 args‿Init‿cache‿id‿src‿customDeps: # src should be CBQN-base-dir-relative, so that cache doesn't store full paths
  args clangd.AddSingeli src
  dst ← cache.File id∾".c"
  GetCMD ← { 𝕊:
    Init @
    ⟨"singeli", rootDir, AtRoot dst, AtRoot po.singeliDir, args, AtRoot src⟩
  }
  AddRule ⟨cache, id, dst, GetCMD, •file.Name src, customDeps⟩
}

MakeLinkerInv ← { 𝕊 GetArgs‿cache‿name‿srcs:
  dst ← cache.File name
  GetCMD ← { 𝕊:
    cmd ← ⟨"sh", rootDir, GetArgs ⟨{𝕩.dst}¨ srcs, dst⟩, @⟩
    {po.staticLib? ⟨"staticlib", AtRoot dst, cmd⟩; cmd}
  }
  AddRule ⟨cache, name, dst, GetCMD, "link", srcs⟩
}



# actual CBQN/Singeli/REPLXX definitions
cachedBin‿linkerCache ← {
  Shorten ← {po.clangd? 𝕩; r ← {𝕩↓˜¯1-⊑'.'⊐˜⌽𝕩}¨ •file.Name¨ 𝕩 ⋄ ! ∧´ ∊r ⋄ r}
  cbqnSrc ← ∾{⌽(⊑𝕩)⊸•file.At¨ 1↓𝕩}¨ ⌽⟨
    ⟨"src/builtins/", "arithd.c", "arithm.c", "cmp.c", "sfns.c", "squeeze.c", "select.c", "slash.c", "group.c", "sort.c", "search.c", "selfsearch.c", "transpose.c", "fold.c", "scan.c", "md1.c", "md2.c", "compare.c", "cells.c", "fns.c", "sysfn.c", "internal.c", "inverse.c", "bit.c", "ffi.c"⟩
    ⟨"src/core/", "tyarr.c", "harr.c", "fillarr.c", "stuff.c", "derv.c", "mm.c", "heap.c", "ns.c"⟩
    ⟨"src/", "main.c"⟩
    ⟨"src/vm/", "load.c", "vm.c", "rtwrap.c"⟩
    ⟨"src/vm/jit/", "nvm.c"⟩
    ⟨"src/utils/", "ryu.c", "utf.c", "hash.c", "file.c", "mut.c", "each.c", "bits.c", "nfns.c"⟩
  ⟩
  cbqnSrc ↩ cbqnSrc clangd.CFiles "src"
  singeliMap ← 1↓¨ ({⊑ ({"X86_64":'x'; "AARCH64":'a'; "RV64":'g'; "NONE":'g'} po.arch) ∊ 𝕩}¨ ⊑¨)⊸/ ⟨
    "xa."‿"src/builtins/arithm.c"‿"monarith", "xa."‿"src/builtins/compare.c"‿"equal",
    "xa."‿"src/builtins/arithd.c"‿"dyarith",  "xa."‿"src/builtins/cmp.c"‿"cmp",
    "xa."‿"src/builtins/squeeze.c"‿"squeeze", "xa."‿"src/utils/mut.c"‿"copy",
    "xa."‿"src/utils/bits.c"‿"bits",          "xag"‿"src/builtins/transpose.c"‿"transpose",
    "xag"‿"src/builtins/search.c"‿"search",   "xag"‿"src/builtins/selfsearch.c"‿"selfsearch"
    "xag"‿"src/builtins/scan.c"‿"scan",       "xag"‿"src/builtins/fold.c"‿"fold",
    "xag"‿"src/builtins/slash.c"‿"slash",     "xag"‿"src/builtins/slash.c"‿"replicate",
    "xag"‿"src/builtins/sort.c"‿"bins",       "xa."‿"src/builtins/slash.c"‿"count"
    "xag"‿"src/builtins/select.c"‿"select",   "xag"‿"src/builtins/group.c"‿"group"
    "xag"‿"src/builtins/bit.c"‿"bit_arith",   "xa."‿"src/builtins/sort.c"‿"sort",
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
    singeliCache ← GetCache ⟨"singeli", "Singeli generated code location", ⟨po.native, po.singeliArch, po.arch, po.usz, po.has, po.singeliFlags, po.singeliDir⟩⟩
    
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
    
    singeliArgs ← ∾⟨
      po.singeliFlags
      ⟨"-l", "gen="∾AtRoot singeliCache.folder, "-c", "usz=u"∾•Repr po.usz⟩
      (⊑"slow-pdep"<⊸∊ po.has) / ⟨"-c", "SLOW_PDEP=1"⟩
      {po.native∧¬po.truecc? ⟨⟩; "-a" ⋈ 1↓∾ ','⊸∾¨ po.singeliArch}
    ⟩
    clangd.SingeliInit po
    {𝕊: "Singeli args: "∾•Repr singeliArgs} _verboseLog @
    {𝕊: "Singeli-determined C arch: "∾•Repr po.singeliCFlags; @} _verboseLog @
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
  linkerCache ← GetCache ⟨"linker", "linker cached result location", ⟨po.Linker "key", {𝕩.folderHash}¨ linkerDeps⟩⟩
  {
    po.versionName≡@? @;
    srcFile ← linkerCache.File "versionInfo.c"
    List ← {¯2↓ ∾ ∾⟜", "¨ /´ <˘⍉> 𝕩}
    
    vnameX‿vname ← {po.versionName≡""? "on "⊸∾⊸⋈ "commit "∾("??"GitCmd ⟨"rev-parse", "HEAD"⟩)∾{""≡"determining version"GitCmd ⟨"status", "--porcelain", "src"⟩? ""; "-dirty"}; ⋈˜ po.versionName}
    
    src ← ∾⟨
      "char* const cbqn_versionInfo = """
      
      ∾∾⟜"\n"¨ ×∘≠¨⊸/ ⟨
        "CBQN "∾vnameX
        "built with "⊸∾⍟(×≠) List ⟨
         ⟨po.ffi, "FFI"⟩
         ⟨po.singeli, "singeli "∾{po.native? "native "∾po.arch; po.arch≡"generic"? po.arch∾" (no SIMD)"; po.arch}∾∾' '∾¨po.has⟩
         ⟨po.replxx, "replxx"⟩
         ⟨¬po.o3, "optimizations disabled"⟩
         {⟨×≠𝕩, "debugging checks enabled ("∾𝕩∾")"⟩} List ⟨po.debug‿"DEBUG", po.heapverify‿"heapverify", po.rtverify‿"rtverify"⟩
        ⟩
      ⟩
      # "build invocation: "⊸∾⍟(×≠) ¯1↓∾∾⟜' '¨ •args
      
      """;", @+10
      "char* const cbqn_versionString = """∾vname∾""";", @+10
    ⟩
    {
      •file.Exists AtRoot srcFile? src≡•FChars AtRoot srcFile? @; # don't update
      (AtRoot srcFile) •FChars src
    }
    objs∾↩ ⋈ MakeCCInv ⟨po.CBQNc, ⊢, linkerCache, "versionInfo", srcFile, ⟨⟩⟩
  }
  
  res ← MakeLinkerInv ⟨po.Linker, linkerCache, {po.emcc? "BQN.js"; "windows"≡po.os? •file.Name po.output; "res"}, objs⟩
  
  res.dst ⋈ linkerCache
}

outPath ← •wdpath •file.At po.output
{
  po.clangd? clangd.Finish ¬po.dry;
  # else, regular build
  success ← Run cachedBin
  { 𝕊:
    po.emcc?
      Cp ← {
        src ← AtRoot linkerCache.File 𝕩
        SH⍟(•file.Exists src) ⟨"cp", "--", src, outPath •file.At 𝕩⟩
      }
      Cp "BQN.js"
      Cp "BQN.wasm"
      Cp "BQN.wasm.map"
    ;
      SH ⟨"cp", "-f", "--", AtRoot cachedBin, outPath⟩
  }⍟⊢ success ∧ ¬po.dry
  OnExit@
  •Exit ¬success
}
