%DEF var V←•internal.Variation ⋄ LV←•internal.ListVariations ⋄ CLR←•internal.ClearRefs
%DEF tvar %USE var ⋄ _tvar ← {F _𝕣 x: (CLR@) ⊢ {F 𝕩 V x}¨ LV 𝕩; w F _𝕣 x: (CLR@) ⊢ (LV 𝕨) {(𝕨 V w) F 𝕩 V x}⌜ LV 𝕩}
%DEF eqvar ⟨_eqvar⟩←⊑•args
%DEF eqvarv ⟨_eqvarv_⟩←⊑•args
%DEF evar %USE tvar ⋄ _evar ← {ok←{⇐} ⋄ r←⥊𝕨 ok∘𝔽⎊{𝕊: •CurrentError@} _tvar 𝕩 ⋄ {⊑ok∊r? !"TEST FAIL: variation didn't error"; ("TEST FAIL: varying error messages"⊸⋈ ! 1=≠)⍷r ⋄ (⊑r)!0}}
%DEF eqerr %USE eqvar ⋄ _eqerr ← {ok←{⇐} ⋄ r←⥊𝕨 ok∘𝔽⎊{𝕊: •CurrentError@}⌜ 𝕩 ⋄ {⊑ok∊r? !"TEST FAIL: case didn't error"; ("TEST FAIL: varying error messages"⊸⋈ ! 1=≠)⍷r ⋄ (⊑r)!0}}
%DEF eqall _eqall0 ← {r←⊑𝕩 ⋄ ("TEST FAIL: eqall failed" ! r⊸𝔽)¨ 𝕩 ⋄ r} ⋄ EqAll ← •internal.Indistinguishable _eqall0
%DEF k _k ← {𝔽○•internal.Keep}
%DEF rand r ← (⊑•args).GetRand@

