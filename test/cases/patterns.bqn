%DEF k _k ← {𝔽○•internal.Keep}

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
!"Calling a modifier" % 4 {˜}´↕1
!"Calling a modifier" % 4 {˜}˝↕1
!"Calling a modifier" % 4 {∘}`↕1
!"Calling a modifier" % {∘}¨↕1
!"Cannot call a modifier" % {˜}¨˜↕1
4 {˜}˙´↕1 %% ˜
4 {˜}˙˝↕1 %% ˜
4 {∘}˙`↕1 %% ⟨∘⟩
{∘}˙¨↕1 %% ⟨∘⟩
{˜}˙¨˜↕1 %% ⟨˜⟩
4 "ab"`↕4 %% 4⥊<"ab"
!"Calling a modifier" % {∘}⌜˜ ↕10
{∘}˙⌜˜ ↕10 %% 10‿10⥊⟨∘⟩
"ab"` "hello" %% 'h'∾4⥊<"ab"

# ˝ plus ˘
%USE k ⋄ (+˝˘ !∘≡ +˝_k˘) 1000000‿3•rand.Range 0
%USE k ⋄ (-˜˝ !∘≡ -˜_k˝) 10 (•MakeRand 0).Range 1e5
%USE k ⋄ (-˜˝˘ !∘≡ -˜_k˝˘) 3↕10 (•MakeRand 0).Range 1e5

# +`
%USE k ⋄ {𝕊:                 (+` !∘≡ +_k`) 1e3•rand.Range 0}¨↕1000
%USE k ⋄ {𝕊: (•rand.Range 0) (+` !∘≡ +_k`) 1e3•rand.Range 0}¨↕1000

# <`
%USE k ⋄ (<` !∘≡ <_k`)¨ (1000⊸⥊)¨⊸∾ "Ab"⊸•internal.Variation¨ "111000"‿"1111000"‿"0111000"-'0'

# ≠`
{𝕤⋄b←"Ab"•internal.Variation a←"Ai32"•internal.Variation 1000 •rand.Range 2 ⋄ ! (≠`a)≡≠`b}¨↕1000
{𝕤⋄b←"Ab"•internal.Variation a←"Ai32"•internal.Variation 1000 •rand.Range 2 ⋄ ! (0≠`a)≡0≠`b}¨↕1000
{𝕤⋄b←"Ab"•internal.Variation a←"Ai32"•internal.Variation 1000 •rand.Range 2 ⋄ ! (1≠`a)≡1≠`b}¨↕1000

# ∨´ & ∧´
{n←𝕩 ⋄ ! (1+n) ≡ +´¬{∨´(-𝕩)↑1∾n↑0}¨↕400}¨↕200 # ∨´ with a single 1; %SLOW
{n←𝕩 ⋄ ! (1+n) ≡ +´¬ {¬∧´¬(-𝕩)↑1∾n↑0}¨↕400}¨↕200 # ∧´ with a single 1; %SLOW
{!¬ ∨´𝕩⥊0}¨↕1000 # ∨´ zeroes
{!¬ ∧´𝕩⥊0}¨1+↕1000 # ∧´ zeroes
{! ∨´𝕩⥊1}¨1+↕1000 # ∨´ ones
{! ∧´𝕩⥊1}¨↕1000 # ∧´ ones
