%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF eqvar ⟨_eqvar⟩←⊑•args
%DEF evar %USE tvar ⋄ _evar ← {ok←{⇐} ⋄ r←⥊𝕨 ok∘𝔽⎊{𝕊: •CurrentError@} _tvar 𝕩 ⋄ {⊑ok∊r? !"TEST FAIL: variation didn't error"; ("TEST FAIL: varying error messages"⊸⋈ ! 1=≠)⍷r ⋄ (⊑r)!0}}
%DEF eqerr %USE eqvar ⋄ _eqerr ← {ok←{⇐} ⋄ r←⥊𝕨 ok∘𝔽⎊{𝕊: •CurrentError@}⌜ 𝕩 ⋄ {⊑ok∊r? !"TEST FAIL: case didn't error"; ("TEST FAIL: varying error messages"⊸⋈ ! 1=≠)⍷r ⋄ (⊑r)!0}}
%DEF rand r ← (⊑•args).GetRand@
# ¨ & ˘
# !% ⊢⌾(⊢¨) 4 # TODO enable
# !% ⊢⌾(⊢˘) 4 # TODO enable

# ¯0¨⌾((⥊𝕨)⊸⊏) 𝕩
%DEF nz InsNZ ← {⥊ (¯64↑1)˘⌾((⥊𝕨)⊸⊏)⌾(64‿1•bit._cast) ≍˘ 𝕩} ⋄ IsNZ ← {(¯64↑1)⊸≡˘ 64‿1•bit._cast ≍˘ 𝕩}
%USE nz ⋄ IsNZ⊸⋈ 1‿2 InsNZ 0.5‿0‿1‿0‿¯1‿2 %% ⟨0‿1‿1‿0‿0‿0, 0.5‿0‿0‿0‿¯1‿2⟩
%USE nz ⋄ IsNZ⊸⋈ 1‿2 InsNZ   0‿0‿1‿0‿¯1‿2 %% ⟨0‿1‿1‿0‿0‿0,   0‿0‿0‿0‿¯1‿2⟩

# ⌾(F⊸Op)
10⊸+⌾((2÷˜≢)⊸⊑) ↕10 %% 0‿1‿2‿3‿4‿15‿6‿7‿8‿9
10⊸+⌾((2÷˜≢)⊑⊢) ↕10 %% 0‿1‿2‿3‿4‿15‿6‿7‿8‿9
10⊸+⌾((↕2÷˜≠)⊸⊏) ↕10 %% 10‿11‿12‿13‿14‿5‿6‿7‿8‿9
10⊸+⌾((2÷˜≠)⊸↑) ↕10 %% 10‿11‿12‿13‿14‿5‿6‿7‿8‿9
10⊸+⌾((2÷˜≠)⊸⥊) ↕10 %% 10‿11‿12‿13‿14‿5‿6‿7‿8‿9
10⊸+⌾((↕∘≠<2÷˜≠)⊸/) ↕10 %% 10‿11‿12‿13‿14‿5‿6‿7‿8‿9
10⊸+⌾((2÷˜≠)⊸↓) ↕10 %% 0‿1‿2‿3‿4‿15‿16‿17‿18‿19
(↕10)⊸+⌾((2÷˜≠)⊸⌽) 10×↕10 %% 5‿16‿27‿38‿49‿50‿61‿72‿83‿94
⌽⌾((¯1+=)⊸⍉) 3‿3⥊↕9 %% 3‿3⥊2‿1‿0‿5‿4‿3‿8‿7‿6
!"Expected integer, got function" % 1⊸+⌾((¯1+≠)˙⊑⊢)"ab"
!"𝕨⊑𝕩: 𝕨 must be a numeric array" % ⊢⌾((≠⊑⟨@,@,¯1+≠⟩˙)⊸⊑)"ab"
!"𝔽◶𝕘𝕩: 𝕘 must have rank 1 when index is a number" % ⊢⌾(≠◶⟨@,@,¯1+≠⟩⊸⊑)"ab"



