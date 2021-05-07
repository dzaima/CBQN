#! /usr/bin/env dbqn
# Modified version of https://github.com/mlochbaum/BQN/blob/master/src/cjs.bqn, which is under the ISC license (https://github.com/mlochbaum/BQN/blob/master/LICENSE)

args←•args
"Usage: ./cc.bqn path/to/mlochbaum/BQN <one of [rcfe] or an expression>"!2≤≠args
return ← 1≡⊑args
args↓˜↩ return
path←(⊑args)∾"/src/"
args↓˜↩1
L  ← {"m_caB("  ∾(⍕≠𝕩)∾",(B[]){"  ∾(1↓∾","⊸∾¨𝕩)∾"})"}
LI ← {"m_cai32("∾(⍕≠𝕩)∾",(i32[]){"∾(1↓∾","⊸∾¨𝕩)∾"})"}
# Escape the special characters that appear in BQN sources.
Esc←{
  in ← (@+0‿9‿10‿13)∾"'"""   # Null, Tab, LF, CR, and quotes
  out ← "0tnr"               # Whitespace characters changed to letters
  i ← in⊐𝕩
  𝕩 ↩ i ⊏⟜out⌾((i<≠out)⊸/) 𝕩 # Replace
  ∾(i<≠in) /⟜"\"⊸∾¨ 𝕩        # Insert \
}
Str ← "m_str32(U"""∾Esc∾""")"˙    # A BQN string
Char ← {"m_c32(U'"∾(Esc⥊𝕩)∾"')"} # A BQN character
Num ← {s←"-"/˜𝕩<0 ⋄ ∞⊸=∘|◶⟨"m_f64("∾")"∾˜s∾⍕∘| ⋄ "m_f64("∾s∾"1.0/0.0)"⟩𝕩} # Format number

F ← ⍕  # Format number

Import ← {𝕨•Import path∾𝕩}
FChars ← {𝕨•FChars path∾𝕩}
FLines ← {𝕨•FLines path∾𝕩}

glyphs ← Import "glyphs.bqn"
_getComp ← { (3+useInd) ↑ (𝕗 Import "c.bqn"){𝔽} }
useInd ← "-i"≡⊑args ⋄ args↓˜↩useInd
Comp ← ((<"inc(runtime["∾⍕∾"])"˙)¨↕62) glyphs _getComp ⊢
J ← ∾∾⟜\n¨
Fconst ← ≡◶⟨@⊸≤◶Num‿Char, Str, ⊑⟩
Fout ← {((≠𝕩)↑⟨LI F¨,L Fconst¨,L (LI ·F¨ 4↑⊢)¨,L (L F¨)¨⟩) {𝕎𝕩}¨ 𝕩}
Frun ← 1⊸Fout
Long ← {¯2↓∾𝕩∾¨<","∾\n}

RT ← {
  src‿need‿inputs←𝕩Import"pr.bqn"
  pr←"runtime_0"‿"provide"{(∾𝕨<⊸(<∘{∾"inc("‿𝕨‿"["‿𝕩‿"])"}⟜F¨)⟜(↕≠)¨𝕩)⊏˜(∾𝕩)⊐∾need}○((-1+1=𝕩)⊸↑)inputs
  Long Fout pr need _getComp src
}
CArg ← {J (¯5⊸↓∾𝕩˙)⌾⊑ FLines "c.bqn"}
LFC ← Long∘Fout∘Comp

•Out⍟(¬return) (⊑"r"‿"r0"‿"r1"‿"c"‿"cc"‿"f"‿"e"‿"p"⊐⊏)◶⟨
  RT∘2, RT∘0, RT∘1
  {𝕩⋄LFC CArg "⟨"∾"⟩"«∾","⊸∾¨'"'(⊣∾∾˜)¨glyphs}
  {𝕩⋄LFC "{"∾"}"∾˜CArg"𝕩"}
  {𝕩⋄LFC FChars "f.bqn"}
  {𝕩⋄LFC SVG "e"}
  {𝕩⋄LFC SVG "p"}
  ¯1 ↓ · J L∘Fout∘Comp¨
⟩ args