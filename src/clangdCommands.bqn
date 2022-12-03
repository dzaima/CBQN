# list all .c & .h files
enableSingeli ← 0
additionalFlags ← ⟨⟩

allFiles ← ⟨⟩
{'d'=•file.Type𝕩? 𝕊¨ 𝕩⊸•file.At¨ •file.List𝕩; allFiles∾↩ <𝕩} ""
allFiles {⊑(<¯2↑𝕩)∊".c"‿".h"}¨⊸/↩

initialFlags ← ⟨
  "/usr/bin/clang","-std=gnu11","-Wall","-DCLANGD","-DFFI=2","-DCBQN_EXPORT","-fms-extensions"
  "-Wno-microsoft-anon-tag","-Wno-unused-function","-Wno-undefined-internal","-Wno-bitwise-instead-of-logical","-Wno-unknown-warning-option"
⟩ ∾ additionalFlags ∾ enableSingeli/⟨"-DSINGELI","-march=native"⟩

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

flagK←⟨⟩
flagV←⟨⟩
WantsFlags ← {
  𝕨 •file.At↩
  # •Out 𝕨∾" wants flags "∾JArr JStr¨ 𝕩
  flagK∾↩ <𝕨
  flagV∾↩ <𝕩
}
WantsIncludes ← {
  𝕨 WantsFlags ∾ ("-include"⋈•file.At)¨ 𝕩
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
  "ns.h", "vm.h", "builtins.h"
  "opt/mm_malloc.c", "opt/mm_buddy.c", "opt/mm_2buddy.c", "opt/mm_buddyTemplate.c", "opt/mm_buddyTemplate.h", "opt/gc.c", "opt/gc.h"
  "builtins/radix.h"
  "jit/nvm.h"
⟩
# and all the utils/ onces
{𝕩 WantsIncludes ⟨"core.h"⟩}¨ "utils/" FilterPrefix allFiles
# and some other things
"core/tyarrTemplate.h" WantsIncludes CoreTil "core/chrarr.h"
"core/tyarrTemplate.c" WantsIncludes ⟨"core/tyarr.c"⟩
"builtins/grade.h" WantsIncludes ⟨"builtins/sort.c"⟩
"singeli/c/arithdDispatch.c" WantsIncludes ⟨"builtins/arithd.c"⟩
"singeli/c/cmp.c" WantsIncludes ⟨"builtins/cmp.c"⟩
"utils/hashmapTemplate.h" WantsIncludes ⟨"utils/hash.h"⟩


# give the expected environment for configuration-specific files
"opt/mm_2buddy.c" WantsFlags ⟨"-DMM=2"⟩
"opt/mm_malloc.c" WantsFlags ⟨"-DMM=0"⟩
"utils/valgrind.h" WantsFlags ⟨"-DUSE_VALGRIND"⟩
"core/heap.h" WantsFlags ⟨"-DHEAP_VERIFY"⟩
{𝕩 WantsFlags ⟨"-DSINGELI"⟩}¨ "singeli/c/" FilterPrefix allFiles

# template files
"builtins/sortTemplate.h" WantsIncludes ⟨"core.h", "utils/talloc.h"⟩
"builtins/sortTemplate.h" WantsFlags ⟨"-DSORT_NAME=", "-DSORT_TYPE=B", "-DSORT_CMP=compare"⟩

# deduplicate flag files
gfk ← ⍷flagK
gfv ← ∾¨ (⊐flagK) ⊔ flagV

# resolve flags to actual files
allFlags ← (gfk⊐•file.At¨ allFiles)⊏gfv ∾ <⟨⟩

FileObj ← {
  "file"‿"directory"‿"arguments" JObj ⟨JStr 𝕨, JStr •path, ", " JArr JStr¨ initialFlags∾𝕩⟩
}

resPath ← ≠◶⟨"../compile_commands.json", •wdpath•file.At⊑⟩ •args
resPath •FChars (","∾n) JArr allFiles FileObj¨ allFlags
•Out "Wrote clangd compile commands to "∾•file.At resPath