# ⊑
!"𝔽⌾(a⊸⊑): Incompatible result elements" % 10‿20⌾(⟨1‿2,1‿2⟩⊸⊑) 4‿5⥊↕9
!"Expected integer, got character" % 1⊸+⌾('a'⊸⊑) "hello"
!"Expected integer, got 1.5" % 1⊸+⌾(1.5⊸⊑) "hello"
!"𝔽⌾(n⊸⊑)𝕩: reading out-of-bounds (n≡100, 5≡≠𝕩)" % 1⊸+⌾(100⊸⊑) "hello"
!"𝕨⊑𝕩: 𝕩 must be a list when 𝕨 is a number (4‿4 ≡ ≢𝕩)" % 1⊸+⌾(100⊸⊑) 4‿4⥊↕16
!"𝔽⌾(nested⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨2⟩, got ⟨3⟩)" % 10‿20‿30⌾(⟨1‿2,2‿1⟩⊸⊑) 4‿4⥊↕16
!"𝔽⌾(nested⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected unit array, got atom)" % 1⌾((<⋈2)⊸⊑) ↕10
!"𝔽⌾(nested⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected unit array, got atom)" % 1⌾((<1‿2)⊸⊑) 3‿3⥊↕9
!"𝔽⌾(nested⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨1⟩, got atom)" % 1⌾((⋈1‿2)⊸⊑) 3‿3⥊↕9
!"𝔽⌾(nested⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨1⟩, got unit array)" % (<1)⌾((⋈1‿2)⊸⊑) 3‿3⥊↕9
!"⁼: Inverse not found" % 1⌾((<⋈⋈2)⊸⊑) ↕10
!"⁼: Inverse not found" % 1⌾((<⋈1‿2)⊸⊑) 3‿3⥊↕9
!"⁼: Inverse not found" % (<1)⌾((<⟨1‿2,2‿1⟩)⊸⊑) 3‿3⥊↕9
!"⁼: Inverse not found" % 3‿4⌾((<⟨1‿2,2‿1⟩)⊸⊑) 3‿3⥊↕9
!"𝕨⊑𝕩: Leaf arrays in 𝕨 must have rank 1 (element: (<2))" % 1⌾((<2)⊸⊑) ↕10
!"𝔽⌾(nested⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨2⟩, got ⟨3⟩)" % 1⊸∾⌾(⟨1‿2,2‿1⟩⊸⊑) 4‿4⥊↕16
!"𝔽⌾(n⊸⊑)𝕩: reading out-of-bounds (n≡¯10, 3≡≠𝕩)" % 1⊸+⌾(¯10⊸⊑) ↕3
!"𝕨⊑𝕩: 𝕨 contained list with mixed-type elements" % 'a'⌾(1‿'a'⊸⊑) 4‿5⥊↕9
!"𝕨⊑𝕩: Indexing out-of-bounds (index 1‿5 in array of shape 4‿5)" % 'a'⌾(1‿5⊸⊑) 4‿5⥊↕9
⟨10,20‿30⟩⌾(⟨1‿2,⟨1‿3,2‿1⟩⟩⊸⊑) 4‿5⥊↕9 %% 4‿5⥊0‿1‿2‿3‿4‿5‿6‿10‿20‿0‿1‿30‿3‿4‿5‿6‿7‿8‿0‿1
!"𝕨⊑𝕩: Picking item at wrong rank (index 1‿1‿1 in array of shape 10‿10)" % 1¨⌾(1‿1‿1⊸⊑) 10‿10⥊1
!"𝕨⊑𝕩: Picking item at wrong rank (index ⟨1⟩ in array of shape 10‿10)" % 1¨⌾(⟨1⟩⊸⊑) 10‿10⥊1
a←10‿10‿10⥊↕1000 ⋄ ! a ≡ a⌾((↕≢a)⊸⊑) a
%USE rand ⋄ ≠{𝕊: a←(1+ r.Range 200) r.Range 2 ⋄ i←r.Range≠a ⋄ ! a ≡ (i⊑a)⌾(i⊸⊑) 2⌾(i⊸⊑) a}¨↕10000
100⊸+⌾(5⊸⊑) ⥊↕4‿4 %% (100×5=↕16) + ⥊↕4‿4
100⊸+⌾(¯5⊸⊑) ⥊↕4‿4 %% (100×11=↕16) + ⥊↕4‿4
100⊸+⌾⊑ ⥊↕4‿4 %% (100×16↑1) + ⥊↕4‿4
1⊸+⌾(¯2⊸⊑) ⥊↕4 %% 0‿1‿3‿3
1⊸+⌾(¯2⊸⊑) ⥊0.5+↕4 %% 0.5‿1.5‿3.5‿3.5
# !% ⊢⌾⊑ 4 # TODO enable
# !% ⊢⌾(⊑⊑) 4 # TODO enable