# batch checks
{>0‿1‿3‿'a'‿{⇐} (≠𝕏`)⎊'e'⌜ ⟨10⥊0, 10⥊10, 10⥊1e4, 10⥊1e9, 10⥊1e20, "AAA", "⍉⍉⍉", "𝕨𝕨𝕨"⟩}¨ ⌊‿⌈‿+‿≠‿<  %%  ⟨5‿8⥊10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e',5‿8⥊10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e',5‿8⥊10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿10‿10‿10‿10‿10‿'e'‿'e'‿'e'‿10‿10‿10‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e',5‿8⥊10‿10‿10‿10‿10‿3‿3‿3‿10‿10‿10‿10‿10‿3‿3‿3‿10‿10‿10‿10‿10‿3‿3‿3‿10‿10‿10‿10‿10‿3‿3‿3‿10‿10‿10‿10‿10‿3‿3‿3,5‿8⥊10‿10‿10‿10‿10‿3‿3‿3‿10‿10‿10‿10‿10‿3‿3‿3‿10‿10‿10‿10‿10‿3‿3‿3‿10‿10‿10‿10‿10‿3‿3‿3‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'‿'e'⟩
⟨⟨⟩, ↕0, ↕0‿0, ""⟩ {{⇐} 𝕏 𝕨 ⋄ 𝕨 𝕏 {⇐}}⌜ +‿-‿×‿÷‿⋆‿√‿⌊‿⌈‿|‿¬‿∧‿∨‿<‿>‿≠‿=

# total array comparison
%USE k ⋄ a ← 0‿@‿'a'‿∞‿¯∞‿1‿¯1∾⍷∾{⟨𝕩⥊0, 𝕩⥊1, {1⌾((¯1¨≢𝕩)⊸⊑)𝕩}⍟(0≠×´∘≢) 𝕩⥊0⟩}¨ ∾{⥊(<⟨⟩)∾⌜´𝕩⥊<↕3}¨↕4 ⋄ r←(⊑(⍋-⍒)∘⋈)_k⌜˜ a ⋄ ! r ≡ •Cmp⌜˜a ⋄ r %% '1'-˜73‿73⥊"1000202002000002222000002000000222222222222200000200000022220000002000000210222222222222222222222222222222222222222222222222222222222222222222222222122222222222222222222222222222222222222222222222222222222222222222222222001222222222222222222222222222222222222222222222222222222222222222222222000010000200000222200000200000022222222222220000020000002222000000200000020002122022020222222020222022022222222222222202022202202222220220222022020000201002000002222000002000000222222222222200000200000022220000002000000200020210200000222200000200000022222222222220000020000002222000000200000020002222122020222222020222022022222222222222202022202202222220220222022020000000001000000000000000000000000000000000000000000000000000000000000000200020220210000222200000200000022222222222220000020000002222000000200000020002222222120222222020222022022222222222222202022202202222220220222022022000202202201002222200002202000222222222222220000220200022222020002202000200022222222212222222202222220222222222222222220222222022222222202222220220002022022020122222020022022022222222222222202002202202222220220222022020000000002000001000000000000000000000000000000000000000000000000000000000000000000200000210200000200000020020020020020000020000002002000000200000000000000020000022120000020000002202202202202000002000000220200000020000000000000002000002001000000000000200000000200000000000000020000000000000000200020220220000222210000200000022222222222220000020000002222000000200000020002222222220222222120222022022222222222222202022202202222220220222022022000202202202002222201002202000222222222222220000220200022222020002202000200022222222222222222212222220222222222222222220222222022222222202222220220002022022020222222020122022022222222222222202002202202222220220222022020000000002000002002000001000000200200000200200000000000020020000000000000200020220220000222220000210000022222222222222000020000002222202000200000020002222222220222222220222122022222222222222222022202202222222220222022022000202202200002222200002201000222222222222220000220000022222020002202000200020220220200222220200220210022222222222222020022020002222202202220200020002222222222222222222222222122222222222222222222222202222222222222222022000202202202002222202002202201222222222222220200220220022222022022202202000000000200000200000000000000010000000000000000000000000000000000000000000000000020000022020000020000002102002002002000002000000200200000020000000000000002000002222000002000000221220220220200000200000022020000002000000000000000200000200200000000000020010000020000000000000002000000000000000000000000020000022020000020000002202102002202000002000000220200000020000000000000002000002222000002000000222221220222200000200000022220000002000000000000000200000200200000200000020020010020020000000000002002000000000000000000000020000022020000020000002202202102202000002000000220200000020000000000000002000002222000002000000222222221222200000200000022220000002000000000000000200000200000000000000020000000010000000000000000000000000000000000000000020000022020000020000002202002002102000002000000200200000020000000000000002000002222000002000000222220220221200000200000022020000002000000000000000200000200200000000000020020000020010000000000002000000000000000020002022022000022222000020000002222222222222100002000000222200000020000002000222222222022222222022202202222222222222221202220220222222022022202202200020220220200222220200220200022222222222222010022020002222202000220200020002222222222222222222222222022222222222222222122222202222222220222222022000202202202022222202022202202222222222222220201220220222222022022202202000000000200000200200000200000020020020020020000010000002002000000000000020002022022000022222000022000002222222222222200002100000222220200020000002000222222222022222222022222202222222222222222202221220222222222022202202200020220220000222220000220200022222222222222000022010002222202000220200020002022022020022222020022022002222222222222202002202100222220220222020002000222222222222222222222222222222222222222222222222221222222222222222202200020220220200222220200220220222222222222222020022022012222202202220220200000000020000020000000000000002000000002000000000000000100000000000000000000000002000002202000002000000220200200220200000200000021020000002000000000000000200000222200000200000022222022022220000020000002212000000200000000000000020000020020000000000002002000002002000000000000200100000000000002000202202200002222200002000000222222222222220000200000022221000002000000200022222222202222222202220220222222222222222220222022022222212202220220220002022022000022222000020000002222222222222200002000000222220100020000002000202202202002222202002202000222222222222220200220200022222021002202000200022222222222222222222222220222222222222222222222222022222222212222220220002022022020022222020022020002222222222222202002202000222220220122020000000000002000002002000002000000200200200200200000200000020020000001000000200020220220000222220000220000022222222222222000022000002222202000210000020002222222220222222220222222022222222222222222022222202222222220222122022000202202200002222200002200000222222222222220000220000022222020002201000200020220220200222220200220220022222222222222020022022002222202202220210020002222222222222222222222222222222222222222222222222222222222222222222122000202202202002222202002202200222222222222220200220220022222022022202201"
!"∧𝕩: 𝕩 cannot have rank 0" % ∧@
!"∨𝕩: 𝕩 cannot have rank 0" % ∨@
(=⋈≡)˜{⇐} %% 1‿1
(≠⋈≢)˜{⇐} %% 0‿0
{⇐}(=⋈≡){⇐} %% 0‿0
{⇐}(≠⋈≢){⇐} %% 1‿1
{2⊸+}=⟨2⊸+, 2⊸-, 3⊸+⟩ %% 1‿0‿0
{-+}=⟨-+, ++, --⟩ %% 1‿0‿0
{+¨}=⟨+¨, -¨, +⌜⟩ %% 1‿0‿0
{+∘-}=⟨+∘-, +∘+, -∘-, +⟜-⟩ %% 1‿0‿0‿0
{•Type}=⟨•Type,•BQN"•Type"⟩ %% 1‿1
{•FChars}=⟨•FChars,•BQN"•FChars"⟩ %% 1‿0
=⌜˜ ⟨+, ⁼, ⌾, {𝕩}, {𝔽𝕩}, {𝔽𝔾}, •internal.PureKeep, •internal.Keep, •Type, •FChars, •_timed, •_while_, 2⊸+, +-, {⇐}⟩ %% 15‿15⥊1∾15⥊0
≡⌜˜ ⟨(0÷0)⊸+, (0÷0)⊸+⟩ %% [1‿1, 1‿1]
=⌜˜ ⟨(0÷0)⊸+, (0÷0)⊸+⟩ %% [1‿1, 1‿1]
≠⍷ ⟨(0÷0)⊸+, 2, 2, 3, (0÷0)⊸+⟩ %% 3
⊐  ⟨(0÷0)⊸+, 2, 2, 3, (0÷0)⊸+⟩ %% 0‿1‿1‿2‿0
⊒  ⟨(0÷0)⊸+, 2, 2, 3, (0÷0)⊸+⟩ %% 0‿0‿1‿0‿1

# comparison
w←1‿1⥊1 ⋄ x←1‿2⥊1           ⋄ {! (∾⟨"𝕨",•Repr𝕩,"𝕩: Expected equal shape prefix (1‿1 ≡ ≢𝕨, 1‿2 ≡ ≢𝕩)"⟩) ≡ w 𝕏⎊(•CurrentError∘@) x}¨ =‿≠‿<‿>‿≤‿≥
w←1‿1⥊1 ⋄ x←1‿2⥊1 ⋄ w‿x <¨↩ ⋄ {! (∾⟨"𝕨",•Repr𝕩,"𝕩: Expected equal shape prefix (1‿1 ≡ ≢𝕨, 1‿2 ≡ ≢𝕩)"⟩) ≡ w 𝕏⎊(•CurrentError∘@) x}¨ =‿≠‿<‿>‿≤‿≥
w←1‿1⥊1 ⋄ x←2⥊1             ⋄ {! (∾⟨"𝕨",•Repr𝕩,"𝕩: Expected equal shape prefix (1‿1 ≡ ≢𝕨, ⟨2⟩ ≡ ≢𝕩)"⟩) ≡ w 𝕏⎊(•CurrentError∘@) x}¨ =‿≠‿<‿>‿≤‿≥
w←1‿1⥊1 ⋄ x←2⥊1 ⋄ w‿x <¨↩   ⋄ {! (∾⟨"𝕨",•Repr𝕩,"𝕩: Expected equal shape prefix (1‿1 ≡ ≢𝕨, ⟨2⟩ ≡ ≢𝕩)"⟩) ≡ w 𝕏⎊(•CurrentError∘@) x}¨ =‿≠‿<‿>‿≤‿≥
!"Invalid comparison" % <˜{⇐}
!"Invalid comparison" % ≤˜{⇐}
!"Invalid comparison" % >˜{⇐}
!"Invalid comparison" % ≥˜{⇐}
{! 5 (𝕏 ≡ 𝕏⌾⋈) ↕10}¨ =‿≠‿<‿>‿≤‿≥
{! (↕10) (𝕏 ≡ 𝕏⌾⋈) 5}¨ =‿≠‿<‿>‿≤‿≥
{! (↕10) (𝕏 ≡ 𝕏⌾⋈) 4⌊↕10}¨ =‿≠‿<‿>‿≤‿≥

# !𝕩 & 𝕨!𝕩
!"Assertion error" % !0
!"Assertion error" % !0
!"foo" % !"foo"
!"foo" % "foo"!0
!"foo" % "foo"!"bar"
!⎊•CurrentError 1‿+ %% ⟨1,+⟩
!⟜0⎊•CurrentError 1‿+ %% ⟨1,+⟩
!1 %% 1
"foo"!1 %% 1


# |𝕩
{| (↕10)∾𝕩}¨ -2⋆7‿15‿31 %% ⟨0‿1‿2‿3‿4‿5‿6‿7‿8‿9‿128,0‿1‿2‿3‿4‿5‿6‿7‿8‿9‿32768,0‿1‿2‿3‿4‿5‿6‿7‿8‿9‿2147483648⟩

# arith
(<1)+ 1 %% <2
  1 +<1 %% <2
(<1)+<1 %% <2
@-@ %% 0
!"𝕨-𝕩: Invalid character" % @-1
{r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ @ 0∘𝕏⎊{𝕊: •CurrentError@} @}¨ ⟨+,    ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩
{r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ 0 0∘𝕏⎊{𝕊: •CurrentError@} @}¨ ⟨   -, ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩
{r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ @ 0∘𝕏⎊{𝕊: •CurrentError@} 0}¨ ⟨      ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩

%USE var ⋄ a←4⥊@ ⋄ (LV a) {r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ @ 0∘𝕏⎊{𝕊: •CurrentError@} 𝕨 V a}⌜ ⟨+,    ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩
%USE var ⋄ a←4⥊@ ⋄ (LV a) {r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ 0 0∘𝕏⎊{𝕊: •CurrentError@} 𝕨 V a}⌜ ⟨   -, ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩
%USE var ⋄ a←4⥊0 ⋄ (LV a) {r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ @ 0∘𝕏⎊{𝕊: •CurrentError@} 𝕨 V a}⌜ ⟨      ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩

%USE var ⋄ a←4⥊@ ⋄ (LV a) {r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ (𝕨 V a) 0∘𝕏⎊{𝕊: •CurrentError@} @}⌜ ⟨+,    ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩
%USE var ⋄ a←4⥊0 ⋄ (LV a) {r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ (𝕨 V a) 0∘𝕏⎊{𝕊: •CurrentError@} @}⌜ ⟨   -, ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩
%USE var ⋄ a←4⥊@ ⋄ (LV a) {r←•Repr𝕩 ⋄ (⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ (𝕨 V a) 0∘𝕏⎊{𝕊: •CurrentError@} 0}⌜ ⟨      ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩

%USE var ⋄ a←4⥊@ ⋄ b←4⥊@ ⋄ {r←•Repr f←𝕩 ⋄ {(⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ (𝕨 V a) 0∘F⎊{𝕊: •CurrentError@} 𝕩 V b}⌜○LV´ a‿b}⌜ ⟨+,    ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩
%USE var ⋄ a←4⥊0 ⋄ b←4⥊@ ⋄ {r←•Repr f←𝕩 ⋄ {(⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ (𝕨 V a) 0∘F⎊{𝕊: •CurrentError@} 𝕩 V b}⌜○LV´ a‿b}⌜ ⟨   -, ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩
%USE var ⋄ a←4⥊@ ⋄ b←4⥊0 ⋄ {r←•Repr f←𝕩 ⋄ {(⊢!≡´) ("𝕨"∾r∾"𝕩: Unexpected argument types") ⋈ (𝕨 V a) 0∘F⎊{𝕊: •CurrentError@} 𝕩 V b}⌜○LV´ a‿b}⌜ ⟨      ×, ÷, ⋆, √, ⌊, ⌈, |, ∧, ∨, ÷⟩

!"𝕨-𝕩: Unexpected argument types" % 0-@
!"𝕨÷𝕩: Unexpected argument types" % 0÷@
!"+𝕩: Argument must consist of numbers" % +@
⟨@, "abc", ⟨1,2,+⟩, {⇐}⟩ {(⊢!≡´) ("𝕩: 𝕩 contained non-number"∾˜•Repr 𝕩) ⋈ 0∘𝕏⎊{𝕊: •CurrentError@} 𝕨}⌜ -‿×‿÷‿⋆‿√‿⌊‿⌈‿|‿¬
!"This function can't be called monadically" % ≤@
!"This function can't be called monadically" % ≥@

# 𝕨/𝕩
2‿3‿0‿1/4‿3⥊↕⋈12 %% ⋈¨6‿3⥊0‿1‿2‿0‿1‿2‿3‿4‿5‿3‿4‿5‿3‿4‿5‿9‿10‿11
2‿3‿0‿1/↕4 %% 6⥊0‿0‿1‿1‿1‿3
2‿3‿0‿1/↕⋈4 %% ⋈¨6⥊0‿0‿1‿1‿1‿3
""/↕3 %% ↕3
!"𝕨/𝕩: Length of compound 𝕨 must be at most rank of 𝕩" % "abc"/↕3
!"𝕨/𝕩: Length of compound 𝕨 must be at most rank of 𝕩" % 1‿'a'/↕2
!"𝕨/𝕩: 𝕨 must consist of natural numbers" % "ab"/2‿2⥊9
!"Expected non-negative integer, got ¯3" % 2‿¯3/↕2
!"Expected non-negative integer, got ¯3e20" % 2‿¯3e20/↕2
!"𝕨/𝕩: Lengths of components of 𝕨 must match 𝕩 (3 ≠ 4)" % 1‿2‿3/4‿5‿6‿7
!"Expected non-negative integer, got ¯1000" % (4/1000‿¯1000) / 8⥊1
!"Expected non-negative integer, got ¯1000" % (4/1000‿¯1000) / 8⥊1
!"Integer out of range: 4e19" % %USE evar ⋄ 2‿4e19 /_evar ↕2

!"𝕨/𝕩: 𝕩 must have rank at least 1 for simple 𝕨" % ⟨↕1⟩/3
!"𝕨/𝕩: 𝕩 must have rank at least 1 for simple 𝕨" % ⟨↕1⟩/<"ab"
!"𝕨/𝕩: 𝕩 must have rank at least 1 for simple 𝕨" % (<↕1)/<"ab"
!"𝕨/𝕩: Length of compound 𝕨 must be at most rank of 𝕩" % (<↕0)/<"ab"
!"𝕨/𝕩: Length of compound 𝕨 must be at most rank of 𝕩" % ⟨↕10,↕10⟩ / 10⥊10
!"𝕨/𝕩: Length of compound 𝕨 must be at most rank of 𝕩" % ⟨↕10,↕10,<0⟩ / 10‿10⥊10
!"𝕨/𝕩: Simple 𝕨 must have rank 0 or 1 (2≡=𝕨)" % (2‿1⥊↕4) / 4‿4⥊↕16
!"𝕨/𝕩: Compound 𝕨 must have rank at most 1" % (2‿1⥊<↕4) / 4‿4⥊↕16
!"𝕨/𝕩: Simple 𝕨 must have rank 0 or 1 (2≡=𝕨)" % (1‿2⥊↕4) / ↕4
!"𝕨/𝕩: Compound 𝕨 must have rank at most 1" % (1‿2⥊<↕4) / ↕4
!"𝕨/𝕩: 𝕨 must consist of natural numbers" % ⟨↕4, 1‿2‿3.5‿4⟩ / 4‿4⥊↕16
!"𝕨/𝕩: 𝕨 must consist of natural numbers" % ⟨↕4, 3.5⟩       / 4‿4⥊↕16
!"𝕨/𝕩: 𝕨 must consist of natural numbers" % (<@)/↕3
!"𝕨/𝕩: 𝕨 must consist of natural numbers" % (<{+})/↕3
!"𝕨/𝕩: 𝕨 cannot be negative" % (<¯2)/↕3
!"𝕨/𝕩: Lengths of components of 𝕨 must match 𝕩 (1 ≠ 3)" % ⟨¯2⟩/↕3
!"Expected non-negative integer, got ¯2" % ⟨¯2⟩/↕1
!"Out of memory" % ⟨1,2⋆62⟩ / 2‿2⥊<⋈0
!"Out of memory" % ⟨1,2⋆62⟩ / 2⥊<⋈0
!"Out of memory" % (1e13×↕1000) / ↕1000
!"Out of memory" % (16 ⥊ ¯1e9 + 2⋆60) / ↕16
!"Out of memory" % (4⥊2⋆60)/⟨4,2⋆8⟩⥊1
⟨↕4, 0,   ↕5⟩ / 4‿4‿5⥊@ %% 6‿0‿10⥊@
⟨↕4, 4⥊0, ↕5⟩ / 4‿4‿5⥊@ %% 6‿0‿10⥊@
1‿0‿2    / ⋈¨ ["ab","cd","ef"] %% ⋈¨ ["ab","ef","ef"]
(<1‿0‿2) / ⋈¨ ["ab","cd","ef"] %% ⋈¨ ["ab","ef","ef"]
(⋈1‿0‿2) / ⋈¨ ["ab","cd","ef"] %% ⋈¨ ["ab","ef","ef"]
1‿0‿0/3‿0‿2⥊0 %% 1‿0‿2⥊0
1‿0‿0/3‿0‿2⥊'a' %% 1‿0‿2⥊' '
1‿0‿0/3‿0‿2⥊<"a" %% 1‿0‿2⥊<" "
(10000⥊1‿0)/10000‿0‿2⥊0 %% 5000‿0‿2⥊0

# /𝕩
!"Expected non-negative integer, got ¯1000" % / 4/1000‿¯1000
!"/𝕩: 𝕩 must have rank 1 (3‿3 ≡ ≢𝕩)" % /↕3‿3
!"/𝕩: 𝕩 must have rank 1 (⟨⟩ ≡ ≢𝕩)" % /0
!"Expected non-negative integer, got ¯1" % %USE evar ⋄ /_evar ¯1⌾(100⊸⊑) 200⥊10
!"Sum too big" % %USE evar ⋄ /_evar 1e2⥊2⋆62
!"Integer out of range: 1e20" % %USE evar ⋄ /_evar 1e20⌾(100⊸⊑) 200⥊10
! (//⁼)⊸≡8‿15‿25‿32‿90‿101‿155‿165‿584‿594‿894‿912‿1312‿1328‿1350‿1367‿1391‿1407‿1469‿1486‿1559‿1566‿1576‿1582‿1592‿1599‿1609‿1616‿1626‿1635‿4086‿4093‿4114‿4122‿4141‿4149‿4169‿4177
/⁼1e4⥊1‿2 %% 0‿5000‿5000
!"Expected integer, got character" % /⁼ "hello"
!"/⁼𝕩: 𝕩 cannot contain negative numbers" % %USE evar ⋄ /⁼_evar (↕10) ∾ ¯2 ∾ 20+↕10
!"/⁼𝕩: 𝕩 must consist of natural numbers" % %USE evar ⋄ /⁼_evar (↕10) ∾ 2.2 ∾ 20+↕10
!"/⁼𝕩: 𝕩 must consist of natural numbers" % %USE evar ⋄ /⁼_evar (↕10) ∾ 1e20 ∾ 20+↕10

# 𝕨⊏𝕩
(
  vals ← ⟨0, 1, 2, 127, 128, 32767, 32768, ¯32767, ¯32768, ¯32769, ¯1+2⋆31, 2⋆31, @, @+1, @+255, @+256, @+1114111, {⇐}, 1‿2⟩
  e1 ← ⌈⌜˜ 0‿0‿1‿1‿2‿2‿3‿2‿2‿3‿3‿4‿8‿8‿8‿8‿8‿8‿8
  e2 ← (8-(=⌜˜<⟜5)×8-⌈⌜˜) 0‿0‿1‿1‿2‿2‿3‿2‿2‿3‿3‿4‿5‿5‿5‿6‿7‿8‿8
  a ← 100⥊↕2
  ⟨Variation, ListVariations, ElType⟩ ← •internal
  { 𝕊 v:
    ! e1 ≡ ElType¨ {(v Variation a) ⊏ 𝕨‿𝕩}⌜˜ vals
    ! e2 ≡ ElType¨ {(v Variation a) ⊏ 𝕨⋈𝕩}⌜˜ vals
  }¨ "i"ListVariations a
)
0‿0‿0 ⊏ 1‿3⥊↕10 %%% 3‿3⥊0‿1‿2
≢¨ ⟨(2‿0‿3‿4‿5⥊1)⊏9‿8‿0‿7⥊0 ⋄ ⟨⟩⊏0‿8‿0‿7⥊0 ⋄ (2‿3‿0‿4⥊1)⊏↕10⟩ %%% ⟨2‿0‿3‿4‿5‿8‿0‿7,0‿8‿0‿7,2‿3‿0‿4⟩
!"𝕨⊏𝕩: Indexing out-of-bounds (0∊𝕨, 0≡≠𝕩)" % %USE evar ⋄ 0‿0 ⊏_evar ↕0
!"𝕨⊏𝕩: Indexing out-of-bounds (0∊𝕨, 0≡≠𝕩)" % %USE evar ⋄ 0‿0 ⊏_evar 0‿2⥊1
!"𝕨⊏𝕩: Indexing out-of-bounds (0∊𝕨, 0≡≠𝕩)" % %USE evar ⋄ [0‿0,0‿0] ⊏_evar ↕0
!"𝕨⊏𝕩: Indexing out-of-bounds (0∊𝕨, 0≡≠𝕩)" % %USE evar ⋄ [0‿0,0‿0] ⊏_evar 0‿0⥊1
!"𝕨⊏𝕩: Indexing out-of-bounds (0∊𝕨, 0≡≠𝕩)" % %USE evar ⋄ ⟨0‿0⟩ ⊏_evar 0‿2⥊1
!"𝕨⊏𝕩: Indexing out-of-bounds (0∊𝕨, 0≡≠𝕩)" % %USE evar ⋄ ⟨0‿0⟩ ⊏_evar ↕0
!"𝕨⊏𝕩: Indexing out-of-bounds along axis 1 (0 ∊ 1⊑𝕨, 2‿0≡≢𝕩)" % %USE evar ⋄ ⟨0‿0,0‿0⟩ ⊏_evar 2‿0⥊1
!"𝕨⊏𝕩: Indexing out-of-bounds (0∊𝕨, 0≡≠𝕩)" % %USE evar ⋄ ⟨0⟩ ⊏_evar ↕0
!"𝕨⊏𝕩: Indexing out-of-bounds (0∊𝕨, 0≡≠𝕩)" % %USE evar ⋄ ⟨⋈0⟩ ⊏_evar ↕0
!"𝕨⊏𝕩: Indexing out-of-bounds (1∊𝕨, 1≡≠𝕩)" % ⊏˜(200⥊1)⥊1
!"𝕨⊏𝕩: Indexing out-of-bounds (24∊𝕨, 4≡≠𝕩)" % %USE evar ⋄ 1‿24 ⊏_evar ↕4‿4
!"𝕨⊏𝕩: Indexing out-of-bounds (¯24∊𝕨, 4≡≠𝕩)" % %USE evar ⋄ 1‿¯24 ⊏_evar ↕4‿4
!"𝕨⊏𝕩: Indexing out-of-bounds (¯26∊𝕨, 5≡≠𝕩)" % %USE evar ⋄ ¯26⊸⊏_evar ↕5
!"Expected integer, got ¯26.5" % %USE evar ⋄ ¯26.5⊸⊏_evar ↕5
!"𝕨⊏𝕩: Indexing out-of-bounds (¯26∊𝕨, 5≡≠𝕩)" % %USE evar ⋄ 1‿¯26 ⊏_evar ↕5
!"𝕨⊏𝕩: Indexing out-of-bounds (26∊𝕨, 5≡≠𝕩)" % %USE evar ⋄ 1‿26 ⊏_evar ↕5
!"Expected integer, got 1.1" % %USE evar ⋄ 0‿1.1‿2 ⊏_evar ↕10
!"𝕨⊏𝕩: Indexing out-of-bounds (1∊𝕨, 1≡≠𝕩)" % %USE evar ⋄ 0‿0‿1 ⊏_evar 1‿3⥊↕10000
!"𝕨⊏𝕩: Indexing out-of-bounds (2∊𝕨, 2≡≠𝕩)" % %USE evar ⋄ (1+9=↕10) ⊏_evar ⟨1‿2,3⟩
!"𝕨⊏𝕩: Indexing out-of-bounds (4∊𝕨, 3≡≠𝕩)" %  %USE evar ⋄   1‿4‿2‿0‿10‿5‿6  ⊏_evar 1‿0‿1
!"𝕨⊏𝕩: Indexing out-of-bounds (¯4∊𝕨, 3≡≠𝕩)" % %USE evar ⋄ (-1‿4‿2‿0‿10‿5‿6) ⊏_evar 1‿0‿1
!"𝕨⊏𝕩: Indexing out-of-bounds (4000∊𝕨, 1001≡≠𝕩)" %  %USE evar ⋄ ( 1000×1‿4‿2‿0‿10‿5‿6) ⊏_evar 1001⥊1‿0‿1
!"𝕨⊏𝕩: Indexing out-of-bounds (¯4000∊𝕨, 1001≡≠𝕩)" % %USE evar ⋄ (¯1000×1‿4‿2‿0‿10‿5‿6) ⊏_evar 1001⥊1‿0‿1
!"𝕨⊏𝕩: Indexing out-of-bounds (3∊𝕨, 2≡≠𝕩)" % %USE evar ⋄ (⌽↕4) ⋈⊸⊏_evar "ab"
!"𝕨⊏𝕩: 𝕩 cannot be a unit" % %USE eqerr ⋄ ⟨0, ⟨⟩⟩ ⊏_eqerr ⟨<"a", <5, <{⇐}⟩
!"𝕨⊏𝕩: 𝕩 cannot be an atom" % %USE eqerr ⋄ ⟨0, ⟨⟩⟩ ⊏_eqerr ⟨5, {⇐}⟩
!"Expected integer, got 1.5" % ⟨1.5⟩ ⊏ "ab"‿"cd"
!"𝕨⊏𝕩: Indexing out-of-bounds (10∊𝕨, 2≡≠𝕩)" % ⟨10⟩ ⊏ "ab"‿"cd"
%USE rand ⋄ {𝕩 ! ↕∘≠⊸⊏⊸≡ 20‿𝕩 r.Range 2}¨ (↕70)∾⥊(64×1‿2‿3‿4‿5)+⌜¯1‿0‿1‿30‿60
%USE eqvar ⋄ {1‿¯1 {r←𝕨⊏𝕩 ⋄ ! 𝕩 ≥○•internal.ElType r ⋄ r}_eqvar 𝕩}¨ ⟨↕2, ↕4, "hello", ⟨"ab", "cd", "ef"⟩, ↕⋈4⟩
%USE tvar ⋄ !∘≡¨⟜⊏ ∾ {⥊𝕨 ⊏⎊{𝕊:•CurrentError@} _tvar 𝕩}´¨ ⟨⟨0‿1, ⋈0⟩ ⋄ ⟨0‿1, "?"⟩ ⋄ ⟨0‿1, ≍0‿1⟩⟩
a←↕2 ⋄ ! "e" ≡ (↕10){b←a‿a‿a‿a‿a‿a‿a‿a‿a‿a ⋄ 𝕨⊏⎊"e"𝕩}⟨1‿2,3‿4⟩ ⋄ ! 0‿1 ≡ a

!"Expected integer, got character" % %USE eqerr ⋄ ⟨@, <@⟩ ⊏_eqerr ⟨↕10, 10‿10⥊↕100⟩
!"Expected integer, got namespace" % %USE eqerr ⋄ ⟨{⇐}, <{⇐}⟩ ⊏_eqerr ⟨↕10, 10‿10⥊↕100⟩
!"Expected integer, got namespace" % %USE eqerr ⋄ ⟨≍⋈{⇐}, ⋈{⇐}⟩ ⊏_eqerr ⟨↕10, 10‿10⥊↕100⟩
%USE eqvar ⋄ (⌽↕4) ⊏_eqvar 10×↕10 %%% 30‿20‿10‿0
%USE eqvar ⋄ (4⥊0) ⊏_eqvar 40‿30‿20 %%% 4⥊40
%USE eqvar ⋄ 1‿0‿0 ⊏_eqvar "ab"⌾(0‿1⊸⊏) ↕2 %%% 0 WF "baa"
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained character)"                % %USE evar ⋄ ⟨ 1,¯26,@⟩ ⊏_evar ↕5
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained both an array and number)" % %USE evar ⋄ ⟨⋈1,¯26,@⟩ ⊏_evar ↕5
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained both an array and number)" % %USE eqerr ⋄ evar ← {%USE evar ⋄ evar} ⋄ (⋈⟜⌽ ⟨1,<1⟩) ⊏_evar _eqerr ⟨10‿10⥊↕100⟩
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained function)"                 % %USE eqerr ⋄ evar ← {%USE evar ⋄ evar} ⋄ ⟨1.5‿{⋈}⟩ ⊏_evar _eqerr ⟨↕10, 10‿10⥊↕100⟩
!"𝕨⊏𝕩: Elements of compound 𝕨 must be arrays of numbers"                                     % %USE eqerr ⋄ evar ← {%USE evar ⋄ evar} ⋄ ⟨<<<4⟩ ⊏_evar _eqerr ⟨↕10, 10‿10⥊↕100⟩
!"𝕨⊏𝕩: Elements of compound 𝕨 must be arrays of numbers"                                     % %USE eqerr ⋄ evar ← {%USE evar ⋄ evar} ⋄ ⟨⋈⋈{⇐}⟩ ⊏_evar _eqerr ⟨↕10, 10‿10⥊↕100⟩
!"𝕨⊏𝕩: Elements of compound 𝕨 must be arrays of numbers"                                     % %USE eqerr ⋄ evar ← {%USE evar ⋄ evar} ⋄ ⟨⋈⋈⋈4⟩ ⊏_evar _eqerr ⟨↕10, 10‿10⥊↕100⟩
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained character)" % ⟨@,@,@⟩ ⊏ 3‿3⥊1
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained both an array and number)" % ⟨↕2,1⟩ ⊏ 10‿10⥊2
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained both an array and number)" % ⟨↕2,1,@⟩ ⊏ 3‿3⥊1
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained both an array and character)" % ⟨↕2,@,1⟩ ⊏ 3‿3⥊1
!"𝕨⊏𝕩: 𝕨 must be an array of numbers or list of such (𝕨 contained both an array and number)" % ⟨⋈99, 1⟩ ⊏ 10‿10⥊2
!"𝕨⊏𝕩: Compound 𝕨 must have rank at most 1 (1‿1 ≡ ≢𝕨)" % %USE eqerr ⋄ ⟨≍⋈↕5⟩ ⊏_eqerr ⟨↕10, 10‿10⥊↕100⟩
!"𝕨⊏𝕩: Compound 𝕨 must have rank at most 1 (2‿2 ≡ ≢𝕨)" % [⟨1,↕2⟩, ⟨3,4⟩] ⊏ 10‿10⥊2
!"𝕨⊏𝕩: Length of compound 𝕨 must be at most rank of 𝕩 (2 ≡ ≠𝕨, ⟨10⟩ ≡ ≢𝕩)" % ⟨⟨⟩,⟨⟩⟩ ⊏ ↕10
!"𝕨⊏𝕩: Length of compound 𝕨 must be at most rank of 𝕩 (2 ≡ ≠𝕨, ⟨10⟩ ≡ ≢𝕩)" % ⟨↕2,↕2⟩ ⊏ ↕10
!"𝕨⊏𝕩: Length of compound 𝕨 must be at most rank of 𝕩 (2 ≡ ≠𝕨, ⟨4⟩ ≡ ≢𝕩)" % ⟨⋈⋈2,⋈⋈2⟩ ⊏ ↕4
!"𝕨⊏𝕩: Indexing out-of-bounds (4∊𝕨, 4≡≠𝕩)" % ⟨⋈4⟩ ⊏ 4⥊1
!"𝕨⊏𝕩: Indexing out-of-bounds (¯5∊𝕨, 4≡≠𝕩)" % ⟨⋈¯5⟩ ⊏ 4⥊1
!"𝕨⊏𝕩: Indexing out-of-bounds along axis 0 (4 ∊ 0⊑𝕨, 4‿5≡≢𝕩)" % ⟨⋈4,⋈0⟩ ⊏ 4‿5⥊1
!"𝕨⊏𝕩: Indexing out-of-bounds along axis 1 (¯6 ∊ 1⊑𝕨, 4‿5≡≢𝕩)" % ⟨⋈0,⋈¯6⟩ ⊏ 4‿5⥊1
!"𝕨⊏𝕩: Bad index: 1.5 along axis 0" % ⟨⋈1.5,⋈¯6⟩ ⊏ 4‿5⥊1
⟨1‿¯1, 2‿2⥊3‿2‿¯3‿¯4⟩ ⊏ 2‿4‿3⥊↕24 %%% 2‿2‿2‿3⥊21‿22‿23‿18‿19‿20‿15‿16‿17‿12‿13‿14‿21‿22‿23‿18‿19‿20‿15‿16‿17‿12‿13‿14
⟨⟨⟩⟩⊏2‿2⥊↕4 %%% 0‿2⥊0
⟨⟨⟩,⟨⟩⟩⊏2‿2⥊↕4 %%% 0‿0⥊0
⟨⟨⟩,"",<2⟩⊏2‿2‿3⥊↕12 %%% 0‿0⥊0
⟨3‿4⥊1‿0‿1⟩ ⊏ 2‿3⥊↕6 %%% (3‿4‿3⥊↕3) + 3‿4⥊3‿0‿3
⟨⟨⟩, 0‿3⥊1, 4‿0‿2⥊2⟩ ⊏ 2‿2‿2‿3‿4⥊3 %%% 0‿0‿3‿4‿0‿2‿3‿4⥊0
⟨⟨⟩, 0‿3⥊1, 4‿0‿2⥊2, <0⟩ ⊏ 1‿1‿2‿3‿4⥊3 %%% 0‿0‿3‿4‿0‿2‿4⥊0
⟨2‿3, 2‿2⥊4‿1‿2‿¯3, <2⟩ ⊏ 5‿7‿3‿3‿2⥊↕999 %%% 2‿2‿2‿3‿2 ⥊ ⟨336, 282, 300, 336, 462, 408, 426, 462⟩ +⌜ ↕6
⟨<0,<1,<0⟩ ⊏ 2‿3‿2‿5⥊↕99 %%% 10‿11‿12‿13‿14
⟨<0,<1,<0,4‿3‿2⟩ ⊏ 2‿3‿2‿5⥊↕99 %%% 14‿13‿12
⟨<1,<2,<0,<4⟩ ⊏ 2‿3‿2‿5⥊↕99 %%% <54
!"𝕨⊏𝕩: Indexing out-of-bounds along axis 3 (4 ∊ 3⊑𝕨, 2‿2‿2‿3‿4≡≢𝕩)" % ⟨⟨⟩, 0‿3⥊1, 4‿0‿2⥊2, <4, ⟨⟩⟩ ⊏ 2‿2‿2‿3‿4⥊3
({(𝕩⥊1)⥊0}¨ 200‿55) ⊏ 2‿2  ⥊0 %%% (   255⥊1)⥊0
({(𝕩⥊1)⥊0}¨ 200‿54) ⊏ 2‿2‿4⥊0 %%% (4∾˜254⥊1)⥊0
!"𝕨⊏𝕩: Result rank too large" % ({(𝕩⥊1)⥊0}¨ 201‿55) ⊏ 2‿2⥊0
!"𝕨⊏𝕩: Result rank too large" % ({(𝕩⥊1)⥊0}¨ 201‿54) ⊏ 2‿2‿4⥊0
%USE eqvar ⋄ (⌽↕4) ⋈⊸⊏_eqvar "abcdefgh" %% "dcba"
%USE eqvar ⋄ ⟨4+↕5, 2+↕4⟩ ⊏_eqvar 10‿10⥊↕100 %%% 5‿4⥊42‿43‿44‿45‿52‿53‿54‿55‿62‿63‿64‿65‿72‿73‿74‿75‿82‿83‿84‿85
%USE eqvar ⋄ ⟨4+↕5, 2+↕4⟩ ⊏_eqvar 10‿10⥊↕100 %%% 5‿4⥊42‿43‿44‿45‿52‿53‿54‿55‿62‿63‿64‿65‿72‿73‿74‿75‿82‿83‿84‿85
%USE eqvar ⋄ ⟨4+↕2, 2+↕5⟩ ⊏_eqvar 10‿10‿3⥊↕300 %%% 2‿5‿3⥊126‿127‿128‿129‿130‿131‿132‿133‿134‿135‿136‿137‿138‿139‿140‿156‿157‿158‿159‿160‿161‿162‿163‿164‿165‿166‿167‿168‿169‿170
(<↕3) ⊏ "hello" %%% "hel"
(<<4) ⊏ "hello" %%% <'o'

# 𝕨⊑𝕩
!"𝕨⊑𝕩: 𝕨 contained a non-integer" % ⟨0.1⟩⊑↕3
!"𝕨⊑𝕩: 𝕨 contained a non-integer" % ⟨2⋆80⟩⊑↕3
!"𝕨⊑𝕩: Leaf arrays in 𝕨 must have rank 1 (element: (1‿2⥊1‿2))" % %USE evar ⋄ (≍1‿2) {⟨𝕨⟩⊑𝕩}_evar ↕5‿5
!"𝕨⊑𝕩: Leaf arrays in 𝕨 must have rank 1 (element: (<2))" % (<2)⊑↕3
!"𝕨⊑𝕩: Leaf arrays in 𝕨 must have rank 1 (element: (<1))" % (<1)⊑"foo"‿"bar"‿"baz"
%USE eqvar ⋄ 1‿2 ⊑_eqvar ↕5‿5 %% 1‿2

# >𝕩
!">𝕩: Result rank too large (200 ≡ =𝕩, 205 ≡ =⊑𝕩)" % > (200⥊1)⥊<(205⥊1)⥊1
!">𝕩: Incompatible element shapes (encountered shapes ⟨2⟩ and 4‿5)" % > ↕¨⟨2, 4‿5⟩
!">𝕩: Incompatible element shapes (encountered shapes 2‿3 and ⟨4⟩)" % > ↕¨⟨2‿3, 4⟩
!">𝕩: Incompatible element shapes (encountered shapes 2‿3 and 4‿5)" % > ↕¨⟨2‿3, 4‿5⟩
!">𝕩: Incompatible element shapes (encountered shapes 2‿0 and 4‿5)" % > ↕¨⟨2‿0, 4‿5⟩
!">𝕩: Incompatible element shapes (encountered shapes ⟨0⟩ and 4‿5)" % > ↕¨⟨0, 4‿5⟩
!">𝕩: Incompatible element shapes (encountered shapes ⟨0⟩ and ⟨⟩)" % > ↕¨⟨0, ⟨⟩⟩
!">𝕩: Incompatible element shapes (encountered shapes 2‿3 and ⟨⟩)" % > ↕¨⟨2‿3, ⟨⟩⟩
!">𝕩: Incompatible element shapes (encountered shapes 4‿5 and 4‿6)" % ≢>2‿3⥊⟨4‿5⥊↕20,4‿6⥊↕1⟩
>⟨<'a',1⟩ %% 'a'‿1
>⟨1,2,<3,4,<<5⟩ %% ⟨1,2,3,4,<5⟩
≢>0‿1⥊<2‿3⥊<"ab" %% ↕4
{a←↕3 ⋄ !'e'≡>⎊'e' ⟨a, a, ↕2, a, a⟩ ⋄ ! 1≡•internal.Refc a}
{a←↕3 ⋄      >     ⟨a, a, ↕3, a, a⟩ ⋄ ! 1≡•internal.Refc a}
{a←(200⥊1)⥊↕3 ⋄ ! 'e'≡>⎊'e' (200⥊1)⥊⟨a, a, ↕2, a, a⟩ ⋄ ! 1≡•internal.Refc a}
%USE eqvar ⋄ a←2‿3⥊↕6 ⋄ b←2‿3⥊"abcdef" ⋄ a {>1‿2‿1‿1⥊𝕨‿𝕩}_eqvar b %% 1‿2‿1‿1‿2‿3⥊0‿1‿2‿3‿4‿5‿'a'‿'b'‿'c'‿'d'‿'e'‿'f'
%USE eqvar ⋄ a←b←2‿3⥊<<˘"abcd"         ⋄ a {>1‿2‿1‿1⥊𝕨‿𝕩}_eqvar b %% 1‿2‿1‿1‿2‿3⥊<<¨"abcd"
%USE eqvar ⋄ {>1‿2‿1‿1⥊𝕨‿𝕩}_eqvar´¨ ⟨2⥊<2‿3⥊<<˘"abcd" ⋄ ⟨2‿3⥊↕6,2‿3⥊"abcdef"⟩⟩ %% ⟨1‿2‿1‿1‿2‿3⥊<<¨"abcd", 1‿2‿1‿1‿2‿3⥊0‿1‿2‿3‿4‿5‿'a'‿'b'‿'c'‿'d'‿'e'‿'f'⟩
%USE eqvar ⋄         {∾𝕨‿𝕩}_eqvar´¨ ⟨2⥊<2‿3⥊<<˘"abcd" ⋄ ⟨2‿3⥊↕6,2‿3⥊"abcdef"⟩⟩ %% ⟨        4‿3⥊<<¨"abcd",         4‿3⥊0‿1‿2‿3‿4‿5‿'a'‿'b'‿'c'‿'d'‿'e'‿'f'⟩

# <𝕩
! ⊑∘<¨⊸≡ ∾⟜- ⥊(¯2+↕5)+⌜2⋆↕40
! ⊑∘<¨⊸≡ @+⥊¯1‿0‿1+⌜2⋆↕20
! ⊑∘<⊸≡ {⇐}

# ⋈𝕩
! ⊑∘⋈¨⊸≡ ∾⟜- ⥊(¯2+↕5)+⌜2⋆↕40
! ⊑∘⋈¨⊸≡ @+⥊¯1‿0‿1+⌜2⋆↕20
! ⊑∘⋈⊸≡ {⇐}

# ∾𝕩
∾⟨"ab", 12, "de"⟩ %% ⟨'a','b',12,'d','e'⟩
!"∾𝕩: Item trailing shapes must be equal (contained arrays with shapes 3‿3 and 3‿4)" % ∾⟨↕3‿3,↕3‿4⟩
!"∾𝕩: Item trailing shapes must be equal (contained arrays with shapes ⟨2⟩ and 3‿3)" % ∾⟨1‿2,↕3‿3⟩
!"∾𝕩: Item ranks along an axis can differ by at most one (contained ranks 1 and 3 along axis 0)" % ∾2‿1 ⥊ ⥊⟜0¨ ⟨0‿2‿3, 0⟩
!"∾𝕩: Incompatible item shapes (contained arrays with shapes 1‿0 and 1‿0 along axis 1)" % ∾ {1‿𝕩⥊0}¨ [0‿1, 0‿0]
!"∾𝕩: Incompatible item ranks" % ∾ ⥊⟜0¨ [0‿1⋈0‿1, 0‿1⋈1]
!"∾𝕩: Item trailing shapes must be equal (contained arrays with shapes ⟨2⟩ and ⟨3⟩ and later higher-rank array)" % ∾⟨1‿1, 1‿1‿1, ⟨1⟩, 2‿2⥊1⟩
!"∾𝕩: Item trailing shapes must be equal (contained arrays with shapes 1‿4 and ⟨0⟩)" % ∾⟨1‿4⥊"abcd",⟨⟩⟩
∾3‿3⥊<1‿0‿3⥊0 %% 3‿0‿3⥊0
∾3‿3⥊<0‿0⥊⟨⟩ %% 0‿0⥊⟨⟩
⊑1↑⥊∾3‿3⥊<1‿0‿3⥊<"hi" %% "  "
!"∾𝕩: Empty array 𝕩 cannot have an atom fill element" % ∾↕0 %PROPER_FILLS
∾⟨⟩ %% ⟨⟩
!"∾𝕩: Item ranks in a list can differ by at most one (contained ranks 0 and 2)" % ∾⟨1,"ab",2‿2⥊'a'⟩
(
  rs ← 0‿1‿2‿3 ∾ 255-↕3
  n ← 2‿3 ∾ {(⊑•args).slow? 4; ↕0}
  as ← ⥊⟜1⍟2¨ rs
  {
    ranks ← 𝕩⊏rs
    got ← 1∘∾⎊{𝕊:
      ¬∧´0=ranks?
      m ← •CurrentError@
      fs ← •ParseFloat¨ ((¯1+⊢×·+`»⊸<) '0'⊸≤∧≤⟜'9')⊸⊔ m↓˜⋈⁼/"contained ranks" ⍷ m
      ranks ! 2=≠fs
      ranks ! 2≤|-´fs
      ranks ! fs ≡ ⊑∾{∨´m←2≤|𝕩-l←¯1⊑𝕩? ⋈⟨{⊑𝕩⊏˜⍒|𝕩-l} m/𝕩,l⟩; ⟨⟩}¨ 2↓↑ ranks
      0;0
    } 𝕩⊏as
    exp ← (∨´ranks≥1) ∧ 1≥|(⌈´-⌊´) ranks
    𝕩 ! got ≡ exp
  }¨ ∾{⥊↕𝕩⥊≠rs}¨ n
)

