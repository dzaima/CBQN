⟨AtRoot⟩ ← •args
Esc←{
  in ← (@+0‿9‿10‿13)∾"'"""   # Null, Tab, LF, CR, and quotes
  out ← "0tnr"               # Whitespace characters changed to letters
  i ← in⊐𝕩
  𝕩 ↩ i ⊏⟜out⌾((i<≠out)⊸/) 𝕩 # Replace
  ∾(i<≠in) /⟜"\"⊸∾¨ 𝕩        # Insert \
}
n←@+10
JStr ← {∾⟨"""",Esc 𝕩,""""⟩}
JArr ← {'['∾']'∾˜2↓∾𝕨⊸∾¨ 𝕩}
JObj ← {∾⟨'{',n,¯2↓∾(JStr¨𝕨) {∾⟨"  ",𝕨,": ",𝕩,',',n⟩}¨ 𝕩, n,'}'⟩}



precompFiles ← ⥊"bytecodeLocal"‿"bytecodeSubmodule" {∾⟨"../build/",𝕨,"/gen/",𝕩⟩}⌜ "compiles"‿"explain"‿"formatter"‿"runtime0"‿"runtime1"‿"runtime1x"
Files ⇐ {
  res ← ⟨⟩
  {'d'=•file.Type AtRoot 𝕩? 𝕊¨ 𝕩⊸•file.At¨ •file.List AtRoot 𝕩; res∾↩ <𝕩} 𝕩
  {⊑(<¯2↑𝕩)∊".c"‿".h"}¨⊸/ res
  res ∾ 𝕩⊸•file.At¨ precompFiles
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
    𝕨 WantsFlags ∾ ("-include"⋈"src"•file.At⊢)¨ 𝕩
  }
  
  FilterPrefix ← {𝕨⊸{𝕨≡(≠𝕨)↑𝕩}¨⊸/ 𝕩}
  
  # main core.h sequence, assuming MM==1
  coreIncludes ← ⟨"h.h","core/stuff.h","core/heap.h","opt/mm_buddy.h","core/gstack.h","core/harr.h","core/numarr.h","core/chrarr.h","core/fillarr.h","core/derv.h","core/arrFns.h"⟩
  {(¯1⊑𝕩) WantsIncludes ¯1↓𝕩}¨ 2↓↑ coreIncludes
  CoreTil ← {coreIncludes↑˜⊑coreIncludes⊐<𝕩}
  
  # MM==0 and MM==2
  coreTilMM ← CoreTil "opt/mm_buddy.h"
  {𝕩 WantsIncludes coreTilMM}¨ "opt/mm_2buddy.h"‿"opt/mm_malloc.h"
  
  # non-'utils/' headers & optional .c files
  {𝕩 WantsIncludes ⟨"core.h"⟩}¨ ⟨
    "ns.h", "vm.h", "builtins.h", "load.h"
    "opt/mm_malloc.c", "opt/mm_buddyTemplate.h", "opt/gc.c", "opt/gc.h"
    "builtins/radix.h", "singeli/c/arithdDispatch.h"
    "jit/nvm.h"
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
  "builtins/sortTemplate.h" WantsIncludes ⟨"core.h", "utils/talloc.h"⟩
  "builtins/sortTemplate.h" WantsFlags ⟨"-DSORT_NAME=", "-DSORT_TYPE=B", "-DSORT_CMP=compare"⟩
  "opt/mm_buddyTemplate.c"‿"opt/mm_buddyTemplate.h" WantsFlags¨ <⟨"-DBN(X)=mm_##X", "-DMMI(X)=X", "-DBSZ(X)=(1ull<<(X))", "-DALSZ=20", "-DMM=1"⟩
  
  # deduplicate flag files
  gfk ← ⍷flagK
  gfv ← ∾¨ (⊐flagK) ⊔ flagV
  
  # resolve flags to actual files
  𝕩 {n‿f 𝕊 f2: ⟨n, f∾⟨"-Wno-undefined-internal", "-DCLANGD"⟩∾f2⟩}¨ (gfk⊐•file.At¨ filenames)⊏gfv ∾ <⟨⟩
}

objects ← ⟨⟩
Finish ⇐ { 𝕊 write:
  objects ModifyCBQNFlags⌾(({"src/"≡4↑⊑𝕩}¨ objects)⊸/)↩
  FileObj ← { name‿flags:
    "file"‿"directory"‿"arguments" JObj ⟨JStr name, JStr AtRoot "", ", " JArr JStr¨ flags⟩
  }
  res ← (","∾n) JArr FileObj¨ objects
  {
    •Out {write? "Removing"; "Would remove"} ∾ " old "∾𝕩
    •file.Remove⍟write 𝕩
  }⍟•file.Exists AtRoot "compile_commands.json"
  resPath ← AtRoot "build/compile_commands.json"
  {
    write?
      resPath •FChars res
      •Out "Wrote clangd compile commands to "∾•file.At resPath
    ;
      •Out "Would write clangd compile commands to "∾•file.At resPath
  }
}
_add ⇐ {objects∾↩ <⟨𝕩, (𝔽@)∾<𝕩⟩}