%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF eqvar %USE tvar ⋄ _eqvar ← {r ← 𝕨 𝔽 _tvar 𝕩 ⋄ !∘≡⟜(⊑r)¨ r ⋄ ⊑r}

(
  %USE eqvar
  LV ↩ ""⊸LV
  Test ← { 𝕊:
    x ← (2•rand.Range 50)•rand.Range 100
    w ← {0: ↕0; (1+•rand.Range 50) •rand.Range 𝕩} 1⊑≢x
    w {𝕨⊸⊏˘ 𝕩}_eqvar x
  }
  
  Test¨ ↕1000
)

(
  Test ← {𝕊 gen:
    { 𝕊 cam:
      {
        i ← •rand.Range∘3◶⟨⊢, ¯1⊸-, {𝕩 - (1+2×𝕩) × 2•rand.Range˜≠𝕩}⟩ 𝕨 •rand.Range 𝕩
        ! (i⊸⊏˘ ≡ i⊸⊏○•internal.Keep˘) Gen cam‿𝕩
      }⌜˜ 1↓⍷(↕20)∾⌊(√2)⋆↕16
    }¨ 1‿6‿100
  }
  
  Test {{0 + 𝕩 •rand.Range 100}}
  Test {{0 + 𝕩 •rand.Range 1000}}
  Test {{0 + 𝕩 •rand.Range 1e9}}
  Test {{0 + 𝕩 •rand.Range 0}}
  
  Test {{@ + 𝕩 •rand.Range 100}}
  Test {{@ + 𝕩 •rand.Range 1000}}
  Test {{@ + 𝕩 •rand.Range 1114111}}
  Test {{⊑⟜"foo"‿@¨ 𝕩 •rand.Range 2}}
)