# 𝕨∾𝕩
!"𝕨∾𝕩: Lengths not matchable (⟨6⟩ ≡ ≢𝕨, 1‿1 ≡ ≢𝕩)" % ("abc"∾"def")∾≍"a"
!"𝕨∾𝕩: Lengths not matchable (⟨6⟩ ≡ ≢𝕨, 1‿1 ≡ ≢𝕩)" % (("abc")∾"def")∾1‿1⥊1

! (∘‿2⥊"abcdefgh") ≡ ["ab","cd"]∾["ef","gh"]
! (∘‿2⥊"abcdef") ≡ ["ab","cd"]∾"ef"
! (∘‿2⥊"abcdef") ≡ "ab"∾["cd","ef"]
! (∘‿2⥊"abcdefgh") ≡ (a←["ab","cd"])∾["ef","gh"] ⋄ •internal.Keep a
! (∘‿2⥊"abcdef") ≡ (a←["ab","cd"])∾"ef" ⋄ •internal.Keep a
! (∘‿2⥊"abcdef") ≡ "ab"∾a←["cd","ef"] ⋄ •internal.Keep a

!"𝕨∾𝕩: Argument ranks must differ by 1 or less (0≡=𝕨, 2≡=𝕩)" % 1∾↕3‿3
!"𝕨∾𝕩: Argument ranks must differ by 1 or less (1≡=𝕨, 3≡=𝕩)" % 1‿2∾↕3‿3‿3
!"𝕨∾𝕩: Argument ranks must differ by 1 or less (3≡=𝕨, 1≡=𝕩)" % 1‿2∾˜↕3‿3‿3
!"𝕨∾𝕩: Lengths not matchable (3‿4 ≡ ≢𝕨, 3‿3 ≡ ≢𝕩)" % (↕3‿4)∾↕3‿3

