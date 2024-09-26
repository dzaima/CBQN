#!/usr/bin/env bqn
# Modified version of https://github.com/mlochbaum/BQN/blob/master/src/cjs.bqn

args ← •args
"Usage: ./cc.bqn path/to/mlochbaum/BQN [-i] [-n name] <(r|r0|r1|r1x|c|cc|f|e|p) or an expression>"!2≤≠args
NextArg ← {𝕊: (args↓˜↩1) ⊢ ⊑args}
TryArg ← {𝕩≡⊑args? 1 ⊣ NextArg@; 0}

return ← TryArg 1
bqnpath ← (NextArg@) •file.At "src"
srcmap ← TryArg "-i"
name ← TryArg◶"(precompiled)"‿NextArg "-n"

Import ← {𝕨 •Import bqnpath •file.At 𝕩}
FChars ← {𝕨 •FChars bqnpath •file.At 𝕩}
FLines ← {𝕨 •FLines bqnpath •file.At 𝕩}

F ← •Repr # Format number
Join ← {(-≠𝕨)↓ ∾∾⟜𝕨¨ 𝕩}

FmtComp ← {
  # Escape the special characters that appear in BQN sources.
  Esc ← {
    in ← (@+0‿9‿10‿13)∾"'"""   # Null, Tab, LF, CR, and quotes
    out ← "0tnr"               # Whitespace characters changed to letters
    i ← in⊐𝕩
    𝕩 ↩ i ⊏⟜out⌾((i<≠out)⊸/) 𝕩 # Replace
    ∾(i<≠in) /⟜"\"⊸∾¨ 𝕩        # Insert \
  }
  
  U32Len ← (≠ - ·+´ ≥⟜55296 ∧ ≤⟜56319)-⟜@ # for dzaima/BQN; shouldn't break things in proper BQN impls, as surrogate values don't map to valid codepoints
  
  CStr ← {"""" (∾∾⊣) Esc 𝕩}
  
  List ← (0<≠)◶⟨"",1↓·∾","⊸∾¨⟩
  
  lf ← ⥊@+10
  Cache ← {
    tag ← 𝕩
    values ← ⟨⟩
    Add ⇐ {(values∾↩ <𝕩) ⊢ tag⋈≠values}
    Done ⇐ { 𝕊:
      unq ← ⊐values
      uncomp ⇐ {(⍋≠¨ 𝕩/values) ⊏ /𝕩} ∊unq # compressed index → canonical original index; sort by length to make length predictable
      comp ⇐ (uncomp⊏unq)⊐unq             # original index → compressed index
      counts ⇐ /⁼comp                     # compressed index → count of such
      compvals ⇐ uncomp⊏values            # compressed index → data
      n ⇐ ≠uncomp
    }
  }
  
  intarrCache ← Cache 1
  Li ← intarrCache.Add
  _Lo ← {{∾⟨"m_lvB_", F≠𝕩, "(", List 𝕩, ")"⟩} 𝔽¨𝕩}
  OStr2C ← { # A BQN string
    a←∧´𝕩<128+@
    ∾⟨a⊑"m_c32vec"‿"m_c8vec","(",a¬⊸⥊"U","",CStr𝕩,",",•Repr U32Len 𝕩,")"⟩
  }
  OChr2C ← {"m_c32(U'"∾(Esc⥊𝕩)∾"')"} # A BQN character
  ONum2C ← {s←"-"/˜𝕩<0 ⋄ ∞⊸=∘|◶⟨"m_f64("∾")"∾˜s∾F∘| ⋄ "m_f64("∾s∾"1.0/0.0)"⟩𝕩} # Format number
  OAny2C ← ≡◶⟨@⊸≤◶ONum2C‿OChr2C, OStr2C, ⊑⟩
  
  TmpNum ← 0⊸⋈
  # 0‿123: number
  # 1‿n: int arr ref
  # ⟨a,b,c⟩: obj arr
  # "foo": literal code
  a ← ((≠𝕩)↑⟨Li ⋄ OAny2C¨ ⋄ {0=⌈´=¨𝕩? Li 𝕩; =◶⟨TmpNum,Li¨⟩¨ 𝕩}¨ ⋄ (Li 2⊸↑)¨ ⋄ Li¨ ⋄ OStr2C⟩) {𝕎𝕩}¨ 𝕩
  
  iarrs ← intarrCache.Done@ ⋄ Li↩!
  
  hasEmpty ← ⟨⟩ ≡ ⊑iarrs.compvals
  
  init ← ⟨⟩
  a {
    1‿i: n ← i⊑iarrs.comp ⋄ (hasEmpty∧0=n) ⊑ ⟨∾⟨"iarrs[",F n,"]"⟩, "iarrs0"⟩; # special-case first array (i.e. empty array) as there isn't enough aliasing info for the compiler to deduplicate loads
    2=•Type⊑𝕩? 𝕩;
    0‿n: ONum2C n;
    ⟨0‿a, 0‿b, ⟨c, d⟩⟩: cs≡⌊cs←a‿b? ∧´cs≥0? ∧´cs<4? ∾⟨"m_blockinfo(", F a+b×4, ", ", 𝕊 c, ", ", 𝕊 d, ")"⟩;
    4≥≠𝕩? 𝕊_Lo 𝕩;
    n ← "a" ∾ F ≠init
    np ← n∾"p"
    init∾↩< ∾⟨lf
      "B ", n, "; B* ", np, " = m_lvBn(&", n, ", ", F ≠𝕩, ");", lf
      "assume_separate_storage(", np, ", ", "iarrs", ");", lf # allow the compiler to vectorize copying from iarrs to np if so beneficial
    ⟩ ∾ (⍋{⊑'('∊𝕩}¨)⊸⊏ {∾⟨np,"[", F 𝕩, "] = ", 𝕨, ";", lf⟩}¨⟜↕⟜≠ 𝕊¨ 𝕩
    n
  }¨↩
  
  ∾⟨
    "B iarrs[", F iarrs.n, "];", lf
    "static const i32 iarrs_data[] = {", lf
      ∾ ∾⟜lf¨ {∾⟨∾∾⟜','¨ F¨ 𝕨, " // ", F 𝕩⟩}¨⟜↕⟜≠ iarrs.compvals
    "};", lf
    "static const u32 iarrs_lens[] = {", ','Join F∘≠¨ iarrs.compvals, "};", lf
    "init_intarrs(iarrs, iarrs_data, iarrs_lens, ", F iarrs.n, ");", lf
    hasEmpty / "B iarrs0 = iarrs[0];", lf
    ∾{0𝕊𝕩: ""; ∾⟨"incByG(iarrs[", F 𝕩, "], ", F 𝕨, ");", lf⟩}¨⟜↕⟜≠ iarrs.counts-1
    ∾init
    "load_importBlock", srcmap/"_src", "(", CStr name, ",", lf
      (','∾lf) Join "  "⊸∾¨ a
      lf
    ");"
  ⟩
}

primN ← ⟨
  "add",      "sub",     "mul",    "div",   "pow",     "root",      "floor",  "ceil",  "stile",   "not"
  "and",      "or",      "lt",     "gt",    "ne",      "eq",        "le",     "ge",    "feq",     "fne"
  "ltack",    "rtack",   "shape",  "join",  "couple",  "pair",      "take",   "drop",  "ud",      "shifta"
  "shiftb",   "reverse", "transp", "slash", "gradeUp", "gradeDown", "select", "pick",  "indexOf", "count"
  "memberOf", "find",    "group",  "asrt",  "const",   "swap",      "cell",   "each",  "tbl",     "undo"
  "fold",     "insert",  "scan",   "atop",  "over",    "before",    "after",  "under", "val",     "cond"
  "rank",     "depth",   "repeat", "catch"
⟩
primC ← "+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍⋈↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"
pre ← "+-=×↕≢≤⊑⊘⌊⌜⥊!<>`|¨´÷˙˜↑↓∘∾≠≥⊢⊣⊸⋆⋈⌈⍟⎊○◶⟜"
isFull ← ¬ primC ∊ "⁼⊏/⊔⌾⍷⚇˝"
fullC ← isFull/primC
extraC ← pre (¬∘∊˜/⊢) fullC
# •Out List "bi_"⊸∾¨ (primC∊extraC) / primN

glyphs ← Import "glyphs.bqn"
_getComp ← { (4+2×srcmap)↑ <∘⊢∾˜ 5↑ (𝕗 Import "c.bqn"){𝔽} }
Comp ← ((<"incG(runtime["∾F∾"])"˙)¨↕≠∾glyphs) glyphs _getComp ⊢
RT ← {
  ext ← 2=≠𝕩
  𝕩 ↩ ⊑𝕩
  src‿need‿inputs←((ext⊑𝕩‿1) ∾ ext/⟨fullC, extraC⟩) Import "pr.bqn"
  pr←"runtime_0"‿"provide"{(∾𝕨<⊸(<∘{∾"incG("‿𝕨‿"["‿𝕩‿"])"}⟜F¨)⟜(↕≠)¨𝕩)⊏˜(∾𝕩)⊐∾need}○((-1+1=𝕩)⊸↑)inputs
  FmtComp pr need _getComp src
}
CArg ← {(@+10) Join (¯5⊸↓∾𝕩˙)⌾⊑ FLines "c.bqn"}
SVG ← {∾⟨"Modify←GetHighlights←⊢⋄"⟩∾ FChars∘∾⟜".bqn"¨ "../svg"‿𝕩}

LFC ← FmtComp∘Comp

(@+10)∾˜ •Out⍟(¬return) (⊑"r"‿"r0"‿"r1"‿"r1x"‿"c"‿"cc"‿"f"‿"e"‿"eu"‿"p"⊐⊏)◶⟨
  RT∘2, RT∘0, RT∘1, RT∘1‿'x'
  {𝕩⋄LFC CArg "⟨"∾"⟩"«∾","⊸∾¨'"'(⊣∾∾˜)¨glyphs}
  {𝕩⋄LFC "{"∾"}"∾˜CArg"𝕩"}
  {𝕩⋄LFC FChars "f.bqn"}
  {𝕩⋄LFC SVG "e"}
  {𝕩⋄LFC FChars "eu.bqn"}
  {𝕩⋄LFC SVG "p"}
  ∾LFC¨
⟩ args