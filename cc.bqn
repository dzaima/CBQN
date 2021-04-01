#! /usr/bin/env dbqn
args←•args
return ← 1≡⊑args
args↓˜↩ return
"call with argument specifying path of mlochbaum/BQN!"!2≤≠args
path←(⊑args)∾"/src/"
args↓˜↩1
L  ← {"m_caB("  ∾(⍕≠𝕩)∾",(B[]){"  ∾(1↓∾","⊸∾¨𝕩)∾"})"}
LI ← {"m_cai32("∾(⍕≠𝕩)∾",(i32[]){"∾(1↓∾","⊸∾¨𝕩)∾"})"}
# Escape the special characters that appear in BQN sources.
Esc←{
  in ← (@+0‿9‿10‿13)∾"'"""    # Null, Tab, LF, CR, and quotes
  out ← "0tnr"                # Whitespace characters changed to letters
  i ← in⊐𝕩
  𝕩 ↩ i ⊏⟜out⌾((i<≠out)⊸/) 𝕩  # Replace
  ∾(i<≠in) /⟜"\"⊸∾¨ 𝕩         # Insert \
}
Str ← "m_str32(U"""∾Esc∾""")"˜     # A BQN string
Char ← {"m_c32(U'"∾(Esc⥊𝕩)∾"')"}          # A BQN character
Num ← {s←"-"/˜𝕩<0 ⋄ ∞⊸=∘|◶⟨"m_f64("∾")"∾˜s∾⍕∘| ⋄ "m_f64("∾s∾"1.0/0.0)"⟩𝕩} # Format number

F ← ⍕  # Format number

Import ← •Import path⊸∾
FChars ← •FChars path⊸∾

compile ← Import "c.bqn"
useInd ← "-i"≡⊑args ⋄ args↓˜↩useInd
Comp ← (3+useInd) ↑ ((<"inci(runtime["∾⍕∾"])"˙)¨↕62)⊸Compile
J ← ∾∾⟜\n¨
Fconst ← ≡◶⟨@⊸≤◶Num‿Char, Str, ⊑⟩
prov ← {"inci("∾𝕩∾")"}¨"bi_type"‿"bi_fill"‿"bi_log"‿"bi_grLen"‿"bi_grOrd"‿"bi_asrt"‿"bi_add"‿"bi_sub"‿"bi_mul"‿"bi_div"‿"bi_pow"‿"bi_floor"‿"bi_eq"‿"bi_le"‿"bi_fne"‿"bi_shape"‿"bi_pick"‿"bi_ud"‿"bi_tbl"‿"bi_scan"‿"bi_fillBy"‿"bi_val"
Fout ← {((≠𝕩)↑⟨LI F¨,L ·prov⊸∾⍟(𝕨⊣0) Fconst¨,L (LI F¨)¨,L (L F¨)¨⟩) {𝕎𝕩}¨ 𝕩}
Frun ← 1⊸Fout
Long ← {¯2↓∾𝕩∾¨<","∾\n}
•Out⍟(¬return) (⊑"r"‿"c"‿"f"‿"e"⊐⊏)◶⟨
  {𝕩⋄ref‿len←Import "pr.bqn"⋄Long Frun len⊸↓⌾(1⊸⊑)Comp ref}
  {𝕩⋄Long Fout Comp FChars "c.bqn"}
  {𝕩⋄Long Fout Comp FChars "f.bqn"}
  {𝕩⋄Long Fout Comp ⟨"Modify←GetHighlights←⊢⋄"⟩∾∾ (FChars ⊢)∘∾⟜".bqn"¨ "../svg"‿"e"}
  ¯1 ↓ · J L∘Fout∘Comp¨
⟩ args