!"𝕨∾𝕩: Lengths not matchable (2‿2 ≡ ≢𝕨, 2‿3 ≡ ≢𝕩)" % ["ab","cd"]∾["efg","hjk"]
!"𝕨∾𝕩: Lengths not matchable (2‿2 ≡ ≢𝕨, ⟨3⟩ ≡ ≢𝕩)" % ["ab","cd"]∾"efg"
!"𝕨∾𝕩: Lengths not matchable (⟨3⟩ ≡ ≢𝕨, 2‿2 ≡ ≢𝕩)" % "abz"∾["cd","ef"]
!"𝕨∾𝕩: Lengths not matchable (2‿2 ≡ ≢𝕨, 2‿3 ≡ ≢𝕩)" % (a←["ab","cd"])∾["efg","hjk"]
!"𝕨∾𝕩: Lengths not matchable (2‿2 ≡ ≢𝕨, ⟨3⟩ ≡ ≢𝕩)" % (a←["ab","cd"])∾"efg"
!"𝕨∾𝕩: Lengths not matchable (⟨3⟩ ≡ ≢𝕨, 2‿2 ≡ ≢𝕩)" % "abz"∾a←["cd","ef"]

# 𝕨≍𝕩
!"𝕨≍𝕩: 𝕨 and 𝕩 must have equal shapes (3‿4 ≡ ≢𝕨, 3‿3 ≡ ≢𝕩)" % (↕3‿4)≍↕3‿3
!"𝕨≍𝕩: 𝕨 and 𝕩 must have equal shapes (⟨3⟩ ≡ ≢𝕨, ⟨2⟩ ≡ ≢𝕩)" % 1‿2‿3≍1‿2
!"𝕨≍𝕩: 𝕨 and 𝕩 must have equal shapes (⟨3⟩ ≡ ≢𝕨, ⟨⟩ ≡ ≢𝕩)" % 1‿2‿3≍1
!"𝕨≍𝕩: Result rank too large" % ≍˜(255⥊1)⥊1