# ⊏
n←500 ⋄ a←↕n ⋄ i←(-n)+↕2×n ⋄ r←⌽(2×n)⥊a ⋄ ! (⌽a) ≡ r⌾(i⊸⊏) a
!"𝕨⊏𝕩: 𝕩 cannot be a unit" % 0‿0⌾(0‿0⊸⊏) <0
!"Expected integer, got character" % 1⊸+⌾('a'⊸⊏) "hello"
!"Expected integer, got 1.5" % 1⊸+⌾(1.5⊸⊏) "hello"
!"𝕨⊏𝕩: Indexing out-of-bounds (100∊𝕨, 5≡≠𝕩)" % 1⊸+⌾(100⊸⊏) "hello"
!"𝕨⊏𝕩: Indexing out-of-bounds (100∊𝕨, 4≡≠𝕩)" % 1⊸+⌾(100⊸⊏) 4‿4⥊↕16
!"𝕨⊏𝕩: 𝕩 cannot be a unit" % %USE evar ⋄ (↕0) {⊢⌾(𝕨⊸⊏)𝕩}_evar <0
!"𝕨⊏𝕩: Indexing out-of-bounds (¯11∊𝕨, 10≡≠𝕩)" % %USE evar ⋄ 5‿¯11 {0‿1⌾(𝕨⊸⊏)𝕩}_evar 10⥊1‿0‿1
!"𝕨⊏𝕩: Indexing out-of-bounds (10∊𝕨, 10≡≠𝕩)" %  %USE evar ⋄ 9‿10  {0‿1⌾(𝕨⊸⊏)𝕩}_evar 10⥊1‿0‿1
!"𝕨⊏𝕩: Indexing out-of-bounds (10∊𝕨, 10≡≠𝕩)" %  %USE evar ⋄ "ab" {𝕨⌾(9‿10⊸⊏)𝕩}_evar 10⥊"foo"
!"𝕨⊏𝕩: Indexing out-of-bounds (4.611686018427388e18∊𝕨, 3≡≠𝕩)" % %USE eqerr ⋄ {𝕩⌾((3⥊2⋆62)⊸⊏) "abc"}_eqerr ⟨"abc", {𝕊: "abc"}⟩
!"𝕨⊏𝕩: Indexing out-of-bounds (2∊𝕨, 2≡≠𝕩)" % %USE evar ⋄ (3‿50⥊1‿0‿1) {𝕨⌾(0‿2⊸⊏) 𝕩}_evar 2‿50⥊↕100
!"𝕨⊏𝕩: Indexing out-of-bounds (2∊𝕨, 2≡≠𝕩)" % %USE evar ⋄ 0‿2 {1¨⌾(𝕨⊸⊏) 𝕩}_evar 2‿50⥊0
%USE eqvar ⋄ (2‿0⥊0) {𝕨⌾(0‿1⊸⊏) 𝕩}_eqvar 10‿0⥊2 %% 10‿0⥊0
%USE eqerr ⋄ ! ⊑ (<⋈⁼ ⍷{1⊣𝕩⌾((3⥊2⋆63)⊸⊏) "abc"}⎊{𝕊: •CurrentError@}¨ ⟨"abc", {𝕊: "abc"}⟩) ∊ "Integer out of range: 9.223372036854776e18"‿"𝕨⊏𝕩: Indexing out-of-bounds (9.223372036854776e18∊𝕨, 3≡≠𝕩)"
!"Expected integer, got 1.1" % %USE eqerr ⋄ {𝕩⌾((3⥊1.1)⊸⊏) ↕3}_eqerr ⟨↕3, {1+𝕩}⟩
!"𝕨⊏𝕩: Indexing out-of-bounds (10∊𝕨, 3≡≠𝕩)" % %USE eqerr ⋄ {𝕩⌾(10‿11‿12⊸⊏) ↕3}_eqerr ⟨↕3, {1+𝕩}⟩
100⊸+⌾(⟨1‿2‿4⟩⊸⊏) <¨↕10 %% <¨ 0‿101‿102‿3‿104‿5‿6‿7‿8‿9
100⊸+⌾(1‿2‿4⊸⊏) <¨↕10   %% <¨ 0‿101‿102‿3‿104‿5‿6‿7‿8‿9
100⊸+⌾(1‿2‿4⊸⊏) ↕10     %%    0‿101‿102‿3‿104‿5‿6‿7‿8‿9
100⊸+¨⌾(1‿2‿4⊸⊏) ↕10    %%    0‿101‿102‿3‿104‿5‿6‿7‿8‿9
100⊸+⌾(1‿2‿¯4⊸⊏) <¨↕10  %% <¨ 0‿101‿102‿3‿4‿5‿106‿7‿8‿9
100⊸+⌾(1‿2‿¯4⊸⊏) ↕10    %%    0‿101‿102‿3‿4‿5‿106‿7‿8‿9
⟨10⊸+⌾(1‿2⊸⊏) 0↓a←↕4, a⟩ %% ⟨0‿11‿12‿3,0‿1‿2‿3⟩
%USE eqvar ⋄ 0‿0‿1‿1 {𝕨⌾(1‿2‿4‿5⊸⊏) 𝕩} _eqvar 1‿0‿1‿0‿1‿0‿1‿1‿0 %% 1‿0‿0‿0‿1‿1‿1‿1‿0
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected unit array, got atom)" % 10⌾(2⊸⊏) ↕10
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected unit array, got atom)" % 0⌾((<4)⊸⊏) "hello"
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨2⟩, got ⟨3⟩)" % 10‿20‿30⌾(2‿3⊸⊏) ↕10
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨2⟩, got ⟨3⟩)" % 1⊸∾⌾(2‿3⊸⊏) ↕10
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected 3‿10, got 2‿10)" % %USE evar ⋄ 1⊸↓⌾(2‿3‿4⊸⊏)_evar 10‿10⥊0
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨1⟩, got atom)" % %USE evar ⋄ ⟨0⟩ {1⌾(𝕨⊸⊏)𝕩}_evar ⟨0⟩
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected 1‿3‿4, got atom)" % %USE evar ⋄ ⟨0⟩ {1⌾(𝕨⊸⊏)𝕩}_evar 2‿3‿4⥊3
!"𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected 2‿3‿4‿5‿1, got ⟨2⟩)" % 1‿2⌾((2‿3‿4⥊2)⊸⊏) 4‿5‿1⥊6
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4⌾(1‿1⊸⊏) ↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4⌾(1‿¯9⊸⊏) ↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % %USE evar ⋄ 3‿4  {𝕨⌾(1‿¯9⊸⊏) 𝕩}_evar ↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % %USE evar ⋄ "ab" {𝕨⌾(1‿¯9⊸⊏) 𝕩}_evar 'a'+↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4‿5⌾(1‿1‿2⊸⊏) ↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4‿5⌾(1‿1‿2⊸⊏) <¨↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4‿5⌾(1‿1‿2⊸⊏) a←<¨↕10 ⋄ •internal.Keep a
!"𝔽⌾(a⊸⊏): Incompatible result elements" % 3‿4⌾(("Ah"•internal.Variation 1‿1)⊸⊏) <¨↕10
!"𝔽⌾(a⊸⊏): Incompatible result elements" % %USE evar ⋄ (27≠↕30) {𝕨⌾((30⥊↕10)⊸⊏) 𝕩} _evar 100⥊1
!"𝔽⌾(a⊸⊏): Incompatible result elements" % %USE evar ⋄ {{¬⌾(31‿4⊸⊑)𝕩}∘•internal.Keep⌾((∾10⥊<↕≠𝕩)⊸⊏) 𝕩} _evar 10‿5⥊1‿1‿0‿0‿1‿1
%USE eqvar ⋄ {¬⌾((∾10⥊<↕≠𝕩)⊸⊏) 𝕩} _eqvar 10‿5⥊1‿1‿0‿0‿1‿1 %% 10‿5⥊0‿0‿1‿1‿0‿0
20‿20⌾(1‿1⊸⊏) ↕10 %% 0‿20∾2+↕8
100⊸+⌾(1‿2‿¯1⊸⊏) ↕5 %% 0‿101‿102‿3‿104
100⊸+⌾(1‿2‿¯1⊸⊏) <¨↕5 %% <¨ 0‿101‿102‿3‿104
1⊸+⌾(((100⥊↕10)∾10/↕10)⊸⊏) ↕10 %% 1+↕10
%USE eqvar ⋄ 1⊸+⌾((2/↕5)⊸⊏) _eqvar    ↕10 %%    <⟜5⊸+ ↕10
%USE eqvar ⋄ 1⊸+⌾((2/↕5)⊸⊏) _eqvar ≍˘ ↕10 %% ≍˘ <⟜5⊸+ ↕10
%USE eqvar ⋄ •internal.Keep⌾((2/↕5)⊸⊏) _eqvar 10‿0‿4⥊0 %% 10‿0‿4⥊0
%USE eqvar ⋄ (<⟨⟩) {•internal.Keep⌾(𝕨⊸⊏) 𝕩}_eqvar¨ ⟨0‿1, "ab"‿"cd"⟩ %% ⟨0‿1, "ab"‿"cd"⟩
%USE eqvar ⋄ ⟨0÷0,0÷0⟩ {𝕨⌾(0‿0⊸⊏) 𝕩}_eqvar 3⥊0 %% ⟨0÷0,0,0⟩
%USE eqvar ⋄ %USE nz ⋄ 0‿0 {(0 InsNZ 2⥊0)⌾(𝕨⊸⊏)𝕩}_eqvar 3⥊3 %% 0‿3‿3
%USE eqvar ⋄ "CD" {𝕨¨⌾(2‿3‿2‿3‿4⊸⊏) 𝕩}_eqvar 10⥊<"ab" %% "ab"‿"ab"‿"CD"‿"CD"‿"CD"‿"ab"‿"ab"‿"ab"‿"ab"‿"ab"
%USE eqvar ⋄ "CD" {𝕨¨⌾(2‿3‿2‿1⊸⊏) 𝕩}_eqvar 5‿2⥊<"ab" %% ≍˜˘ "ab"‿"CD"‿"CD"‿"CD"‿"ab"
%USE eqvar ⋄ 2‿3‿2‿1 {!˙¨⌾(𝕨⊸⊏) 𝕩}_eqvar 5‿2⥊<"ab" %% ≍˜˘ "ab"‿!‿!‿!‿"ab"
%USE eqvar ⋄ 2‿3‿2‿1 {(   32⊸+⊸≍˘ "AB"‿"CD"‿"AB"‿"EF")⌾(𝕨⊸⊏) 𝕩}_eqvar 5‿2⥊<"?" %% 5‿2⥊⟨"?","?","ef","EF","ab","AB","cd","CD","?","?"⟩
%USE eqvar ⋄ 2‿3‿2‿1 {{𝕊: 32⊸+⊸≍˘ "AB"‿"CD"‿"AB"‿"EF"}⌾(𝕨⊸⊏) 𝕩}_eqvar 5‿2⥊<"?" %% 5‿2⥊⟨"?","?","ef","EF","ab","AB","cd","CD","?","?"⟩
%USE eqvar ⋄ 2‿3‿2‿1 {(0‿1⊸+˘ 10‿20‿10‿30)⌾(𝕨⊸⊏) 𝕩}_eqvar 5‿2⥊↕10 %% 5‿2⥊0‿1‿30‿31‿10‿11‿20‿21‿8‿9
%USE eqvar ⋄ 2‿3‿2‿1 {0¨⌾(𝕨⊸⊏) 𝕩}_eqvar 5‿2⥊1 %% 5‿2⥊1‿1‿0‿0‿0‿0‿0‿0‿1‿1
%USE eqvar ⋄ 2‿3‿2‿1 {0¨⌾(𝕨⊸⊏) 𝕩}_eqvar 5‿1⥊1 %% ≍˘1‿0‿0‿0‿1
%USE eqvar ⋄ 2‿3‿2‿1 {0¨⌾(𝕨⊸⊏) 𝕩}_eqvar 2≠↕10 %% 1‿0‿0‿0‿1‿1‿1‿1‿1‿1
%USE eqvar ⋄ 2‿3‿2‿1 {!˙¨⌾(𝕨⊸⊏) 𝕩}_eqvar 2≠↕10 %% 1‿!‿!‿!‿1‿1‿1‿1‿1‿1
%USE eqvar ⋄ [1‿2,3‿1,1‿8] {99¨⌾(𝕨⊸⊏) 𝕩}_eqvar ↕10 %% 0‿99‿99‿99‿4‿5‿6‿7‿99‿9
%USE eqvar ⋄ 2‿3‿2‿1 {'?'¨⌾(𝕨⊸⊏) 𝕩}_eqvar "abcdefgh"+⌜↕3 %% ["abc","???","???","???","efg","fgh","ghi","hij"]
%USE eqvar ⋄ 2‿3‿2‿1 {'?'¨⌾(𝕨⊸⊏) 𝕩}_eqvar "abcdefgh" %% "a???efgh"
%USE eqvar ⋄ 2‿3‿2‿1 {'𝕨'¨⌾(𝕨⊸⊏) 𝕩}_eqvar "abcdefgh" %% "a𝕨𝕨𝕨efgh"


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

