⟨AtRoot⟩ ← •args
Esc←{
  in ← (@+0‿9‿10‿13)∾"'"""   # Null, Tab, LF, CR, and quotes
  out ← "0tnr"               # Whitespace characters changed to letters
  i ← in⊐𝕩
  𝕩 ↩ i ⊏⟜out⌾((i<≠out)⊸/) 𝕩 # Replace
  ∾(i<≠in) /⟜"\"⊸∾¨ 𝕩        # Insert \
}
lf←@+10
JStr ← {∾⟨"""",Esc 𝕩,""""⟩}
JArr ← {'['∾']'∾˜ (≠𝕨) ↓ ∾ 𝕨⊸∾¨ 𝕩}
JObj ← {∾⟨'{',lf,¯2↓∾(JStr¨𝕨) {∾⟨"  ",𝕨,": ",𝕩,',',lf⟩}¨ 𝕩, lf,'}'⟩}



precompFiles ← ⥊"bytecodeLocal"‿"bytecodeSubmodule" {∾⟨"../build/",𝕨,"/gen/",𝕩⟩}⌜ "compiles"‿"explain"‿"formatter"‿"runtime0"‿"runtime1"‿"runtime1x"
RecursiveFiles ⇐ {
  res ← ⟨⟩
  {'d'=•file.Type AtRoot 𝕩? 𝕊¨ 𝕩⊸•file.At¨ •file.List AtRoot 𝕩; res∾↩ <𝕩} 𝕩
  res
}

CFiles ⇐ {
  res ← RecursiveFiles 𝕩
  res {⊑(<¯2↑𝕩)∊".c"‿".h"}¨⊸/↩
  res ∾ 𝕩⊸•file.At¨ precompFiles
}

cObjects ← ⟨⟩
_addC ⇐ {cObjects∾↩ <⟨𝕩, (𝔽@)∾<𝕩⟩}

singeliState ← @
singeliObjects ← ⟨⟩
AddSingeli ⇐ {
  singeliObjects∾↩ <𝕨‿𝕩
}
SingeliInit ⇐ { 𝕊 po:
  singeliState ↩ ⟨po.singeliDir, 1↓∾ ','⊸∾¨ {𝕩.basearch/𝕩.feats} po.SingeliArchInfo 1⟩
}


ModifyCBQNFlags ← {
  filenames ← 4↓¨ ⊑¨ 𝕩
  flagK←⟨⟩
  flagV←⟨⟩
  WantsFlags ← {
    𝕨 •file.At↩ # ⋄ •Out 𝕨∾" wants flags "∾JArr JStr¨ 𝕩
    flagK∾↩ <𝕨
    flagV∾↩ <𝕩
  }
  WantsIncludes ← {
    # {•file.Exists "../../src" •file.At 𝕩?@;•Out "Unknown file: "∾𝕩} 𝕨
    𝕨 WantsFlags ∾ ("-include"⋈"src"•file.At⊢)¨ 𝕩
  }
  
  FilterPrefix ← {𝕨⊸{𝕨≡(≠𝕨)↑𝕩}¨⊸/ 𝕩}
  
  # main core.h sequence, assuming MM==1
  coreIncludes ← ⟨"core/h.h","core/stuff.h","opt/mm_buddy.h","core/gstack.h","core/harr.h","core/numarr.h","core/chrarr.h","core/fillarr.h","core/derv.h","core/arrFns.h"⟩
  {(¯1⊑𝕩) WantsIncludes ¯1↓𝕩}¨ 2↓↑ coreIncludes
  CoreTil ← {coreIncludes↑˜⊑coreIncludes⊐<𝕩}
  
  # MM==0 and MM==2
  coreTilMM ← CoreTil "opt/mm_buddy.h"
  {𝕩 WantsIncludes coreTilMM}¨ "opt/mm_2buddy.h"‿"opt/mm_malloc.h"
  
  # non-'utils/' headers & optional .c files
  {𝕩 WantsIncludes ⟨"core.h"⟩}¨ ⟨
    "core/ns.h", "vm/vm.h", "builtins.h", "vm/load.h"
    "opt/mm_malloc.c", "opt/mm_buddyTemplate.h", "opt/gc.c", "opt/gc.h"
    "builtins/radix.h", "singeli/c/arithdDispatch.h"
  ⟩
  # and all the utils/ onces
  {𝕩 WantsIncludes ⟨"core.h"⟩}¨ "utils/" FilterPrefix filenames
  # and some other things
  "core/tyarrTemplate.h" WantsIncludes CoreTil "core/chrarr.h"
  "core/tyarrTemplate.c" WantsIncludes ⟨"core/tyarr.c"⟩
  "builtins/grade.h" WantsIncludes ⟨"builtins/sort.c"⟩
  "singeli/c/arithdDispatch.c" WantsIncludes ⟨"builtins/arithd.c"⟩
  "singeli/c/cmp.c" WantsIncludes ⟨"builtins/cmp.c"⟩
  "utils/hashmapTemplate.h" WantsIncludes ⟨"utils/hash.h"⟩
  
  precompFiles WantsIncludes¨ <⟨"load.c"⟩
  precompFiles WantsFlags¨ <⟨"-xc"⟩
  
  # give the expected environment for configuration-specific files
  "opt/mm_buddyTemplate.c"‿"opt/mm_buddy.c"‿"opt/mm_2buddy.c" WantsIncludes¨ <⟨"core/mm.c"⟩
  "opt/mm_malloc.h"‿"opt/mm_malloc.c" WantsFlags¨ <⟨"-DMM=0"⟩
  "opt/mm_buddy.h" ‿"opt/mm_buddy.c"  WantsFlags¨ <⟨"-DMM=1"⟩
  "opt/mm_2buddy.h"‿"opt/mm_2buddy.c" WantsFlags¨ <⟨"-DMM=2"⟩
  "utils/valgrind.h" WantsFlags ⟨"-DUSE_VALGRIND"⟩
  "core/heap.h" WantsFlags ⟨"-DHEAP_VERIFY"⟩
  {𝕩 WantsFlags ⟨"-DSINGELI"⟩}¨ "singeli/c/" FilterPrefix filenames
  
  # template files
  "builtins/ffi/ffiExport.c" WantsIncludes ⟨"builtins/ffi.c"⟩
  "builtins/sortTemplate.h" WantsIncludes ⟨"core.h", "utils/talloc.h"⟩
  "builtins/sortTemplate.h" WantsFlags ⟨"-DSORT_NAME=", "-DSORT_TYPE=B", "-DSORT_CMP=compare"⟩
  "opt/mm_buddyTemplate.c"‿"opt/mm_buddyTemplate.h" WantsFlags¨ <⟨"-DBN(X)=mm_##X", "-DMMI(X)=X", "-DBSZ(X)=(1ull<<(X))", "-DALSZ=20", "-DMM=1"⟩
  
  # deduplicate flag files
  gfk ← ⍷flagK
  gfv ← ∾¨ (⊐flagK) ⊔ flagV
  
  # resolve flags to actual files
  𝕩 {n‿f 𝕊 f2: ⟨n, f∾⟨"-Wno-undefined-internal", "-DCLANGD"⟩∾f2⟩}¨ (gfk⊐•file.At¨ filenames)⊏gfv ∾ <⟨⟩
}

Finish ⇐ { 𝕊 write:
  _doWrite ← {
    𝕩 •FChars⍟write 𝕗
    •Out ∾⟨{write? "Wrote "; "Would write "}, 𝕨, " to ", •file.At 𝕩⟩
  }
  TLArr ← (","∾lf)⊸JArr
  
  {
    cObjects ModifyCBQNFlags⌾(({"src/"≡4↑⊑𝕩}¨ cObjects)⊸/)↩
    FileObj ← { name‿flags:
      "file"‿"directory"‿"arguments" JObj ⟨JStr name, JStr AtRoot "", ", " JArr JStr¨ flags⟩
    }
    res ← TLArr FileObj¨ cObjects
    {
      •Out {write? "Removing"; "Would remove"} ∾ " old "∾𝕩
      •file.Remove⍟write 𝕩
    }⍟•file.Exists AtRoot "compile_commands.json"
    
    
    "clangd compile commands" res _doWrite AtRoot "build/compile_commands.json"
  }
  
  AtRootRel ← "../"⊸•file.At
  
  {
    singeliState≡@? @;
    dir‿arch ← singeliState
    cmd0 ← ⟨"-a", arch, "-l", "="∾dir •file.At "include"⟩
    res ← TLArr { cmd‿name:
      "file"‿"arguments" JObj ⟨JStr AtRootRel name, ", " JArr JStr¨ cmd0 ∾ cmd⟩
    }¨ singeliObjects
    "Singeli compile commands" res _doWrite AtRoot "build/.singeli_compile_commands.json"
  }
}