# ≍𝕩
!"≍𝕩: Result rank too large (255≡=𝕩)" % ≍⍟256 1
%USE eqvar ⋄ 1‿2‿3‿4‿5‿64 {c𝕊e: ∾_eqvar´¨ ⟨4‿c, c⟩ ⋈⌜○({𝕩⥊<e}¨) ⟨0‿c, 1‿c, c⟩}⌜ ⟨1, 'a', 1‿2⟩

# « & »
!"𝕨»𝕩: Lengths not matchable (⟨2⟩ ≡ ≢𝕨, 3‿3 ≡ ≢𝕩)" % 1‿2»↕3‿3
!"𝕨»𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (0≡=𝕨, 3≡=𝕩)" % 1»↕2‿2‿2
!"𝕨»𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (2≡=𝕨, 1≡=𝕩)" % (↕2‿2)»1‿2
!"𝕨«𝕩: Lengths not matchable (⟨2⟩ ≡ ≢𝕨, 3‿3 ≡ ≢𝕩)" % 1‿2«↕3‿3
!"𝕨«𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (0≡=𝕨, 3≡=𝕩)" % 1«↕2‿2‿2
!"𝕨«𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (2≡=𝕨, 1≡=𝕩)" % (↕2‿2)«1‿2
!"«𝕩: 𝕩 cannot be a unit" % «0
!"»𝕩: 𝕩 cannot be a unit" % »<0

# ⍉𝕩
%USE rand ⋄ %USE eqvar ⋄ ⍉_eqvar 2‿3‿4 r.Range 2
%USE rand ⋄ %USE eqvar ⋄ ⍉_eqvar ⋈¨ 2‿3‿4 r.Range 2
%USE rand ⋄ %USE eqvar ⋄ ⍉_eqvar @+2‿3‿4 r.Range 100

# ⌽𝕩
%USE rand ⋄ %USE var ⋄ {{! 𝕩 ≡ ⌽"Ai8"V⌽𝕩} 𝕩 r.Range 2}¨ ↕1000
!"⌽𝕩: 𝕩 cannot be a unit" % ⌽ 2
!"⌽𝕩: 𝕩 cannot be a unit" % ⌽ <"foo"
!"⌽𝕩: 𝕩 cannot be a unit" % ⌽⍟2 <2
!"⌽𝕩: 𝕩 cannot be a unit" % ⌽⍟¯3 2
!"⌽𝕩: 𝕩 cannot be a unit" % ⌽⁼ 0

# 𝕨⌽𝕩
!"Integer out of range: 1e20" % %USE eqerr ⋄ ⟨1e20,⟨1e20⟩⟩ ⌽_eqerr ⟨↕10, ↕0, 0‿0⥊0, ⟨⟩⟩
!"Expected integer, got NaN" %  %USE eqerr ⋄ ⟨0÷0, ⟨0÷0⟩⟩  ⌽_eqerr ⟨↕10, ↕0, 0‿0⥊0, ⟨⟩⟩
!"Integer out of range: ∞" %    %USE eqerr ⋄ ⟨∞,   ⟨∞⟩⟩    ⌽_eqerr ⟨↕10, ↕0, 0‿0⥊0, ⟨⟩⟩
!"Expected integer, got function" %  %USE eqerr ⋄ ⟨⊑  ⟩ ⌽_eqerr ⟨↕10, ↕0, 0‿0⥊0, ⟨⟩⟩
!"Expected integer, got character" %  %USE eqerr ⋄ ⟨'a'⟩ ⌽ _eqerr ⟨↕10, ↕0, 0‿0⥊0, ⟨⟩⟩
!"𝕨⌽⁼𝕩: 𝕨 must consist of integers" % %USE eqerr ⋄ ⟨'a'⟩ ⌽⁼_eqerr ⟨↕10, ↕0, 0‿0⥊0, ⟨⟩⟩
!"𝕨⌽𝕩: 𝕨 contained non-number" %  %USE eqerr ⋄ ⟨3‿'a', 'a'‿3, 0‿'a', 'a'‿0⟩ ⌽ _eqerr ⟨0‿0⥊0,0‿0‿0⥊0⟩
!"𝕨⌽⁼𝕩: 𝕨 contained non-number" % %USE eqerr ⋄ ⟨3‿'a', 'a'‿3, 0‿'a', 'a'‿0⟩ ⌽⁼_eqerr ⟨0‿0⥊0,0‿0‿0⥊0⟩
!"Integer out of range: ∞" % %USE eqerr ⋄ ⟨3‿∞, ∞‿3, 0‿∞, ∞‿0⟩ ⌽_eqerr ⟨0‿0⥊0,0‿0‿0⥊0⟩
!"𝕨⌽𝕩: 𝕨 must have rank at most 1 (1‿1 ≡ ≢𝕨)" %  %USE eqerr ⋄ ⟨≍⋈1⟩ ⌽ _eqerr ⟨2‿2⥊1, ↕1⟩
!"𝕨⌽⁼𝕩: 𝕨 must have rank at most 1 (1‿1 ≡ ≢𝕨)" % %USE eqerr ⋄ ⟨≍⋈1⟩ ⌽⁼_eqerr ⟨2‿2⥊1, ↕1⟩
!"𝕨⌽𝕩: Length of list 𝕨 must be at most rank of 𝕩 (3 ≡ ≠𝕨, ⟨⟩ ≡ ≢𝕩⟩" % 1‿2‿3 ⌽ 1
!"𝕨⌽⁼𝕩: 𝕩 cannot be an atom" % 1‿2‿3 ⌽⁼ 1
!"𝕨⌽𝕩: Length of list 𝕨 must be at most rank of 𝕩 (3 ≡ ≠𝕨, ⟨3⟩ ≡ ≢𝕩⟩" % 1‿2‿3 ⌽ ↕3
!"𝕨⌽⁼𝕩: 𝕩 cannot be an atom" % ⟨⟩ ⌽⁼ 1
(<⁼ ⟨⟩⌽{a⇐1‿2}).a %% 1‿2
1‿2‿3 ⌽ 4‿4‿4‿4⥊↕256 %% F←{(4|𝕨+↕4)⊏𝕩} ⋄ 3 F⎉2 2 F˘ 1 F 4‿4‿4‿4⥊↕256
(1+1e15) ⌽ ↕10 %% (1+↕9)∾0
(-1+1e15) ⌽ ↕10 %% 9∾↕9
b←1↓1∾a←"hello" ⋄ b ⌽⎊'e' ⥊⟜1⍟2 5 ⋄ a ≡○•Hash b %% 1

# ↑𝕩 & ↓𝕩
!"↑𝕩: 𝕩 must have rank at least 1" % ↑0
!"↓𝕩: 𝕩 must have rank at least 1" % ↓<0

# 𝕨↓𝕩 & 𝕨↑𝕩
≠⍷•Hash¨ ⟨¯1↓•internal.Squeeze 1e9∾˜↕10000, ↕10000⟩ %% 1
≠⍷•Hash¨ ⟨10000↑•internal.Squeeze 1e9∾˜↕10000, ↕10000⟩ %% 1
(-2⋆63)↓3 %%% ↕0
(-2⋆63)↓"foo" %%% ""
(-2⋆63)↓↕3‿0‿2 %%% ↕0‿0‿2
""⌾((-2⋆63)⊸↓) ⟨⟩ %%% ⟨⟩