⍉⌾(2‿2⊸⥊) ↕10 %% 0‿2‿1‿3‿4‿5‿6‿7‿8‿9
1⊸+⌾(10‿2⊸⥊) ↕7 %% 1‿2‿3‿4‿5‿6‿7
{!𝕩≡<0 ⋄ <9}⌾(⟨⟩⊸⥊) ↕7 %% 9‿1‿2‿3‿4‿5‿6
⌽⌾(∘‿2⊸⥊) ↕10 %% 8‿9‿6‿7‿4‿5‿2‿3‿0‿1
!"𝔽⌾(a⊸⥊): 𝔽 must return an array with the same shape as its input (⟨⟩ ≡ ≢a⥊𝕩, ⟨⟩ ≡ shape of result of 𝔽)" % ⊑⌾(⟨⟩⊸⥊) ↕7
!"𝔽⌾(a⊸⥊): 𝔽 must return an array with the same shape as its input (⟨2⟩ ≡ ≢a⥊𝕩, ⟨⟩ ≡ shape of result of 𝔽)" % 0⌾(2⊸⥊) ↕10
!"𝔽⌾(a⊸⥊): 𝔽 must return an array with the same shape as its input (⟨2⟩ ≡ ≢a⥊𝕩, ⟨1⟩ ≡ shape of result of 𝔽)" % 1⊸↓⌾(2⊸⥊) ↕10
!"𝔽⌾(a⊸⥊): 𝔽 must return an array with the same shape as its input (2‿2 ≡ ≢a⥊𝕩, 1‿2 ≡ shape of result of 𝔽)" % 1⊸↓⌾(2‿2⊸⥊) ↕10
!"𝔽⌾(a⊸⥊): 𝔽 must return an array with the same shape as its input (2‿2 ≡ ≢a⥊𝕩, ⟨⟩ ≡ shape of result of 𝔽)" % ⊑⌾(2‿2⊸⥊) ↕10
!"𝔽⌾(a⊸⥊): 𝔽 must return an array with the same shape as its input (3‿2 ≡ ≢a⥊𝕩, 2‿3 ≡ shape of result of 𝔽)" % ⍉⌾(3‿2⊸⥊) ↕10
!"⁼: Inverse not found" % ⊑⌾(2‿20⊸⥊) ↕10
!"⁼: Inverse not found" % ⍉⌾(3‿20⊸⥊) ↕10
1⊸+⌾(2‿2⊸⥊) 5 %% <6
!"⌾: Incompatible result elements in structural Under" % 1‿2⊸+⌾(2‿2⊸⥊) 5
10‿20‿30⊸+˘⌾(4‿3⊸⥊) 1‿2‿3 %% 11‿22‿33


