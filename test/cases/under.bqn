%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF eqvar %USE tvar ⋄ _eqvar ← {r ← 𝕨 𝔽 _tvar 𝕩 ⋄ !∘≡⟜(⊑r)¨ r ⋄ ⊑r}
%DEF evar %USE tvar ⋄ _evar ← {ok←{⇐} ⋄ r←⥊𝕨 ok∘𝔽⎊{𝕊: •CurrentError@} _tvar 𝕩 ⋄ {⊑ok∊r? !"TEST FAIL: variation didn't error"; ("TEST FAIL: varying error messages"⊸⋈ ! 1=≠)⍷r ⋄ !⊑r}}
# ¨ & ˘
# !% ⊢⌾(⊢¨) 4 # TODO enable
# !% ⊢⌾(⊢˘) 4 # TODO enable



# ⊑
!"𝔽⌾(a⊸⊑): Incompatible result elements" % 10‿20⌾(⟨1‿2,1‿2⟩⊸⊑) 4‿5⥊↕9
!"𝔽⌾(a⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨2⟩, got ⟨3⟩)" % 10‿20‿30⌾(⟨1‿2,2‿1⟩⊸⊑) 4‿4⥊↕16
!"𝔽⌾(a⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨2⟩, got ⟨3⟩)" % 1⊸∾⌾(⟨1‿2,2‿1⟩⊸⊑) 4‿4⥊↕16
!"𝔽⌾(n⊸⊑)𝕩: reading out-of-bounds (n≡¯10, 3≡≠𝕩)" % 1⊸+⌾(¯10⊸⊑) ↕3
!"⊑: 𝕨 contained list with mixed-type elements" % 'a'⌾(1‿'a'⊸⊑) 4‿5⥊↕9
!"⊑: Indexing out-of-bounds (index 1‿5 in array of shape 4‿5)" % 'a'⌾(1‿5⊸⊑) 4‿5⥊↕9
⟨10,20‿30⟩⌾(⟨1‿2,⟨1‿3,2‿1⟩⟩⊸⊑) 4‿5⥊↕9 %% 4‿5⥊0‿1‿2‿3‿4‿5‿6‿10‿20‿0‿1‿30‿3‿4‿5‿6‿7‿8‿0‿1
!"⊑: Picking item at wrong rank (index 1‿1‿1 in array of shape 10‿10)" % 1¨⌾(1‿1‿1⊸⊑) 10‿10⥊1
!"⊑: Picking item at wrong rank (index ⟨1⟩ in array of shape 10‿10)" % 1¨⌾(⟨1⟩⊸⊑) 10‿10⥊1
a←10‿10‿10⥊↕1000 ⋄ ! a ≡ a⌾((↕≢a)⊸⊑) a
≠{𝕊: a←(1+•rand.Range 200)•rand.Range 2 ⋄ i←•rand.Range≠a ⋄ ! a ≡ (i⊑a)⌾(i⊸⊑) 2⌾(i⊸⊑) a}¨↕10000
100⊸+⌾(5⊸⊑) ⥊↕4‿4 %% (100×5=↕16) + ⥊↕4‿4
100⊸+⌾(¯5⊸⊑) ⥊↕4‿4 %% (100×11=↕16) + ⥊↕4‿4
100⊸+⌾⊑ ⥊↕4‿4 %% (100×16↑1) + ⥊↕4‿4
1⊸+⌾(¯2⊸⊑) ⥊↕4 %% 0‿1‿3‿3
1⊸+⌾(¯2⊸⊑) ⥊0.5+↕4 %% 0.5‿1.5‿3.5‿3.5
# !% ⊢⌾⊑ 4 # TODO enable
# !% ⊢⌾(⊑⊑) 4 # TODO enable