# ¨
%USE eqall ⋄ Eqall {𝕊: 1‿2}¨⌜˜ ⟨1, <1⟩ %% <1‿2
%USE eqall ⋄ Eqall      1‿2¨⌜˜ ⟨1, <1⟩ %% <1‿2
%USE eqall ⋄ Eqall ⟨1‿2˙,1‿2⟩ {𝕨¨˜ 𝕩}⌜ ⟨↕5, "hello", ↕⋈5⟩ %% 5⥊<1‿2
%USE eqall ⋄ Eqall ⟨{𝕊:1‿2}⟩  {𝕨¨˜ 𝕩}⌜ ⟨↕5, "hello", ↕⋈5⟩ %% 5⥊<1‿2
!"Mapping: Expected equal shape prefix (⟨2⟩ ≡ ≢𝕨, 4‿4 ≡ ≢𝕩)" % 1‿2+¨↕4‿4
!"Mapping: Expected equal shape prefix (⟨2⟩ ≡ ≢𝕨, ⟨4⟩ ≡ ≢𝕩)" % 1‿2+¨↕4
⊢¨ {a←𝕩 ⋄ ⥊𝕩} 2‿3⥊<"a"      %% 6⥊<"a"
⊢¨ {a←𝕩 ⋄ 2‿3‿1⥊𝕩} 2‿3⥊<"a" %% 2‿3‿1⥊<"a"
⊢¨ {a←𝕩 ⋄ 2‿3‿1⥊𝕩} 6⥊<"a"   %% 2‿3‿1⥊<"a"
⊢¨ {a←𝕩 ⋄ 2‿3⥊⥊𝕩} 2‿3⥊<"a"  %% 2‿3⥊<"a"
⊢¨ 2‿3⥊<"a"                 %% 2‿3⥊<"a"
%USE tvar ⋄ {a ← •internal.PureKeep¨_tvar 𝕩 ⋄ b ← ⊢¨_tvar 𝕩 ⋄ a •Repr∘𝕩 {𝔽⍟¬⊸! 𝕨 •internal.Indistinguishable 𝕩}¨ b}¨ ⟨⊔↕2, ⊔↕0, ↕5, ↕0, "hello", ""⟩

# ⊔
!"𝕨⊔𝕩: ≠𝕨 must be either ≠𝕩 or one bigger (2≡≠𝕨, 3≡≠𝕩)" % 0‿0⊔↕3
!"Out of memory" % ⟨∞⟩⊔"a"
!"Out of memory" % ⟨1e50⟩⊔"a"
{≢¨ (𝕩⥊¯1‿0) ⊔ 𝕩‿1‿2‿3⥊'z'}¨ 11×1+↕30 %% (⋈∾⟜1‿2‿3)¨ ⌊2÷˜11×1+↕30
!"𝕨⊔𝕩: 𝕨 can't contain elements less than ¯1" % %USE evar ⋄ 1‿2‿¯4   ⊔_evar 3⥊0
!"𝕨⊔𝕩: 𝕨 can't contain elements less than ¯1" % %USE evar ⋄ 1‿2‿¯4 ⋈⊸⊔_evar 3⥊0
!"𝕨⊔𝕩: ≠𝕨 must be either ≠𝕩 or one bigger (3≡≠𝕨, 4≡≠𝕩)" % %USE evar ⋄ (↕3)   ⊔_evar 4⥊0
!"𝕨⊔𝕩: ≠𝕨 must be either ≠𝕩 or one bigger (3≡≠𝕨, 4≡≠𝕩)" % %USE evar ⋄ (↕3) ⋈⊸⊔_evar 4⥊0
!"𝕨⊔𝕩: ≠𝕨 must be either ≠𝕩 or one bigger (4≡≠𝕨, 2≡≠𝕩)" % %USE evar ⋄ (↕4)   ⊔_evar 2‿2⥊0
!"𝕨⊔𝕩: ≠𝕨 must be either ≠𝕩 or one bigger (4≡≠𝕨, 2≡≠𝕩)" % %USE evar ⋄ (↕4) ⋈⊸⊔_evar 2‿2⥊0
!"⊔: Grouping argument must consist of integers" % %USE evar ⋄ 1‿2‿π ⋈⊸⊔_evar 3⥊0
!"⊔: Grouping argument must consist of integers" % %USE evar ⋄ 1‿2‿@ ⋈⊸⊔_evar 3⥊0
!"⊔: Grouping argument must consist of integers" % %USE evar ⋄ ⊔_evar 1‿2‿@
!"𝕨⊔𝕩: 𝕨 can't contain elements less than ¯1" % %USE evar ⋄ ⊔_evar 1‿2‿¯4

# ↕𝕩
!"Expected non-negative integer, got character" % ↕@
!"Expected non-negative integer, got character" % ↕"hi"
!"↕𝕩: 𝕩 must be either an integer or integer list (had rank 2)" % ↕2‿2⥊1
!"↕𝕩: Result rank too large (300≡≠𝕩)" % ↕300⥊1
!"↕𝕩: Result too large" % ↕20⥊10
!"Expected non-negative integer, got 3000.0000000000005" % ↕1e4×1.3-1
!"Expected non-negative integer, got ¯3000.0000000000005" % ↕-1e4×1.3-1
!"Expected non-negative integer, got ¯10000" % ↕-1e4
!"Out of memory" % ↕2e9 %USZ32
!"Out of memory" % ↕1e18 %USZ64
!"Integer out of range: 1e20" % ↕1e20
↕⟨⟩ %% <↕0
≢↕↕100 %% ↕100
↕⋈10 %% ⋈¨ ↕10
{! (⋈¨↕𝕩) ≡ ↕⋈𝕩}¨ 0‿1+⌜2⋆7‿15
{! ((<⟨⟩)<⊸∾⌜´ ↕¨ 𝕩) ≡ ↕𝕩}¨ 1+ (∾⟜-65‿64‿63‿3‿5) ↑⌜ ⟨↕0,1,1‿1⟩
{! ((<⟨⟩)<⊸∾⌜´ ↕¨ 𝕩) ≡ ↕𝕩}¨ ⌽¨⊸∾ ⥊ 0‿1‿2 ⋈⌜ (126+↕7) ∾ 0‿1 + 2⋆15

# 𝕨↕𝕩
!"𝕨↕𝕩: Length of 𝕨 must be at most rank of 𝕩" % 0↕0
!"𝕨↕𝕩: Length of 𝕨 must be at most rank of 𝕩" % 0‿0↕3⥊1
0‿0↕3‿3⥊1 %% 4‿4‿0‿0⥊0
1↕↕2‿3 %% 2‿1‿3⥊↕2‿3
0↕↕3 %% 4‿0⥊0

# ≡𝕩
≡¨ ⟨⟨⟩, ↕0, ""⟩ %% 1‿1‿1
≡↕0‿0‿0 %% 1
≡⟨1,1‿2,1⟩ %% 2
≡⋈⍟100 0 %% 100
≡↕10 %% 1
≡0 %% 0

# 𝕨⥊𝕩
!"𝕨⥊𝕩: Result rank too large (301 ≡ ≠𝕨)" % (0∾300⥊2)⥊0
!"Out of memory" % %USE eqerr ⋄ {(𝕩⥊2) ⥊ 1.1}¨_eqerr 63-↕10 %USZ64
!"Out of memory" % %USE eqerr ⋄ {(𝕩⥊2) ⥊ 7⥊1.1}¨_eqerr 63-↕10 %USZ64
1e8‿1e8‿⌊‿1e8‿1e8⥊↕5 %% 1e8‿1e8‿0‿1e8‿1e8⥊0 %USZ64
1e4‿1e4‿⌊‿1e4‿1e4⥊↕5 %% 1e4‿1e4‿0‿1e4‿1e4⥊0 %USZ32
⍷ {1e8‿1e8‿𝕩‿1e8‿1e8⥊↕0}¨ ∘‿⌽‿↑ %% ⋈ 1e8‿1e8‿0‿1e8‿1e8⥊0 %USZ64
⍷ {1e4‿1e4‿𝕩‿1e4‿1e4⥊↕0}¨ ∘‿⌽‿↑ %% ⋈ 1e4‿1e4‿0‿1e4‿1e4⥊0 %USZ32
!"𝕨⥊𝕩: 𝕨 too large" % %USE eqerr ⋄ {1e8‿1e8‿𝕩‿1e8‿1e8⥊"foo"}_eqerr ∘‿⌽‿↑

!"𝕨⥊𝕩: 𝕨 must be a list or unit (4 ≡ =𝕩)" % %USE eqerr ⋄ ⟨3‿1‿4‿1⥊2⟩ ⥊_eqerr ⟨↕10, ↕0, 4⟩
!"𝕨⥊𝕩: Empty 𝕩 and non-empty result" % %USE eqerr ⋄ ⟨10‿10, 1, ⟨⟩, ⋈1⟩ ⥊_eqerr ⟨↕0, "", 0‿0⥊0, 10‿0⥊"", 0‿10⥊<"hi"⟩
!"𝕨⥊𝕩: Empty 𝕩 and non-empty result" % %USE eqvar ⋄ (↕0) ⥊_eqvar ↕0
!"𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained namespace)" % %USE eqerr ⋄ ({⟨𝕩,⋈𝕩,1‿𝕩⟩} {⇐}) ⥊_eqerr ⟨↕0, 1, ↕10⟩
!"𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained function)"  % %USE eqerr ⋄ ({⟨𝕩,⋈𝕩,1‿𝕩⟩}{+¨}) ⥊_eqerr ⟨↕0, 1, ↕10⟩
!"𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained function)"  % %USE eqerr ⋄ ({⟨𝕩,⋈𝕩,1‿𝕩⟩}•out) ⥊_eqerr ⟨↕0, 1, ↕10⟩
!"𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained ¨)"         % %USE eqerr ⋄ ({⟨𝕩,⋈𝕩,1‿𝕩⟩} {¨}) ⥊_eqerr ⟨↕0, 1, ↕10⟩
!"𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained ∧)"         % %USE eqerr ⋄ ({⟨𝕩,⋈𝕩,1‿𝕩⟩} {∧}) ⥊_eqerr ⟨↕0, 1, ↕10⟩
!"𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained 1e40)"      % %USE eqerr ⋄ ({⟨𝕩,⋈𝕩,1‿𝕩⟩}1e40) ⥊_eqerr ⟨↕0, 1, ↕10⟩
!"𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained ¯10)"       % %USE eqerr ⋄ ({⟨𝕩,⋈𝕩,1‿𝕩⟩} ¯10) ⥊_eqerr ⟨↕0, 1, ↕10⟩
!"𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained 10.5)"      % %USE eqerr ⋄ ({⟨𝕩,⋈𝕩,1‿𝕩⟩}10.5) ⥊_eqerr ⟨↕0, 1, ↕10⟩
!"𝕨⥊𝕩: Shape must be exact when reshaping with ∘ (⟨42⟩ ≡ ≢𝕩, 10 is the product of non-computed axis)" % ∘‿2‿5 ⥊ ↕42
!"𝕨⥊𝕩: Can't compute axis when the rest of the shape is empty" % %USE eqerr ⋄ ∘‿⌽‿↑‿⌊ {4‿𝕨‿0 ⥊ 𝕩}_eqerr ⟨↕1, ↕0⟩
⍷ ⥊⟜⟨⟩¨ ∘‿⌊‿↑‿⌽ %% ⋈ ⟨⟩
⍷ ⥊⟜1¨ ∘‿⌊‿↑‿⌽ %% ⋈ ⟨1⟩
⍷ ⥊⟜"hello"¨ ∘‿⌊‿↑‿⌽ %% ⋈ "hello"
!"𝕨⥊𝕩: 𝕨 contained multiple computed axes" % ∘‿∘⥊1
!"𝕨⥊𝕩: 𝕨 contained multiple computed axes" % ∘‿∧⥊1

%USE eqvarv ⋄  ↑‿4 ⥊_eqvarv_"i" ↕9 %% [0‿1‿2‿3, 4‿5‿6‿7, 8‿0‿0‿0]
%USE eqvar  ⋄  ⌽‿4 ⥊_eqvar      ↕9 %% [0‿1‿2‿3, 4‿5‿6‿7, 8‿0‿1‿2]
%USE eqvar  ⋄  ⌊‿4 ⥊_eqvar      ↕9 %% [0‿1‿2‿3, 4‿5‿6‿7]
%USE eqvarv ⋄  ↑‿4 ⥊_eqvarv_"i" "abcde" %% ["abcd", "e   "]
%USE eqvar  ⋄  ⌽‿4 ⥊_eqvar      "abcde" %% ["abcd", "eabc"]
%USE eqvar  ⋄  ⌊‿4 ⥊_eqvar      "abcde" %% ["abcd"]
%USE eqvar  ⋄  2‿2 ⥊_eqvar ↕1000 %% [0‿1,2‿3]

