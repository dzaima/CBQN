%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF eqvar ⟨_eqvar⟩←⊑•args
%DEF eqvarv ⟨_eqvarv_⟩←⊑•args
%DEF rand r ← (⊑•args).GetRand@

(
  %USE rand ⋄ %USE eqvarv
  Test ← { 𝕊:
    x ← (2 r.Range 50) r.Range 100
    w ← {0: ↕0; (1+r.Range 50) r.Range 𝕩} 1⊑≢x
    w {𝕨⊸⊏˘ 𝕩}_eqvarv_"" x
  }
  
  Test¨ ↕1000
)

(
  %USE rand
  Test ← {𝕊 gen:
    { 𝕊 cam:
      {
        i ← r.Range∘3◶⟨⊢, ¯1⊸-, {𝕩 - (1+2×𝕩) × 2 r.Range˜≠𝕩}⟩ 𝕨 r.Range 𝕩
        ! (i⊸⊏˘ ≡ i⊸⊏○•internal.Keep˘) Gen cam‿𝕩
      }⌜˜ 1↓⍷(↕20)∾⌊(√2)⋆↕16
    }¨ 1‿6‿100
  }
  
  Test {{0 + 𝕩 r.Range 100}}
  Test {{0 + 𝕩 r.Range 1000}}
  Test {{0 + 𝕩 r.Range 1e9}}
  Test {{0 + 𝕩 r.Range 0}}
  
  Test {{@ + 𝕩 r.Range 100}}
  Test {{@ + 𝕩 r.Range 1000}}
  Test {{@ + 𝕩 r.Range 1114111}}
  Test {{⊑⟜"foo"‿@¨ 𝕩 r.Range 2}}
)


(
  %USE rand ⋄ %USE var
  F ← {𝕨⊸⊏˘ 𝕩}
  {
    𝕩+↩ 1
    is ← 𝕨 r.Range 𝕩
    n ← r.Range 200
    d ← n‿𝕩 r.Range 2
    is‿n‿𝕩 ! 1=≠⍷ ⟨is F d, is F "Ai8"V d⟩ ∾ {𝕩=2? ⟨("Ai8"V is) F d⟩; ⟨⟩}𝕩
  }⌜˜ ↕10
)

(
  %USE rand ⋄ %USE var
  F ← {𝕨⊸⊏˘ 𝕩}⎊'e'
  {
    𝕩+↩ 1
    𝕨+↩ 1
    is ← 𝕨⥊𝕩
    n ← 1+ r.Range 200
    d ← n‿𝕩 r.Range 2
    
    𝕨‿𝕩‿n! ∧´ 'e'⊸≡¨ ⟨is F d, is F "Ai8"V d⟩ ∾ {𝕩=2? ⟨("Ai8"V is) F d⟩; ⟨⟩}𝕩
  }⌜˜ ↕10
)

(
  %USE rand ⋄ %USE var
  F ← {𝕨⊸⊏˘ 𝕩}⎊'e'
  
  {𝕊 inds:
    csz ← ≠⊑inds
    inds {
      ⟨csz, 𝕨, ≢𝕩⟩ ! 1=≠⍷ ⥊ (V⟜𝕨¨ (∧´𝕨≤1)◶⟨⋈, "Ab"⊸⋈⟩ "Ai8") F⌜ V⟜𝕩¨ "Ab"‿"Ai8"‿"Ai32"
    }⌜ {𝕩‿csz r.Range 2}¨ (↕8)∾⥊0‿1+⌜3↓2⋆↕9
  }¨ ⟨⥊↕2‿2⟩ ∾ {⟨↕𝕩, r.Range˜𝕩⟩}¨ 1‿3‿4‿5‿8‿9‿16
)