⟨EEqual, ClearRefs, Variation, Squeeze, Type⟩ ← •internal
u ← ⌊100×(•UnixTime+1|100×•MonoTime)@

r ← •MakeRand •Show u

ntn ← "bit"‿"i8"‿"i16"‿"i32"‿"f64"
ntv ← "Ab"‿"Ai8"‿"Ai16"‿"Ai32"‿"Af64"
nmin ← 0∾  -2⋆¯1+8×2⋆↕3
nmax ← 1∾¯1+2⋆¯1+8×2⋆↕3
int ← 1‿1‿1‿1‿0

ctn ← "c8"‿"c16"‿"c32"
ctv ← "Ac8"‿"Ac16"‿"Ac32"
cmax ← 1114112⌊2⋆8×2⋆↕3

⟨specF64⟩ ← ⟨r⟩ •Import "utils.bqn"
spec ← specF64∾{⇐}∾"a⍉𝕨"∾<"foo"

# currently tests an array of 0s, with only one outstanding element
Do ← { 𝕊:
  val ← {r.Range∘≠◶𝕩𝕩} ⟨
    {𝕊: r.Range∘≠⊸⊑ spec}
    {𝕊: (¯5+r.Range 10) + 2⋆r.Range 64}
  ⟩
  base ← 0
  
  len ← 1+r.Range 2⋆3+r.Range 7
  pos ← r.Range len
  arr ← "Ah" Variation val⌾(pos⊸⊑) len⥊<base
  
  tOut‿tIn ← {
    1=•Type val?
      msk ← ((¬int) ∨ val≡⌊val) ∧ 1∾˜ (≥⟜nmin∧≤⟜nmax)val
      ⟨1↑msk/ntn ⋄ "Ah"‿"Af"∾msk/ntv⟩;
    (2≡•Type val) ∧ 1≡len?
      msk ← {2≡•Type val? (1≡len) ∧ (val-@)<cmax; 0}
      ⟨1↑msk/ctn ⋄ "Ah"‿"Af"∾msk/ctv⟩;
    # else
      ⟨"h"‿"fill" ⋄ "Ah"‿"Af"⟩
  }
  tIn ↩ {'S'∾1↓𝕩}¨⊸∾ ∾⟜"Inc"¨⊸∾ tIn
  
  { 
    arrv ← 𝕩 Variation arr
    sq ← Squeeze arrv
    sqt ← Type sq
    sqt↩{
         "arr"≡¯3↑sqt?  ¯3↓sqt;
      !"slice"≡¯5↑sqt ⋄ ¯5↓sqt
    }
    ¬ (arrv EEqual arr) ∧ (arrv EEqual sq) ∧ (⊑(<sqt) ∊ tOut)?
      •Out "Fail:"
      •Show arrv
      •Show sq
      •Show Type sq
      •Show tOut‿𝕩
      •Exit 1
    ;@
  }¨ tIn
  ClearRefs@
}

n0a ← "Af64" Variation 1↓1‿1‿¯1×π‿0‿0
n0b ← Squeeze n0a
{
  n0a≡○÷n0b? •Out "Squeeze behavior on negative zero: keep";
             •Out "Squeeze behavior on negative zero: discard"
}

Do¨ ↕100000