# /
!"𝕨/𝕩: Lengths of components of 𝕨 must match 𝕩 (3 ≠ 10)" % 1⊸∾⌾(1‿0‿1⊸/) ↕10
10⊸+⌾(⟨1,0‿1⟩⊸/)↕2‿2 %% 2‿2⥊⟨0‿0,10‿11,1‿0,11‿11⟩
!"𝔽⌾(a⊸/): Incompatible result elements" % 1‿2⌾(2‿0⊸/)↕2
1⊸+⌾((↕10)⊸/) ↕10 %% ×⊸+ ↕10
%USE eqvar ⋄ <‿≥‿> {𝕨⌾(1‿0‿0‿1‿1⊸/) 𝕩}_eqvar +‿-‿×‿÷‿⋆ %% <‿-‿×‿≥‿>
%USE eqvar ⋄ ["wx"‿"WX","yz"‿"YZ"] {𝕨⌾(1‿0‿0‿1⊸/) 𝕩}_eqvar (<"aA")+↕4‿2 %% 4‿2⥊⟨"wx","WX","bA","bB","cA","cB","yz","YZ"⟩
%USE rand ⋄ {m←𝕩 r.Range 2 ⋄ ! (m{𝕨?"huh";⥊'a'+𝕩}¨↕𝕩) ≡ "huh"¨⌾(m⊸/) 'a'+↕⋈𝕩}¨ ↕100
!"Integer out of range: 3.6893488147419103e19" % 1¨⌾(a⊸/) a←100⥊2⋆65
!"Expected non-negative integer, got ¯10" % %USE evar ⋄ 1¨⌾(a⊸/)_evar a←¯10⌾(50⊸⊑) 100⥊10

