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


(
  %USE var
  F ← {𝕨⊸⊏˘ 𝕩}
  {
    𝕩+↩ 1
    is ← 𝕨 •rand.Range 𝕩
    n ← •rand.Range 200
    d ← n‿𝕩•rand.Range 2
    is‿n‿𝕩 ! 1=≠⍷ ⟨is F d, is F "Ai8"V d⟩ ∾ {𝕩=2? ⟨("Ai8"V is) F d⟩; ⟨⟩}𝕩
  }⌜˜ ↕10
)

(
  %USE var
  F ← {𝕨⊸⊏˘ 𝕩}⎊'e'
  {
    𝕩+↩ 1
    𝕨+↩ 1
    is ← 𝕨⥊𝕩
    n ← 1+•rand.Range 200
    d ← n‿𝕩•rand.Range 2
    
    𝕨‿𝕩‿n! ∧´ 'e'⊸≡¨ ⟨is F d, is F "Ai8"V d⟩ ∾ {𝕩=2? ⟨("Ai8"V is) F d⟩; ⟨⟩}𝕩
  }⌜˜ ↕10
)

(
  %USE var
  F ← {𝕨⊸⊏˘ 𝕩}⎊'e'
  
  {𝕊 inds:
    csz ← ≠⊑inds
    inds {
      ⟨csz, 𝕨, ≢𝕩⟩ ! 1=≠⍷ ⥊ (V⟜𝕨¨ (∧´𝕨≤1)◶⟨⋈, "Ab"⊸⋈⟩ "Ai8") F⌜ V⟜𝕩¨ "Ab"‿"Ai8"‿"Ai32"
    }⌜ {𝕩‿csz •rand.Range 2}¨ (↕8)∾⥊0‿1+⌜3↓2⋆↕9
  }¨ ⟨⥊↕2‿2⟩ ∾ {⟨↕𝕩, •rand.Range˜𝕩⟩}¨ 1‿3‿4‿5‿8‿9‿16
)