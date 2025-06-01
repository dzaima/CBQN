### WHOLE-FILE-TEST
# tests if squeeze produces the optimal result
helpers ← ⊑•args
r ← helpers.GetRand@
⟨EEqual, ClearRefs, Variation, Unshare, Squeeze, Type⟩ ← •internal

ntn ← "bit"‿"i8"‿"i16"‿"i32"‿"f64"
ntv ← "Ab"‿"Ai8"‿"Ai16"‿"Ai32"‿"Af64"
nmin ← 0∾  -2⋆¯1+8×2⋆↕3
nmax ← 1∾¯1+2⋆¯1+8×2⋆↕3
int ← 1‿1‿1‿1‿0

ctn ← "c8"‿"c16"‿"c32"
ctv ← "Ac8"‿"Ac16"‿"Ac32"
cmax ← 1114112⌊2⋆8×2⋆↕3

⟨specF64⟩ ← ⟨r⟩ helpers.Import "utils.bqn"
spec ← specF64∾{⇐}∾"a⍉𝕨"∾(@+1114111-↕5)∾<"foo"

RandEl ← r.Range∘3◶⟨
  {𝕊: r.Range∘≠⊸⊑ spec} # some special values
  {𝕊: -⍟(r.Range 2) (¯5+r.Range 10) + 2⋆r.Range 64} # random near-power-of-two number
  {𝕊: @+0⌈1114111⌊(¯5+r.Range 10) + 2⋆2+r.Range 20} # random near-power-of-two character
⟩

Do ← { 𝕊:
  val ← RandEl@
  base ← RandEl@
  
  len ← 1+r.Range 2⋆3+r.Range 7
  pos ← r.Range len
  arr ← "Ah" Variation val⌾(pos⊸⊑) len⥊<base
  
  vals ← ∾⟜⟨base⟩⍟(len>1) ⟨val⟩
  expg ← (∧´⊑=⊢)◶3‿⊑ {
    1=𝕩? 1;
    2=𝕩? 2;
    3
  }∘•type¨ vals
  
  tOut‿tIn ← {
    expg=1?
      msk ← ∧´ {((¬int) ∨ 𝕩≡⌊𝕩) ∧ 1∾˜ (≥⟜nmin∧≤⟜nmax)𝕩}¨ vals
      ⟨1↑msk/ntn ⋄ "Ah"‿"Af"∾msk/ntv⟩;
    expg=2?
      msk ← ∧´ {cmax>𝕩-@}¨ vals
      ⟨1↑msk/ctn ⋄ "Ah"‿"Af"∾msk/ctv⟩;
    expg=3?
      ⟨"h"‿"fill" ⋄ "Ah"‿"Af"⟩
  }
  tIn ↩ {'S'∾1↓𝕩}¨⊸∾ ∾⟜"Inc"¨⊸∾ tIn
  
  { 
    arrv ← 𝕩 Variation arr
    arrv0 ← Unshare arrv
    sq ← Squeeze arrv
    sqt ← Type sq
    sqt↩{
         "arr"≡¯3↑sqt?  ¯3↓sqt;
      !"slice"≡¯5↑sqt ⋄ ¯5↓sqt
    }
    ¬ (arrv0 EEqual arr) ∧ (arrv0 EEqual sq) ∧ (⊑(<sqt) ∊ tOut)?
      •Out "Fail:"
      •Out "Array: "∾•Repr arr
      •Out "Squeezed: "∾•Repr sq
      •Out "Testing variation "∾•Repr 𝕩
      •Out "Got: "∾(Type sq)∾" / "∾sqt
      •Out "Expected one of: "∾•Repr tOut
      !0
    ;@
  }¨ tIn
  ClearRefs@
}

Do¨ ↕100000