!"𝕨/𝕩: Simple 𝕨 must have rank 0 or 1 (2≡=𝕨)" % %USE evar ⋄ (2‿2⥊1‿0) {⊣⌾(𝕨⊸/)˜ 𝕩}_evar 2‿2⥊'a'
!"𝕨/𝕩: 𝕩 must have rank at least 1 for simple 𝕨" % %USE evar ⋄ 1 ⊣⌾(1‿0‿1⊸/) "ABC"
!"𝕨/𝕩: 𝕩 must have rank at least 1 for simple 𝕨" % %USE evar ⋄ "abc" ⊣⌾(1‿0‿1⊸/) 1
%USE eqvar ⋄ {"abc" ⊣⌾(𝕩⊸/)_eqvar "ABC"}¨ ↕3 %%% "ABC"‿"abc"‿"abc"
!"𝕨/𝕩: Lengths of components of 𝕨 must match 𝕩 (3 ≠ 2)" % %USE evar ⋄ "abc" ⊣⌾(1‿0‿1⊸/) "AB"
!"𝕨/𝕩: Lengths of components of 𝕨 must match 𝕩 (3 ≠ 2)" % %USE evar ⋄ "ab" ⊣⌾(1‿0‿1⊸/) "ABC"
!"𝕨/𝕩: Lengths of components of 𝕨 must match 𝕩 (2 ≠ 3)" % %USE evar ⋄ "abc" ⊣⌾(1‿0⊸/) "ABC"
%USE eqvar ⋄ (4/0‿1) ⊣⌾(0‿0‿1‿1‿0‿0‿1‿1⊸/)_eqvar 8⥊0‿1 %%% 0‿1‿0‿0‿0‿1‿1‿1
%USE eqvar ⋄ (4/0‿1) ⊣⌾(0‿0‿1‿2‿0‿0‿8‿9⊸/)_eqvar 8⥊0‿1 %%% 0‿1‿0‿0‿0‿1‿1‿1
!"Expected non-negative integer, got ¯1" % %USE evar ⋄ ¯1‿0 {⊣⌾(𝕨⊸/)˜ 𝕩}_evar ↕2
%USE eqvar ⋄  "abcde" ⊣⌾(1‿0‿0‿1‿1⊸/)_eqvar "ABCDE" %%% "aBCde"
%USE eqvar ⋄  "abcde" ⊣⌾(1‿0‿0‿1‿1⊸/)_eqvar 'A'+↕⋈5 %%% " "WF⟨'a',"B","C",'d','e'⟩
%USE eqvar ⋄ ('a'+↕⋈5)⊣⌾(1‿0‿0‿1‿1⊸/)_eqvar 'A'+↕⋈5 %%% " "WF ⋈¨ "aBCde"
%USE eqvar ⋄ ('a'+↕⋈5)⊣⌾(1‿0‿0‿1‿1⊸/)_eqvar "ABCDE" %%% ' 'WF⟨"a",'B','C',"d","e"⟩

