%DEF k _k ← {𝔽○•internal.Keep}
%DEF rand r ← (⊑•args).GetRand@

# weird "functions"
4 10´⟨⟩ %% 4
4 10˝⟨⟩ %% 4
4 10`⟨⟩ %% ⟨⟩
4 10´↕0 %% 4
4 10˝↕0 %% 4
4 10`↕0 %% ↕0
4 {˜}´↕0 %% 4
4 {˜}˝↕0 %% 4
4 {∘}`↕0 %% ↕0
{∘}¨↕0 %% ⟨⟩
{˜}¨˜↕0 %% ⟨⟩
!"Cannot call a 1-modifier as a function" % 4 {˜}´↕1
!"Cannot call a 1-modifier as a function" % 4 {˜}˝↕1
!"Cannot call a 2-modifier as a function" % 4 {∘}`↕1
!"Cannot call a 2-modifier as a function" % {∘}¨↕1
!"Cannot call a 1-modifier as a function" % {˜}¨˜↕1
4 {˜}˙´↕1 %% ˜
4 {˜}˙˝↕1 %% ˜
4 {∘}˙`↕1 %% ⟨∘⟩
{∘}˙¨↕1 %% ⟨∘⟩
{˜}˙¨˜↕1 %% ⟨˜⟩
4 "ab"`↕4 %% 4⥊<"ab"
!"Cannot call a 2-modifier as a function" % {∘}⌜˜ ↕10
{∘}˙⌜˜ ↕10 %% 10‿10⥊⟨∘⟩
"ab"` "hello" %% 'h'∾4⥊<"ab"
"ab"˙` "hello" %% 'h'∾4⥊<"ab"
(<4)´↕6 %% <4
(<4)˝↕6‿0 %% <4
(<4)˙´↕6 %% <4
(<4)˙˝↕6‿0 %% <4

# ⊣⊢ as operands
⟨⊢,⊣⟩ {i𝕊m: F←I _m ⋄ ! F⊸≡ ↕0 ⋄ ! @⊸F⊸≡ ""}⌜ ⟨˘,¨,⌜,`⟩
%USE k ⋄ (⟨⊢,⊣⟩ {i𝕊m: I _m !∘≡ I _k _m}⌜ ⟨˘,¨,⌜,´,˝,`⟩) {𝕎𝕩}⌜ ⟨↕10, 4‿0‿2⥊¨"abc"⟩
%USE k ⋄ (⟨⊢,⊣⟩ {i𝕊m: I _m !∘≡ I _k _m}⌜ ⟨˘,¨,⌜,  ˝,`⟩) {𝕎𝕩}⌜ ⟨↕2‿3, "xyz"≍˘⎉0‿1↕8⟩
%USE k ⋄ {(𝕏¨ !∘≡ 𝕏_k¨){𝔽⌜∧𝔽⌜˜} (⊢∾⊢⋈≍˘⟜⌽) <⍟(↕4)@}¨ ⊣‿⊢
%USE k ⋄ {(𝕏⌜ !∘≡ 𝕏_k⌜)⌜˜ ⟨@, <3, ↕4, ↕0‿0, ⥊⟜(↕×´)3‿5‿1⟩}¨ ⊣‿⊢
%USE k ⋄ a ← ⥊⟜(↕×´)4⥊2 ⋄ ⊣‿⊢ {(𝕎⎉𝕩⌜ !∘≡ 𝕎_k⎉𝕩⌜)˜ a}⌜ ⋈⌜˜ ↕5
! (⊢` ≡ ⊢) ⟨@,1,2.5⟩+<↕4
! "ignore" <⊸(⊢` ≡ ⊢) ↕12
⊣` ⟨@,1,2.5⟩+<↕4 %% 3⥊<@+↕4
5 ⊣` ⟨@,1,2.5⟩+<↕4 %% 3⥊5
(↕4) ⊣` ↕6‿4 %% 6‿4⥊↕4
!"Mapping: Expected equal shape prefix (⟨3⟩ ≡ ≢𝕨, ⟨2⟩ ≡ ≢𝕩)" % 1‿2‿3 ⊢¨ 1‿2
!"Mapping: Expected equal shape prefix (⟨3⟩ ≡ ≢𝕨, ⟨2⟩ ≡ ≢𝕩)" % 1‿2‿3 ⊣¨ 1‿2

# ˝ plus ˘
%USE rand ⋄ %USE k ⋄ (+˝˘ !∘≡ +˝_k˘) 1000000‿3 r.Range 0
%USE rand ⋄ %USE k ⋄ (-˜˝ !∘≡ -˜_k˝) 10 r.Range 1e5
%USE rand ⋄ %USE k ⋄ (-˜˝˘ !∘≡ -˜_k˝˘) 3↕10 r.Range 1e5

# +`
%USE rand ⋄ %USE k ⋄ {𝕊:              (+` !∘≡ +_k`) 1e3 r.Range 0}¨↕1000
%USE rand ⋄ %USE k ⋄ {𝕊: ( r.Range 0) (+` !∘≡ +_k`) 1e3 r.Range 0}¨↕1000

# <`
%USE k ⋄ (<` !∘≡ <_k`)¨ (1000⊸⥊)¨⊸∾ "Ab"⊸•internal.Variation¨ "111000"‿"1111000"‿"0111000"-'0'

# ≠`
%USE rand ⋄ {𝕤⋄b←"Ab"•internal.Variation a←"Ai32"•internal.Variation 1000 r.Range 2 ⋄ ! (≠`a)≡≠`b}¨↕1000
%USE rand ⋄ {𝕤⋄b←"Ab"•internal.Variation a←"Ai32"•internal.Variation 1000 r.Range 2 ⋄ ! (0≠`a)≡0≠`b}¨↕1000
%USE rand ⋄ {𝕤⋄b←"Ab"•internal.Variation a←"Ai32"•internal.Variation 1000 r.Range 2 ⋄ ! (1≠`a)≡1≠`b}¨↕1000

# ∨´ & ∧´
{n←𝕩 ⋄ ! (1+n) ≡ +´¬{∨´(-𝕩)↑1∾n↑0}¨↕400}¨↕200 # ∨´ with a single 1; %SLOW
{n←𝕩 ⋄ ! (1+n) ≡ +´¬ {¬∧´¬(-𝕩)↑1∾n↑0}¨↕400}¨↕200 # ∧´ with a single 1; %SLOW
{!¬ ∨´𝕩⥊0}¨↕1000 # ∨´ zeroes
{!¬ ∧´𝕩⥊0}¨1+↕1000 # ∧´ zeroes
{! ∨´𝕩⥊1}¨1+↕1000 # ∨´ ones
{! ∧´𝕩⥊1}¨↕1000 # ∧´ ones

a←0↑∧1‿10⥊0 ⋄ a⍋↕10 ⋄ •internal.Squeeze a %% 0‿10⥊0 # sorted integer squeeze path on empty array