# ⊏
n←500 ⋄ a←↕n ⋄ i←(-n)+↕2×n ⋄ r←⌽(2×n)⥊a ⋄ ! (⌽a) ≡ r⌾(i⊸⊏) a
!"⊏: 𝕩 cannot be a unit" % 0‿0⌾(0‿0⊸⊏) <0
!"⊏: 𝕩 cannot be a unit" % %USE evar ⋄ (↕0) {⊢⌾(𝕨⊸⊏)𝕩}_evar <0
!"𝔽⌾(a⊸⊏)𝕩: Indexing out-of-bounds (¯11∊a, ⟨10⟩≡≢𝕩)" % %USE evar ⋄ 5‿¯11 {0‿1⌾(𝕨⊸⊏)𝕩}_evar 10⥊1‿0‿1
!"𝔽⌾(a⊸⊏)𝕩: Indexing out-of-bounds (10∊a, ⟨10⟩≡≢𝕩)"  % %USE evar ⋄ 9‿10  {0‿1⌾(𝕨⊸⊏)𝕩}_evar 10⥊1‿0‿1
!"𝔽⌾(a⊸⊏)𝕩: Indexing out-of-bounds (10∊a, ⟨10⟩≡≢𝕩)"  % %USE evar ⋄ "ab" {𝕨⌾(9‿10⊸⊏)𝕩}_evar 10⥊"foo"
100⊸+⌾(⟨1‿2‿4⟩⊸⊏) <¨↕10 %% <¨ 0‿101‿102‿3‿104‿5‿6‿7‿8‿9
100⊸+⌾(1‿2‿4⊸⊏) <¨↕10   %% <¨ 0‿101‿102‿3‿104‿5‿6‿7‿8‿9
100⊸+⌾(1‿2‿4⊸⊏) ↕10     %%    0‿101‿102‿3‿104‿5‿6‿7‿8‿9
100⊸+¨⌾(1‿2‿4⊸⊏) ↕10    %%    0‿101‿102‿3‿104‿5‿6‿7‿8‿9
100⊸+⌾(1‿2‿¯4⊸⊏) <¨↕10  %% <¨ 0‿101‿102‿3‿4‿5‿106‿7‿8‿9
100⊸+⌾(1‿2‿¯4⊸⊏) ↕10    %%    0‿101‿102‿3‿4‿5‿106‿7‿8‿9
⟨10⊸+⌾(1‿2⊸⊏) 0↓a←↕4, a⟩ %% ⟨0‿11‿12‿3,0‿1‿2‿3⟩
%USE eqvar ⋄ 0‿0‿1‿1 {𝕨⌾(1‿2‿4‿5⊸⊏) 𝕩} _eqvar 1‿0‿1‿0‿1‿0‿1‿1‿0 %% 1‿0‿0‿0‿1‿1‿1‿1‿0
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (⟨2⟩ ≡ shape of a, ⟨⟩ ≡ shape of ⊏𝕩, ⟨3⟩ ≡ shape of result of 𝔽)" % 10‿20‿30⌾(2‿3⊸⊏) ↕10
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (⟨2⟩ ≡ shape of a, ⟨⟩ ≡ shape of ⊏𝕩, ⟨3⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾(2‿3⊸⊏) ↕10
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (⟨3⟩ ≡ shape of a, ⟨10⟩ ≡ shape of ⊏𝕩, 2‿10 ≡ shape of result of 𝔽)" % %USE evar ⋄ 1⊸↓⌾(2‿3‿4⊸⊏)_evar 10‿10⥊0
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4⌾(1‿1⊸⊏) ↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4⌾(1‿¯9⊸⊏) ↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % %USE evar ⋄ 3‿4  {𝕨⌾(1‿¯9⊸⊏) 𝕩}_evar ↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % %USE evar ⋄ "ab" {𝕨⌾(1‿¯9⊸⊏) 𝕩}_evar 'a'+↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4‿5⌾(1‿1‿2⊸⊏) ↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4‿5⌾(1‿1‿2⊸⊏) <¨↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4‿5⌾(1‿1‿2⊸⊏) a←<¨↕10 ⋄ •internal.Keep a
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4⌾(("Ah"•internal.Variation 1‿1)⊸⊏) <¨↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % %USE evar ⋄ (27≠↕30) {𝕨⌾((30⥊↕10)⊸⊏) 𝕩} _evar 100⥊1
!"𝔽⌾(a⊸⊏): Incompatible result elements" % %USE evar ⋄ !∘≡¨⟜⊏⊸⊢⟜⊑ {{¬⌾(31‿4⊸⊑)𝕩}∘•internal.Keep⌾((∾10⥊<↕≠𝕩)⊸⊏) 𝕩} _evar 10‿5⥊1‿1‿0‿0‿1‿1
%USE eqvar ⋄ {¬⌾((∾10⥊<↕≠𝕩)⊸⊏) 𝕩} _eqvar 10‿5⥊1‿1‿0‿0‿1‿1 %% 10‿5⥊0‿0‿1‿1‿0‿0
20‿20⌾(1‿1⊸⊏) ↕10 %% 0‿20∾2+↕8
100⊸+⌾(1‿2‿¯1⊸⊏) ↕5 %% 0‿101‿102‿3‿104
100⊸+⌾(1‿2‿¯1⊸⊏) <¨↕5 %% <¨ 0‿101‿102‿3‿104
1⊸+⌾(((100⥊↕10)∾10/↕10)⊸⊏) ↕10 %% 1+↕10
%USE eqvar ⋄ 1⊸+⌾((2/↕5)⊸⊏) _eqvar    ↕10 %%    <⟜5⊸+ ↕10
%USE eqvar ⋄ 1⊸+⌾((2/↕5)⊸⊏) _eqvar ≍˘ ↕10 %% ≍˘ <⟜5⊸+ ↕10
%USE eqvar ⋄ •internal.Keep⌾((2/↕5)⊸⊏) _eqvar 10‿0‿4⥊0 %% 10‿0‿4⥊0


# ⥊
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (16 ≡ ≢⥊𝕩, ⟨17⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾⥊ 4‿4⥊↕16
1⊸+⌾⥊ 5      %% (<6)
1⊸+⌾⥊ <5     %% (<6)
1⊸+⌾⥊ ⥊5     %% (⥊6)
1⊸+⌾⥊ ↕2     %% 1‿2
1⊸+⌾⥊ 2‿3⥊↕6 %% (1+2‿3⥊↕6)
⌽⌾⥊ 2‿3⥊↕6   %% (2‿3⥊⌽↕6)
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (1 ≡ ≢⥊𝕩, ⟨2⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾⥊ 4
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (1 ≡ ≢⥊𝕩, ⟨2⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾⥊ <4
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (1 ≡ ≢⥊𝕩, ⟨2⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾⥊ ⥊4
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (2 ≡ ≢⥊𝕩, ⟨3⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾⥊ 1‿2
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (6 ≡ ≢⥊𝕩, ⟨7⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾⥊ 2‿3⥊↕6
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (1 ≡ ≢⥊𝕩, ⟨⟩ ≡ shape of result of 𝔽)" % ⊑⌾⥊ 4
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (1 ≡ ≢⥊𝕩, ⟨⟩ ≡ shape of result of 𝔽)" % ⊑⌾⥊ <4
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (1 ≡ ≢⥊𝕩, ⟨⟩ ≡ shape of result of 𝔽)" % ⊑⌾⥊ ⥊4
!"𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (2 ≡ ≢⥊𝕩, 2‿1 ≡ shape of result of 𝔽)" % 2‿1⊸⥊⌾⥊ 1‿2
# !% ⊢⌾⥊ 4 # TODO enable
# !% ⊢⌾(3⥊⊢) 4 # TODO enable


# /
!"/: Lengths of components of 𝕨 must match 𝕩 (3 ≠ 10)" % 1⊸∾⌾(1‿0‿1⊸/) ↕10
10⊸+⌾(⟨1,0‿1⟩⊸/)↕2‿2 %% 2‿2⥊⟨0‿0,10‿11,1‿0,11‿11⟩
!"𝔽⌾(a⊸/): Incompatible result elements" % 1‿2⌾(2‿0⊸/)↕2
1⊸+⌾((↕10)⊸/) ↕10 %% ×⊸+ ↕10

# ↓ & ↑
!"𝔽⌾(n⊸↑)𝕩: 𝔽 must return an array with the same shape as its input (2 ≡ n, ⟨1⟩ ≡ shape of result of 𝔽)" % ⟨1⟩⌾(2⊸↑) ↕4
!"𝔽⌾(n⊸↓)𝕩: 𝔽 must return an array with the same shape as its input (2 ≡ n, ⟨1⟩ ≡ shape of result of 𝔽)" % ⟨1⟩⌾(2⊸↓) ↕4
!"𝔽⌾(n⊸↑)𝕩: 𝔽 must return an array with the same shape as its input (¯2 ≡ n, ⟨3⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾(¯2⊸↑) ↕4
!"𝔽⌾(n⊸↓)𝕩: 𝔽 must return an array with the same shape as its input (¯2 ≡ n, ⟨3⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾(¯2⊸↓) ↕4
!"𝔽⌾(n⊸↓)𝕩: 𝔽 must return an array with the same shape as its input (5 ≡ n, ⟨1⟩ ≡ shape of result of 𝔽)" % ⟨1⟩⌾(5⊸↓) ↕4
!"𝔽⌾(n⊸↓)𝕩: 𝔽 must return an array with the same shape as its input (1 ≡ n, 4‿5‿2 ≡ shape of result of 𝔽)" % ⍉⌾(1⊸↓) 3‿4‿5⥊↕60
!"𝔽⌾(n⊸↑)𝕩: 𝔽 must return an array with the same shape as its input (¯2 ≡ n, 4‿5‿2 ≡ shape of result of 𝔽)" % ⍉⌾(¯2⊸↑) 3‿4‿5⥊↕60
!"𝔽⌾(n⊸↑)𝕩: Cannot modify fill with Under (5 ≡ n, ⟨4⟩ ≡ ≢𝕩)" % ⊢⌾(5⊸↑) ↕4
⟨⟩⌾(4⊸↓) ↕4 %% ↕4
⟨⟩⌾(5⊸↓) ↕4 %% ↕4
•internal.Keep⌾(¯5⊸↓) ↕4 %% ↕4
{!(a×1+9× 0‿1‿2∊𝕩+↕3) ≡ 10⊸×⌾(𝕩⊸↓) a←3‿4‿5⥊↕60}¨ ¯5+↕10
{!(a×1+9×¬0‿1‿2∊𝕩+↕3) ≡ 10⊸×⌾(𝕩⊸↑) a←3‿4‿5⥊↕60}¨ ¯3+↕7