# ↓ & ↑
!"𝔽⌾(n⊸↑)𝕩: 𝔽 must return an array with the same shape as its input (2 ≡ n, ⟨1⟩ ≡ shape of result of 𝔽)" % ⟨1⟩⌾(2⊸↑) ↕4
!"𝔽⌾(n⊸↓)𝕩: 𝔽 must return an array with the same shape as its input (2 ≡ n, ⟨1⟩ ≡ shape of result of 𝔽)" % ⟨1⟩⌾(2⊸↓) ↕4
!"𝔽⌾(n⊸↑)𝕩: 𝔽 must return an array with the same shape as its input (¯2 ≡ n, ⟨3⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾(¯2⊸↑) ↕4
!"𝔽⌾(n⊸↓)𝕩: 𝔽 must return an array with the same shape as its input (¯2 ≡ n, ⟨3⟩ ≡ shape of result of 𝔽)" % 1⊸∾⌾(¯2⊸↓) ↕4
!"𝔽⌾(n⊸↓)𝕩: 𝔽 must return an array with the same shape as its input (5 ≡ n, ⟨1⟩ ≡ shape of result of 𝔽)" % ⟨1⟩⌾(5⊸↓) ↕4
!"𝔽⌾(n⊸↓)𝕩: 𝔽 must return an array with the same shape as its input (1 ≡ n, 4‿5‿2 ≡ shape of result of 𝔽)" % ⍉⌾(1⊸↓) 3‿4‿5⥊↕60
!"𝔽⌾(n⊸↑)𝕩: 𝔽 must return an array with the same shape as its input (¯2 ≡ n, 4‿5‿2 ≡ shape of result of 𝔽)" % ⍉⌾(¯2⊸↑) 3‿4‿5⥊↕60
!"𝔽⌾(n⊸↑)𝕩: 𝔽 must return an array with the same shape as its input (5 ≡ n, 5‿2 ≡ shape of result of 𝔽)" % 5‿2⊸⥊⌾(5⊸↑)"abcdefg"
!"𝔽⌾(n⊸↑)𝕩: 𝔽 must return an array with the same shape as its input (1 ≡ n, 1‿1 ≡ shape of result of 𝔽)" % 1‿1⊸⥊⌾(1⊸↑) 3
!"𝔽⌾(n⊸↑)𝕩: Cannot modify fill with Under (5 ≡ n, ⟨4⟩ ≡ ≢𝕩)" % ⊢⌾(5⊸↑) ↕4
⟨⟩⌾(4⊸↓) ↕4 %% ↕4
⟨⟩⌾(5⊸↓) ↕4 %% ↕4
•internal.Keep⌾(¯5⊸↓) ↕4 %% ↕4
{!(a×1+9× 0‿1‿2∊𝕩+↕3) ≡ 10⊸×⌾(𝕩⊸↓) a←3‿4‿5⥊↕60}¨ ¯5+↕10
{!(a×1+9×¬0‿1‿2∊𝕩+↕3) ≡ 10⊸×⌾(𝕩⊸↑) a←3‿4‿5⥊↕60}¨ ¯3+↕7

%USE eqvar ⋄ 9‿0‿0‿0‿5‿3 {  1¨⌾(𝕨⊸/)𝕩}_eqvar 0‿1‿0‿1‿0‿1 %% 1‿1‿0‿1‿1‿1
%USE eqvar ⋄ 1‿0‿0‿0‿1‿1 {  1¨⌾(𝕨⊸/)𝕩}_eqvar 0‿1‿0‿1‿0‿1 %% 1‿1‿0‿1‿1‿1
%USE eqvar ⋄ 1‿0‿0‿0‿1‿1 {  0¨⌾(𝕨⊸/)𝕩}_eqvar 0‿1‿0‿1‿0‿1 %% 0‿1‿0‿1‿0‿0
%USE eqvar ⋄ 1‿0‿0‿0‿1‿1 {'!'¨⌾(𝕨⊸/)𝕩}_eqvar 0‿1‿0‿1‿0‿1 %% '!'‿1‿0‿1‿'!'‿'!'
%USE eqvar ⋄ 1‿0‿0‿0‿1‿1 { ≥˙¨⌾(𝕨⊸/)𝕩}_eqvar 0‿1‿0‿1‿0‿1 %% ≥‿1‿0‿1‿≥‿≥
%USE eqvar ⋄ 1‿0‿0‿0‿1‿1 {  3¨⌾(𝕨⊸/)𝕩}_eqvar "hellow" %% 3‿'e'‿'l'‿'l'‿3‿3
%USE eqvar ⋄ 1‿0‿0‿0‿1‿1 { +˙¨⌾(𝕨⊸/)𝕩}_eqvar -‿÷‿×‿=‿<‿> %% +‿÷‿×‿=‿+‿+
!"𝕨/𝕩: Lengths of components of 𝕨 must match 𝕩 (6 ≠ 7)" % 0¨⌾(1‿0‿0‿0‿1‿1⊸/) 0‿1‿0‿1‿0‿1‿0
4↑ (⋈⋈3)⌾(0‿1‿0⊸/) ↕⋈3 %% ⟨⋈0,⋈3,⋈2,⋈0⟩
4↑     ⊢⌾(0‿1‿0⊸/) ↕⋈3 %% ⟨⋈0,⋈1,⋈2,⋈0⟩
4↑     ⊢⌾(0‿0‿0⊸/) ↕⋈3 %% ⟨⋈0,⋈1,⋈2,⋈0⟩

# <
⊢⌾< 4 %% 4
(<5)⌾< 4 %% 5
{<"cd"}⌾< "ab" %% "cd"
!"<⁼𝕩: Argument wasn't a rank 0 array" % 4⌾< "ab"
!"<⁼𝕩: Argument wasn't a rank 0 array" % {⇐}⌾< "ab"
!"<⁼𝕩: Argument wasn't a rank 0 array" % ≍⌾< 2