# 𝕨⍷𝕩
%USE eqvar ⋄ "ab" ⍷_eqvar "" %% ⟨⟩
%USE eqvar ⋄ "" ⍷_eqvar "ab" %% 1‿1‿1
%USE eqvar ⋄ ⟨       ⟩ ⍷_eqvar ↕0 %% ⟨1⟩
%USE eqvar ⋄ ⟨{⇐}    ⟩ ⍷_eqvar ↕0 %% ⟨⟩
%USE eqvar ⋄ ⟨0, 1   ⟩ ⍷_eqvar ↕0 %% ⟨⟩
%USE eqvar ⋄ ⟨{⇐},{⇐}⟩ ⍷_eqvar ↕0 %% ⟨⟩
%USE eqvar ⋄ (↕0) ⍷_eqvar 1‿0 %% 1‿1‿1
%USE eqvar ⋄ "ab" ⍷_eqvar 0‿5⥊"ab" %% 0‿4⥊0
%USE eqvar ⋄ ⟨0÷0,1,0÷0⟩ ⍷_eqvar ⟨0÷0,1⟩⊏˜ "01010000010101010"-'0' %% 15⥊0
%USE eqvar ⋄ ⟨0÷0,1,0÷0⟩ ⍷_eqvar 4/≍⟨0÷0,1⟩⊏˜ "01010000010101010"-'0' %% 4‿15⥊0
%USE eqvar ⋄ [1⋈0÷0, ⋈˜0÷0] ⍷_eqvar 10‿10 ⥊ 1∾13⥊0÷0 %% 9‿9⥊0
(0⥊0)⍷1‿0⥊0 %% 1‿1⥊1
⟨⟩⍷1‿0⥊"" %% 1‿1⥊1
(0‿0‿0‿0‿0‿0⥊0)⍷1‿0‿0‿0‿0‿0‿0⥊0 %% 1‿1‿1‿1‿1‿1‿1⥊1
(0‿0‿0‿0‿0‿0⥊0)⍷3‿0‿0‿0‿3‿4‿0⥊0 %% 3‿1‿1‿1‿4‿5‿1⥊1
!"Out of memory" % (0‿0‿0‿0‿0‿0⥊0)⍷1e9‿0‿0‿1e9‿1e9‿0‿0⥊0
'a'‿1 ⍷ 0‿5⥊⟨⟩ %% 0‿4⥊0
"abc" ⍷ 5‿1⥊'a' %% 5‿0⥊0
⟨(<"ab")⍷<"aa", (<"ab")⍷<"ab"⟩ %% ⟨<0,<1⟩
{(=𝕨)≤=𝕩?𝕨⍷𝕩;'e'}⌜˜ ⟨'a','b',<'a','b',"a","b","aa","bb"⟩ %% (8‿8⥊⟨<1,<0,<1,<0,⟨1⟩,⟨0⟩,1‿1,0‿0,<0,<1,<0,<1,⟨0⟩,⟨1⟩,0‿0,1‿1,<1,<0,<1,<0,⟨1⟩,⟨0⟩,1‿1,0‿0,<0,<1,<0,<1,⟨0⟩,⟨1⟩,0‿0,1‿1,'e','e','e','e',⟨1⟩,⟨0⟩,1‿1,0‿0,'e','e','e','e',⟨0⟩,⟨1⟩,0‿0,1‿1,'e','e','e','e',⟨⟩,⟨⟩,⟨1⟩,⟨0⟩,'e','e','e','e',⟨⟩,⟨⟩,⟨0⟩,⟨1⟩⟩)
0∘⍷⎊1⌜˜ ⟨'a','b',<'a','b',"a","b","aa","bb",2‿2⥊'a'⟩ %% 9‿9⥊0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿0‿1‿1‿1‿1‿0‿0‿0‿0‿0‿1‿1‿1‿1‿0‿0‿0‿0‿0‿1‿1‿1‿1‿0‿0‿0‿0‿0‿1‿1‿1‿1‿0‿0‿0‿0‿0‿1‿1‿1‿1‿1‿1‿1‿1‿0
(5‿999‿0‿9‿10‿11‿12⥊"a") ⍷ 10‿20‿30‿0‿40‿50‿60‿70‿80‿10‿10‿10‿10⥊0 %% 10‿20‿30‿0‿40‿50‿56‿0‿81‿2‿1‿0‿0⥊0

# ⊐𝕩
⊐ (⊢∾¯1×⊢) 0‿1.5 %% 0‿1‿0‿2

# ⍷𝕩, ∊𝕩
≠⍷ 2 ⥊ 0÷0 %% 2
≠⍷ 200 ⥊ 0÷0 %% 200
∊ 2 ⥊ 0÷0 %% 1‿1
∊ 200 ⥊ 0÷0 %% 200⥊1
≠⍷ 2 ⥊ π %% 1
≠⍷ 200 ⥊ π %% 1
∊ 2 ⥊ π %% 1‿0
∊ 200 ⥊ π %% 1‿199/1‿0

⍷ 4‿0 ⥊ ⟨⟩ %% 1‿0 ⥊ ⟨⟩
∊ 4‿0 ⥊ ⟨⟩ %% 1‿0‿0‿0
⍷ 1000‿0 ⥊ 1 %% 1‿0 ⥊ 1
∊ 1000‿0 ⥊ 1 %% 1 ∾ 999⥊0
⍷ 0‿4 ⥊ 1 %% 0‿4⥊1

# 𝕨⊒𝕩
1‿2‿2‿3‿3‿2‿2‿1⊒1‿8‿5‿5‿6‿2‿5‿3‿1‿5‿1‿2‿4‿3‿4‿3‿1‿5‿2‿3‿4‿1‿5‿2 %% 0‿8‿8‿8‿8‿1‿8‿3‿7‿8‿8‿2‿8‿4‿8‿8‿8‿8‿5‿8‿8‿8‿8‿6

# 𝕨∊𝕩, 𝕨⊐𝕩, 𝕨⊒𝕩
{! ∧´(𝕨∊𝕩) =        𝕨≡○⊑𝕩}⌜˜          3‿100 ⥊⌜ "a⍉𝕩"∾1‿10‿1e4‿1e9‿1e12∾⟨"a",{⇐}⟩
{! ∧´(𝕨⊐𝕩) = (≠𝕨) × 𝕨≢○⊑𝕩}⌜˜          3‿100 ⥊⌜ "a⍉𝕩"∾1‿10‿1e4‿1e9‿1e12∾⟨"a",{⇐}⟩
{! ∧´(𝕨⊒𝕩) = ↕∘≠⊸⌊⍟(𝕨≡○⊑𝕩) 𝕩 ⥊○≠ 𝕨}⌜˜ 3‿100 ⥊⌜ "a⍉𝕩"∾1‿10‿1e4‿1e9‿1e12∾⟨"a",{⇐}⟩

# ⍋𝕩
%USE rand ⋄ ⟨100, 30000, 2e9, ¯100, ¯30000, ¯2e9, @+200, @+60000, @+1111111⟩ {! a ≡ ⍋ •internal.Keep ⍋ 𝕨 + a ← r.Deal 𝕩}⌜ 3‿10‿100‿1000

# 𝕨⍋𝕩
!"𝕨⍋𝕩: 𝕨 must be sorted" % a←-¨ ∧⋈¨ ↕10 ⋄ a⍋⋈¨↕10
!"𝕨⍋𝕩: 𝕨 must be sorted"                     % %USE evar ⋄ ⟨0÷0,1,0÷0⟩ ⍋_evar ⋈1
!"𝕨⍒𝕩: 𝕨 must be sorted in descending order" % %USE evar ⋄ ⟨0÷0,1,0÷0⟩ ⍒_evar ⋈1
(⍷⥊¨)⊸({𝕨‿𝕩! (𝕨⍋𝕩) ≡ (≢𝕩)⥊ (≠𝕨)×0≥𝕨 •Cmp○⊑ 𝕩}⌜) ⟨3, <3, 10⥊3, 'a', <'a', 10⥊'a', 0÷0, 10⥊0÷0⟩
(⍷⥊¨)⊸({𝕨‿𝕩! (𝕨⍒𝕩) ≡ (≢𝕩)⥊ (≠𝕨)×0≤𝕨 •Cmp○⊑ 𝕩}⌜) ⟨3, <3, 10⥊3, 'a', <'a', 10⥊'a', 0÷0, 10⥊0÷0⟩
%USE eqvar ⋄ ⟨0⟩ ⍋_eqvar ⟨0,0÷0⟩ %% 1‿1
%USE eqvar ⋄ ⟨0÷0⟩ ⍋_eqvar ⟨¯1÷0,¯3,0,1,2,1÷0,0÷0⟩ %% 0‿0‿0‿0‿0‿0‿1
%USE eqvar ⋄ ⟨0,1⟩ ⍋_eqvar ⟨0,0÷0⟩ %% 1‿2
%USE eqvar ⋄ ⟨0,1,0÷0⟩ ⍋_eqvar ⟨0,0÷0⟩ %% 1‿3
%USE eqvar ⋄ ⟨0,0÷0⟩ ⍋_eqvar ⟨0,0÷0⟩ %% 1‿2
%USE eqvar ⋄ (100/⟨0,0÷0⟩) ⍋_eqvar ⟨0,0÷0⟩ %% 100‿200
%USE eqvar ⋄ (¯5+↕10) ⍋_eqvar ⟨0,0÷0⟩ %% 6‿10
%USE eqvar ⋄ (¯5+↕10) ⍋_eqvar 10⥊⟨0,0÷0⟩ %% 10⥊6‿10
%USE eqvar ⋄ ((↕5)∾100⥊0÷0) ⍋_eqvar ↕10 %% 5⌊1+↕10

# 𝕨⍒𝕩
%USE eqvar ⋄ ⟨0⟩ ⍒_eqvar ⟨0,0÷0⟩ %% 1‿0
%USE eqvar ⋄ ⟨0÷0⟩ ⍒_eqvar ⟨¯1÷0,¯3,0,1,2,1÷0,0÷0⟩ %% 1‿1‿1‿1‿1‿1‿1
%USE eqvar ⋄ ⟨1,0⟩ ⍒_eqvar ⟨0,0÷0⟩ %% 2‿0
%USE eqvar ⋄ ⟨0÷0,1,0⟩ ⍒_eqvar ⟨0,0÷0⟩ %% 3‿1
%USE eqvar ⋄ ⟨0÷0,0⟩ ⍒_eqvar ⟨0,0÷0⟩ %% 2‿1
%USE eqvar ⋄ (100/⟨0÷0,0⟩) ⍒_eqvar ⟨0,0÷0⟩ %% 200‿100
%USE eqvar ⋄ (5-↕10) ⍒_eqvar ⟨0,0÷0⟩ %% 6‿0
%USE eqvar ⋄ (5-↕10) ⍒_eqvar 10⥊⟨0,0÷0⟩ %% 10⥊6‿0
%USE eqvar ⋄ (⌽(↕5)∾100⥊0÷0) ⍒_eqvar ↕10 %% 105-5⌊↕10

# ∧𝕩, ∨𝕩
∧ "abc"∾↕3 %% (↕3)∾"abc"
∧∨"abc"∾↕3 %% (↕3)∾"abc"
∧∧"abc"∾↕3 %% (↕3)∾"abc"
∨ "abc"∾↕3 %% "cba"∾2‿1‿0
∨∨"abc"∾↕3 %% "cba"∾2‿1‿0
∨∧"abc"∾↕3 %% "cba"∾2‿1‿0

