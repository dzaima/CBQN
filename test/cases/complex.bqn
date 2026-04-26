%DEF fill Fill←•internal.HasFill◶'e'‿{⊑1↑0⥊𝕩}
%DEF rand r ← (⊑•args).GetRand@

+⊸+ ⍷ 0‿1‿2‿0‿2‿5 + 3×√¯1 %%% 0‿2‿4‿10
≡⌜˜ 0‿1‿2‿0‿2‿5 + 3×√¯1 %%% =⌜˜ "abcacd"
1↓4↑3×√¯1 %%% 0‿0‿0
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 3↑⟨3×√¯1, 2⟩
"a"⌾(1‿0⊸/) (3×√¯1) ⋈ 2 %% 'a'‿2

%USE fill ⋄ Fill < √¯1 %% 0
%USE fill ⋄ Fill ⋈ √¯1 %% 0
%USE fill ⋄ Fill ⥊ √¯1 %% 0
%USE fill ⋄ Fill ⍉ √¯1 %% 0

%USE fill ⋄ Fill ∾˜ √¯1 %% 0
%USE fill ⋄ Fill ⋈˜ √¯1 %% 0
%USE fill ⋄ Fill 2 ⥊  √¯1 %% 0
%USE fill ⋄ Fill @ ⋈  √¯1 %% 'e'
%USE fill ⋄ Fill 1 ⋈  √¯1 %% 0
%USE fill ⋄ Fill 1 ⋈˜ √¯1 %% 0
%USE fill ⋄ Fill 1 ∾  √¯1 %% 0
%USE fill ⋄ Fill 1 ∾˜ √¯1 %% 0
%USE rand ⋄ a‿b‿c←1000 r.Range¨ 3⥊0 ⋄ +´ (2×a÷c) = +⟜+ (a+b×√¯1)÷c %% 1000