# ⋈
⊢⌾⋈ 4 %% 4
(⋈5)⌾⋈ 4 %% 5
{⋈"cd"}⌾⋈ "ab" %% "cd"
!"⋈⁼𝕩: Argument wasn't a length-1 list" % 4⌾⋈ "ab"
!"⋈⁼𝕩: Argument wasn't a length-1 list" % {⇐}⌾⋈ "ab"
!"⋈⁼𝕩: Argument wasn't a length-1 list" % ≍⌾⋈ 2
!"⋈⁼𝕩: Argument wasn't a length-1 list" % (<4)⌾⋈ 2

# k⊸⌽
!"𝕨⌽⁼𝕩: 𝕩 cannot be an atom" % 1⌾(2⊸⌽) ↕10
⊏⌾(2⊸⌽) 10‿10⥊↕100 %% 28‿29‿20‿21‿22‿23‿24‿25‿26‿27
≍⌾(1‿2⊸⌽) 5‿5⥊↕25 %% 1‿5‿5⥊22‿23‿24‿20‿21‿2‿3‿4‿0‿1‿7‿8‿9‿5‿6‿12‿13‿14‿10‿11‿17‿18‿19‿15‿16

# NaN things
⟨@⋈0÷0, @⋈-0÷0⟩⌾(0‿0⊸⊏) ⋈3 %% ⋈@ ⋈ 0÷0
(
  %USE eqvar
  _test_ ← { set Repl _𝕣_ different exp:
    # •Show ⟨"test", set, Repl, exp⟩
    bad ⇐ {⇐}
    _res ← {different? "ok"∘𝔽; 𝔽}
    _mod ← {exp? 𝔽; {"Expected error!"!bad≢𝕩⋄𝕩}∘(bad∘𝔽⎊{𝕊: •CurrentError@})}
    
    HasMsg ← {⟨"mismatched result/error message", exp, 𝕨, 𝕩⟩ ! (¬exp) ≡ 𝕨≡𝕩}
    
    exp {
      @𝕊·: @;
      𝕨𝕊0:
        (1‿0‿1‿1∾6⥊0) {Repl⌾(𝕨⊸/) 𝕩}_res _eqvar 10⥊0‿1
        "𝔽⌾(a⊸/): Incompatible result elements"               HasMsg (1‿0‿2∾7⥊0) {    Repl ⌾(𝕨⊸/) 𝕩}_res _mod _eqvar 10  ⥊0‿1
        "⌾: Incompatible result elements in structural Under" HasMsg (1‿0‿2∾7⥊0) {(3⥊˘Repl)⌾(𝕨⊸/) 𝕩}_res _mod _eqvar 10‿3⥊0‿1
        
        1‿2‿5 {Repl⌾(𝕨⊸⊏) 𝕩}_res _eqvar 10⥊0‿1
        "𝔽⌾(a⊸⊏): Incompatible result elements" HasMsg 1‿4‿4 {    Repl ⌾(𝕨⊸⊏) 𝕩}_res _mod _eqvar 10  ⥊0‿1
        "𝔽⌾(a⊸⊏): Incompatible result elements" HasMsg 1‿4‿4 {(3⥊˘Repl)⌾(𝕨⊸⊏) 𝕩}_res _mod _eqvar 10‿3⥊0‿1
      ;
      𝕨𝕊1:
        "⌾: Incompatible result elements in structural Under" HasMsg 1‿2‿2 {Repl⌾⊑⌾(𝕨⊸⊏¨) ⋈𝕩}_res _mod 0‿1‿0
    } set
  }
  Batch ← { different‿expNative‿expRuntime‿Get:
    val ← Get@
    { 𝕊 rt:
      # •Show ⟨"batch", get, val, rt⟩
      expNew‿expSame ← {rt? expRuntime; expNative}
      rt (Get¨     ) _test_ different expNew
      rt (Get¨   ↕3) _test_ different expNew
      rt {𝕊: 3⥊<val} _test_ 0 expSame
      rt (   3⥊<val) _test_ 0 expSame
      rt (val˙¨    ) _test_ 0 expSame
    }¨ 0‿1
  }
  ShouldNotBeCalled ← { 𝕊:
    ! •Show "I should not be called!"
  }
  Batch¨ ⟨
    ⟨0, 1‿1, 1‿1, 0÷0⟩
    ⟨0, 1‿1, 1‿1, <0÷0⟩
    ⟨0, 1‿1, 1‿1, ⟨3,0÷0,4⟩⟩
    ⟨0, 1‿1, 1‿1, {𝕊: ShouldNotBeCalled}⟩
    ⟨0, 1‿1, 1‿1, ShouldNotBeCalled˙⟩
    ⟨0, 1‿1, 1‿1, {𝕊: ShouldNotBeCalled⟜(0÷0)}⟩
    ⟨0, 1‿1, 1‿1, ShouldNotBeCalled⟜(0÷0)˙⟩
    ⟨0, 1‿1, 1‿1, ∞⟩
    ⟨1, 0‿1, 0‿1, {𝕊: 4⊸{𝕩}}⟩
    ⟨1, 0‿1, 0‿1, {𝕊: (0÷0)⊸{𝕩}}⟩
  ⟩
)