# 𝕨|𝕩
{x←2⥊𝕨|𝕩 ⋄ ! x •internal.EEqual 𝕨‿𝕨|𝕩‿𝕩 ⋄ ! x •internal.EEqual 𝕨|𝕩‿𝕩}⌜˜ ⍷∧∾⟜- ⟨π, ∞, 0÷0, 0.5, 0.1⟩∾(↕10)∾⥊(¯4+↕9)+⌜2⋆7‿8‿15‿31‿32
(
  # AS/SA/AA over types, 2-item array; %SLOW
  %USE var ⋄ EE←•internal.EEqual
  tests ← ⍷∧∾⟜- ⟨π, ∞, 0÷0, 0.5, 0.1⟩∾(↕10)∾⥊(¯4+↕9)+⌜2⋆7‿8‿15‿31‿32
  {w𝕊x:
    wl‿xl←2⥊¨w‿x
    w‿x ! ∧´(⊏EE¨⊢) ⥊ wl {⟨  w|(𝕩 V xl)  ⋄  (𝕨 V wl)|x  ⋄  (𝕨 V wl)|(𝕩 V xl)  ⟩}⌜○LV xl
    CLR@
  }⌜˜ tests
)
(
  %USE rand
  # 1|scalar vs 1|f64arr over float exponents %SLOW
  {𝕩! (1⊸| •internal.EEqual 1|¨⊢) ∾⟜- (2⋆𝕩)×1‿1.5∾1000 r.Range 0}¨ ¯1080+↕2110
)
# 𝕨÷𝕩
!"𝕨÷𝕩: Unexpected argument types" % ⟨1‿2, "ab"⟩÷2
50⊑"7.299999247803105e-309" {(100⥊𝕨)÷𝕩}○•ParseFloat "0.34361186623573303" %% •ParseFloat "2.1244898576334326e-308"
50⊑"3.8863870613495216e-308" {(100⥊𝕨)÷𝕩}○•ParseFloat "0.3815327155289232" %% •ParseFloat "1.0186248526451468e-307"




# ⍟
%USE eqvar ⋄ a←¯5+↕10 ⋄ {1⊸+⍟𝕩 ⋈10}_eqvar a %% ⋈¨ 5+↕10
%USE eqvar ⋄ a←¯5+↕10 ⋄ {1+⍟𝕩 ⋈10}_eqvar a %% ⋈¨ 5+↕10
%USE eqvar ⋄ a←¯5+↕10 ⋄ {1+⍟(𝕩{!𝕩≡⋈10 ⋄ 𝕗}) ⋈10}_eqvar a %% ⋈¨ 5+↕10
!∘0⍟⟨↕0, ⟨⟩, ""⟩ 0 %% ⟨⟨⟩,⟨⟩,⟨⟩⟩
!"Cannot call a 2-modifier as a function" % 1⊸+⍟{∘} 0
!"⍟: 𝔾 contained non-number" % 1⊸+⍟⟨1,∘,3⟩ 0
!"Expected integer, got 1.5" % 1⊸+⍟1.5 0
!"Integer out of range: 1e20" % 1⊸+⍟1e20 0
!"⍟: 𝔾 contained non-integer or integer was out of range" % %USE evar ⋄ {1⊸+⍟𝕩 0}_evar ⟨1.5⟩
!"⍟: 𝔾 contained non-integer or integer was out of range" % %USE evar ⋄ {1⊸+⍟𝕩 0}_evar ⟨1e20⟩
!"⍟: 𝔾 contained non-integer or integer was out of range" % %USE evar ⋄ {1⊸+⍟𝕩 0}_evar ⟨9, 1.5⟩
!"⍟: 𝔾 contained non-integer or integer was out of range" % %USE evar ⋄ {1⊸+⍟𝕩 0}_evar ⟨1.5, 9⟩
!"⍟: 𝔾 contained non-integer or integer was out of range" % %USE eqerr ⋄ {1⊸+⍟𝕩‿𝕩 2}_eqerr -⊸∾ ⟨2⋆63, 2⋆60⟩

# ◶
!"𝔽◶𝕘𝕩: 𝕘 must have rank 1 when index is a number" % 1◶[1‿2,3‿4] 3
!"𝕨𝔽◶𝕘𝕩: 𝕘 must have rank 1 when index is a number" % "ab" ⊢◶[1‿2,3‿4] 3
!"𝕨𝔽◶𝕘𝕩: Index out of bounds of 𝕘" % 1 2◶⊢‿- 3
!"𝔽◶𝕘𝕩: Index out of bounds of 𝕘" % 2◶⊢‿- 3
!"𝕨⊑𝕩: Picking item at wrong rank (index 0‿0 in array of shape ⟨2⟩)" % ⊢◶+‿- 0‿0 # shouldn't reference ⊑ but whatever
!"𝕨⊑𝕩: Picking item at wrong rank (index 1‿2 in array of shape ⟨3⟩)" % 1 +◶+‿-‿⊢ 0‿1
10 +◶"ab"‿"cd" ¯9 %% "cd"
⟨⋈1,⋈2,⋈0⟩◶"ab"‿"cd"‿1 @ %% "cd"‿1‿"ab"


# ˘ & ⎉
≢ (1⥊˜ 0∾203⥊2)⎉50 1⥊˜ 0∾100⥊2 %% 1‿50‿1‿203/0‿2‿0‿2
!"⎉: Result rank too large" % (1⥊˜ 0∾203⥊2)⎉49 1⥊˜ 0∾100⥊2
!"˘: Result rank too large" % ((255⥊1)⥊1)˘ 1‿2
!"˘: Empty argument too large (0‿1000000000‿1000000000‿1000000000 ≡ ≢𝕩)" % 2 •internal.PureKeep˘ 0‿1e9‿1e9‿1e9⥊0
!"˘: Empty argument too large (0‿1000000000‿1000000000‿1000000000 ≡ ≢𝕩)" % •internal.PureKeep˘ 0‿1e9‿1e9‿1e9⥊0
!"⎉: Empty argument too large (0‿0‿1000000000‿1000000000‿1000000000 ≡ ≢𝕩)" % 2 •internal.PureKeep⎉3 0‿0‿1e9‿1e9‿1e9⥊0
!"⎉: Empty argument too large (0‿0‿1000000000‿1000000000‿1000000000 ≡ ≢𝕩)" % •internal.PureKeep⎉3 0‿0‿1e9‿1e9‿1e9⥊0
!"≍˘ 𝕩: Result rank too large (255≡=𝕩)" % ≍˘ (255⥊1)⥊1
!"≍⎉𝕘 𝕩: Result rank too large (255≡=𝕩)" % ≍⎉100 (255⥊1)⥊1
!"𝔽˘: Incompatible result shapes (encountered shapes ⟨0⟩ and ⟨10⟩)" % {↕⊑𝕩}˘ 4‿10⥊↕100
!"𝔽˘: Incompatible result shapes (encountered shapes ⟨0⟩ and ⟨10⟩)" % 3 {↕⊑𝕩}˘ 4‿10⥊↕100
!"𝔽⎉𝕘: Incompatible result shapes (encountered shapes ⟨0⟩ and ⟨10⟩)" % {↕⊑𝕩}⎉1 4‿10⥊↕100
!"𝔽⎉𝕘: Incompatible result shapes (encountered shapes ⟨0⟩ and ⟨10⟩)" % 3 {↕⊑𝕩}⎉1 4‿10⥊↕100

# `
!"𝔽`𝕩: 𝕩 cannot have rank 0" % ⊢`0
!"𝕨𝔽`𝕩: Shape of 𝕨 must match the cell of 𝕩 (2‿2 ≡ ≢𝕨, 3‿2‿3 ≡ ≢𝕩)" % (2‿2⥊1)+`↕3‿2‿3
!"𝕨𝔽`𝕩: Shape of 𝕨 must match the cell of 𝕩 (2‿2 ≡ ≢𝕨, 3‿3‿2 ≡ ≢𝕩)" % (2‿2⥊1)+`↕3‿3‿2
!"𝕨𝔽`𝕩: Shape of 𝕨 must match the cell of 𝕩 (⟨⟩ ≡ ≢𝕨, 3‿3 ≡ ≢𝕩)" % 2+`↕3‿3
%USE eqvar ⋄ %USE k ⋄ ⟨+,-,×,÷,⋆,√,⌊,⌈,¬,∧,∨,<,>,≠,=,≤,≥,⊣,⊢⟩ {f𝕊arr: ! (  F _k`_k ⥊arr) ≡ ⥊_k         F`  _eqvar arr}⌜ {𝕩∾≍˘¨𝕩} ≍˘¨ ⟨⋈1, 1‿0‿1⟩
%USE eqvar ⋄ %USE k ⋄ ⟨+,-,×,÷,⋆,√,⌊,⌈,¬,∧,∨,<,>,≠,=,≤,≥,⊣,⊢⟩ {f𝕊arr: ! (1 F _k`_k ⥊arr) ≡ ⥊_k (1¨⊏𝕩)⊸(F`) _eqvar arr}⌜ {𝕩∾≍˘¨𝕩} ≍˘¨ ⟨⋈1, 1‿0‿1⟩


# ´
!"𝔽´𝕩: 𝕩 must be a list (⟨⟩ ≡ ≢𝕩)" % +´0
!"𝔽´𝕩: 𝕩 must be a list (3‿3 ≡ ≢𝕩)" % +´↕3‿3
!"𝕨𝔽´𝕩: 𝕩 must be a list (3‿3 ≡ ≢𝕩)" % 2+´↕3‿3
!"𝔽´𝕩: Identity not found" % •internal.Keep´⟨⟩
%USE eqvar ⋄ +´_eqvar 1e3⥊1e2 %% 1e5
%USE eqvar ⋄ +´_eqvar 1e3⥊↕100 %% 4950e1
%USE eqvar ⋄ +´_eqvar 1e4⥊-↕100 %% ¯4950e2
%USE eqvar ⋄ +´_eqvar 1e4⥊-1e9 %% ¯1e13
+⌜´↕0 %% <0
×⌜´↕0 %% <1
+⌜⌜⌜´↕0 %% <<<0
×⌜⌜⌜´↕0 %% <<<1

# ˝
!"𝔽˝𝕩: 𝕩 must have rank at least 1" % ⊢˝0
!"𝔽˝𝕩: 𝕩 must have rank at least 1" % ⊢˝<0
!"𝕨𝔽˝𝕩: 𝕩 must have rank at least 1" % 0⊢˝0
!"𝕨𝔽˝𝕩: 𝕩 must have rank at least 1" % 0⊢˝<0
!"𝔽˝𝕩: Identity not found" % •internal.Keep˝⟨⟩
(↕4)+˝↕7 %% 21‿22‿23‿24
(2‿2⥊↕4)+˝7‿2‿2⥊↕28 %% 2‿2⥊84‿92‿100‿108
(2‿2⥊↕4)+˝2‿2⥊↕28 %% 2‿2⥊2‿3‿6‿7
+˝ ↕10‿4 %% 45 ⋈¨ 10×↕4
(   ↕4) +˝ ↕10‿4 %% (↕4) + 45 ⋈¨ 10×↕4
(<¨ ↕4) +˝ ↕10‿4 %% (↕4) + 45 ⋈¨ 10×↕4
(0‿5⊸+¨↕4) +˝ ↕10‿4 %% ((↕4) + 45) ⋈¨ 5 + 11×↕4
{! 1≥•internal.ElType 𝕏˜ "Ah"•internal.Variation 10⥊1}¨ ⟨+,-,×,÷,⋆,√,⌊,⌈,|,¬,∧,∨,<,>,≠,=,≤,≥⟩ # insert_scal relies on all pervasive dyadic builtins implicitly squeezing

# ⌜
!"𝕨𝔽⌜𝕩: Result rank too large (200≡=𝕨, 200≡=𝕩)" % +⌜˜(200⥊1)⥊1
%USE eqvar ⋄ (! + ○•internal.Keep⌜ ≡ + ⌜_eqvar)○↕⌜˜ 0‿1‿2‿4‿9‿16
%USE eqvar ⋄ (! -˜○•internal.Keep⌜ ≡ -˜⌜_eqvar)○↕⌜˜ 0‿1‿2‿4‿9‿16
%USE eqvar ⋄ (!  ⊢○•internal.Keep⌜ ≡  ⊢⌜_eqvar)○↕⌜˜ 0‿1‿2‿4‿9‿16
%USE eqvar ⋄ (!  ⊣○•internal.Keep⌜ ≡  ⊣⌜_eqvar)○↕⌜˜ 0‿1‿2‿4‿9‿16
%USE eqvar ⋄ (!  9○•internal.Keep⌜ ≡  9⌜_eqvar)○↕⌜˜ 0‿1‿2‿4‿9‿16
%USE eqall ⋄ Eqall {𝕊: 1‿2}⌜⌜˜ ⟨1, <1⟩ %% <1‿2
%USE eqall ⋄ Eqall      1‿2⌜⌜˜ ⟨1, <1⟩ %% <1‿2

# ˜
5˜˝ "ab" %% 5
5˜⌜˜ "a" %% [[5]]
5˜˝˘≍"ab" %% ⟨5⟩
