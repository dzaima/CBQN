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
compile ← glyphs Import "c.bqn"
useInd ← "-i"≡⊑args ⋄ args↓˜↩useInd
Comp ← (3+useInd) ↑ ((<"inc(runtime["∾⍕∾"])"˙)¨↕62)⊸Compile
J ← ∾∾⟜\n¨
Fconst ← ≡◶⟨@⊸≤◶Num‿Char, Str, ⊑⟩
prov ← {"inc("∾𝕩∾")"}¨"bi_type"‿"bi_fill"‿"bi_log"‿"bi_grLen"‿"bi_grOrd"‿"bi_asrt"‿"bi_add"‿"bi_sub"‿"bi_mul"‿"bi_div"‿"bi_pow"‿"bi_floor"‿"bi_eq"‿"bi_le"‿"bi_fne"‿"bi_shape"‿"bi_pick"‿"bi_ud"‿"bi_tbl"‿"bi_scan"‿"bi_fillBy"‿"bi_val"‿"bi_catch"
Fout ← {((≠𝕩)↑⟨LI F¨,L ·prov⊸∾⍟(𝕨⊣0) Fconst¨,L (LI F¨)¨,L (L F¨)¨⟩) {𝕎𝕩}¨ 𝕩}
Frun ← 1⊸Fout
Long ← {¯2↓∾𝕩∾¨<","∾\n}

LFC ← Long∘Fout∘Comp
CArg ← {J (¯5⊸↓∾𝕩˙)⌾⊑ FLines "c.bqn"}

•Out⍟(¬return) (⊑"r"‿"c"‿"cc"‿"f"‿"e"‿"p"⊐⊏)◶⟨
  {𝕩⋄ref‿len←Import "pr.bqn"⋄Long Frun len⊸↓⌾(1⊸⊑)Comp ref}
  {𝕩⋄LFC CArg "⟨"∾"⟩"«∾","⊸∾¨'"'(⊣∾∾˜)¨glyphs}
  {𝕩⋄LFC "{"∾"}"∾˜CArg"𝕩"}
  {𝕩⋄LFC FChars "f.bqn"}
  {𝕩⋄LFC SVG "e"}
  {𝕩⋄LFC SVG "p"}
  ¯1 ↓ · J L∘Fout∘Comp¨
⟩ args
