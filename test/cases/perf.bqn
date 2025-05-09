%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar2 %USE var ⋄ _tvar2_ ← {F _𝕣_ 𝕘 x: (CLR@) ⊢ {F 𝕩 V x}¨ 𝕘 LV 𝕩; w F _𝕣_ 𝕘 x: (CLR@) ⊢ (𝕘 LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ 𝕘 LV 𝕩}
%DEF fastone (
  _fastone ← { F _𝕣 𝕩:
    t ← 0
    n ← 𝕩
    @∘F¨ ↕n
    { 𝕊:
      c ← F•_timed @
      "wasn't fast enough" ! (n<10) ∨ t<5
      t+↩ c
      n+↩ 1
      c≥10000e¯9
    }•_while_⊢ 1
    n
  }
)

# in-place ⌾(n⊸⊑)
(
  %USE fastone
  { 𝕊 to‿ti:
    a←b← to•internal.Variation 10⥊ < ti•internal.Variation ↕1e6
    n ← {𝕊: a 10⊸+⌾(4⊸⊑)⌾(1⊸⊑) ↩} _fastone 10
    ! (4+10×n) ≡ 4⊑1⊑a
  }¨ ⟨"Ah"‿"Ai32", "Af"‿"Ai32", "Sh"‿"Ai32", "Sf"‿"Ai32", "Ah"‿"Ah"⟩
)

# in-place ⌾⊑
%USE fastone ⋄ a←5+↕1e6 ⋄ n←{𝕊: a 1⊸+⌾⊑↩}_fastone 4 ⋄ ! (10↑a) ≡ (5+n)∾6+↕9

# in-place ⌾(l⊸⊏) list
%USE fastone ⋄ a←⋈¨5+↕1e6       ⋄ n←{𝕊: a 1⊸+⌾(1‿3‿3‿¯1⊸⊏)↩}_fastone 4 ⋄ ! (⋈¨5‿6‿7‿8‿9‿1000000‿1000001‿1000002‿1000003‿1000004+0‿n‿0‿n‿0‿0‿0‿0‿0‿n) ≡ (5↑a)∾¯5↑a %!DEBUG
%USE fastone ⋄ a←5+↕1e6         ⋄ n←{𝕊: a 1⊸+⌾(1‿3‿3‿¯1⊸⊏)↩}_fastone 4 ⋄ ! (  5‿6‿7‿8‿9‿1000000‿1000001‿1000002‿1000003‿1000004+0‿n‿0‿n‿0‿0‿0‿0‿0‿n) ≡ (5↑a)∾¯5↑a %!DEBUG
%USE fastone ⋄ a←5+1e6⥊@+↕10000 ⋄ n←{𝕊: a 1⊸+⌾(1‿3‿3‿¯1⊸⊏)↩}_fastone 4 ⋄ ! (@+5‿6‿7‿8‿9‿10000‿10001‿10002‿10003‿10004        +0‿n‿0‿n‿0‿0‿0‿0‿0‿n) ≡ (5↑a)∾¯5↑a %!DEBUG

# in-place ⌾(l⊸⊏) highrank
%USE fastone ⋄ a←∘‿2‿2⥊⋈¨5+↕1e6       ⋄ n←{𝕊: a 1⊸+⌾(1‿3‿3‿¯1⊸⊏)↩}_fastone 4 ⋄ ! a ≡ (n×⟨1,3,¯1+≠a⟩∊˜↕≠a) + ∘‿2‿2⥊⋈¨5+↕1e6       %!DEBUG
%USE fastone ⋄ a←∘‿2‿2⥊5+↕1e6         ⋄ n←{𝕊: a 1⊸+⌾(1‿3‿3‿¯1⊸⊏)↩}_fastone 4 ⋄ ! a ≡ (n×⟨1,3,¯1+≠a⟩∊˜↕≠a) + ∘‿2‿2⥊5+↕1e6         %!DEBUG
%USE fastone ⋄ a←∘‿2‿2⥊5+1e6⥊@+↕10000 ⋄ n←{𝕊: a 1⊸+⌾(1‿3‿3‿¯1⊸⊏)↩}_fastone 4 ⋄ ! a ≡ (n×⟨1,3,¯1+≠a⟩∊˜↕≠a) + ∘‿2‿2⥊5+1e6⥊@+↕10000 %!DEBUG

# in-place ∾⟜atom & ∾⟜list
%USE fastone ⋄ %USE tvar2 ⋄ {𝕊v: j←<⍟(×≡) v ⋄ {a←𝕩 ⋄ n←{𝕊: a∾↩j}_fastone 4 ⋄ !(    n +≠𝕩)≡≠a ⋄ !a≡v¨a}_tvar2_"f" 1e6   ⥊<𝕩}¨ ⟨1, 1‿2, 'a'⟩
%USE fastone ⋄ %USE tvar2 ⋄ {𝕊v: j←10⥊<v    ⋄ {a←𝕩 ⋄ n←{𝕊: a∾↩j}_fastone 4 ⋄ !((10×n)+≠𝕩)≡≠a ⋄ !a≡v¨a}_tvar2_"f" 1e6   ⥊<𝕩}¨ ⟨1, 1‿2, 'a'⟩
%USE fastone ⋄ %USE tvar2 ⋄ {𝕊v: j←10⥊<v    ⋄ {a←𝕩 ⋄ n←{𝕊: a∾↩j}_fastone 4 ⋄ !(10⋈˜ n+≠𝕩)≡≢a ⋄ !a≡v¨a}_tvar2_"f" 1e5‿10⥊<𝕩}¨ ⟨1, 1‿2, 'a'⟩

# amortized fast ⍋ with long 𝕨 & tiny 𝕩
%USE fastone ⋄ %USE tvar2 ⋄ (1e6/↕10) {𝕨⊸⍋⟜𝕩 _fastone 1} _tvar2_"f" ↕10
