%DEF eqvar ⟨_eqvar⟩←⊑•args
%DEF arithdTest %USE eqvar ⋄ Eq ← ⊢!•internal.Indistinguishable ⋄ ArithdTest ← {𝕨𝕊w‿F‿x: el←10⥊𝕨 ⋄ 𝕨 Eq r←w F x ⋄ el Eq w⊸F _eqvar 10⥊x ⋄ el Eq F⟜x _eqvar 10⥊w ⋄ el Eq w F _eqvar○(10⊸⥊) x ⋄ r} ⋄ ArithdTestX ← {w‿F‿x: w F x} ArithdTest ⊢
%DEF fill Fill←•internal.HasFill◶'e'‿{⊑1↑0⥊𝕩}
%DEF rand r ← (⊑•args).GetRand@
%DEF approxCpx ApproxCpx ← {⍳´ ⌊0.5+ 1e3×(•math.Re⋈•math.Im) 𝕩}

# ⟨0, 1, ∞, ¯∞, ∞i∞, ¯∞i∞, ∞i¯∞, ¯∞i¯∞⟩ %% ⟨0, 1, i←1÷0, n←-i, i⍳i, n⍳i, i⍳n, n⍳n⟩

√¯1 %% 0i1
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx √_eqvar 1‿2‿3‿¯1‿4 %% 1000‿1414‿1732‿0i1000‿2000
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx √_eqvar 1‿2‿3‿1i2‿4‿5 %% 1000‿1414‿1732‿1272i786‿2000‿2236
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx √_eqvar 1i2‿2i3‿3i5 %% 1272i786‿1674i896‿2101i1190
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx 3‿4‿5   √_eqvar 1i2‿2i3‿3i4 %% 1220i472‿1337i335‿1356i254
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx 3‿4‿5i3 √_eqvar 1i2‿2i3‿3i4 %% 1220i472‿1337i335‿1375i¯8
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx 1i3‿0i4‿5i3 √_eqvar 3‿4‿5 %% 1056i¯361‿941i¯340‿1254i¯179
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx 3‿3 √_eqvar 3‿¯3 %% 1442‿721i1249
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx 3‿¯3 ⋆_eqvar 2.2‿2.2 %% 11212‿9070i6590
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx 2‿¯2 ⋆⁼_eqvar 3‿3 %% 1585‿74i¯333
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx ×_eqvar ∾⟜(1.2i1.4⊸×) 0‿¯1‿1‿¯1.5‿1.5‿0i1‿0i¯1‿1i1‿¯1i1‿1i¯1‿¯1i¯1 %% 0‿¯1000‿1000‿¯1000‿1000‿0i1000‿0i¯1000‿707i707‿¯707i707‿707i¯707‿¯707i¯707‿0‿¯651i¯759‿651i759‿¯651i¯759‿651i759‿¯759i651‿759i¯651‿¯77i997‿¯997i¯77‿997i77‿77i¯997
%USE eqvar ⋄ %USE approxCpx ⋄ ApproxCpx {r←¬_eqvar 𝕩 ⋄ !r≡1-𝕩 ⋄ r} 0‿¯1‿1‿¯1.5‿1.5‿0i1‿0i¯1‿1i1‿¯1i1‿1i¯1‿¯1i¯1 %% 1000‿2000‿0‿2500‿¯500‿1000i¯1000‿1000i1000‿0i¯1000‿2000i¯1000‿0i1000‿2000i1000
%USE rand ⋄ K←•internal.Keep ⋄ ! (×∘K∘× ≡○K ×⍟2∘K) 1i1 ∾ ⍳˝ 2‿10 r.Range 0
%USE arithdTest ⋄ %USE approxCpx ⋄ ApproxCpx ArithdTestX ⟨2.1i3.3,+ ,5.2i2.1⟩ %% 7300i5400
%USE arithdTest ⋄ %USE approxCpx ⋄ ApproxCpx ArithdTestX ⟨2.1i3.3,- ,5.2i2.1⟩ %% ¯3100i1200
%USE arithdTest ⋄ %USE approxCpx ⋄ ApproxCpx ArithdTestX ⟨2.1i3.3,× ,5.2i2.1⟩ %% 3990i21570
%USE arithdTest ⋄ %USE approxCpx ⋄ ApproxCpx ArithdTestX ⟨2.1i3.3,÷ ,5.2i2.1⟩ %% 568i405
%USE arithdTest ⋄ %USE approxCpx ⋄ ApproxCpx ArithdTestX ⟨2.1i3.3,⋆ ,5.2i2.1⟩ %% ¯33495i142146
%USE arithdTest ⋄ %USE approxCpx ⋄ ApproxCpx ArithdTestX ⟨2.1i3.3,⋆⁼,5.2i2.1⟩ %% 954i¯421
%USE arithdTest ⋄ %USE approxCpx ⋄ ApproxCpx ArithdTestX ⟨2.1i3.3,√ ,5.2i2.1⟩ %% 1307i¯432

# TODO don't, and move to prims.bqn arithdTest
•Repr ¯∞⋆⁼∞ %% "NaNiNaN"
•Repr ¯∞⋆⁼6 %% "NaNiNaN"

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
%USE fill ⋄ •math.Re 2+3×√¯1 %% 2
%USE fill ⋄ •math.Re 12 %% 12
%USE fill ⋄ •math.Re 3‿3⥊↕9 %% 3‿3⥊↕9
%USE fill ⋄ •math.Im 12 %% 0
%USE fill ⋄ •math.Im 3‿3⥊↕9 %% 3‿3⥊0
%USE fill ⋄ •math.Im 2+3×√¯1 %% 3
%USE rand ⋄ a‿b‿c←1000 r.Range¨ 3⥊0 ⋄ +´ (2×a÷c) = +⟜+ (a+b×√¯1)÷c %% 1000
(•math.Re⋈•math.Im) (1+√¯1) ÷ -0 %% ∞‿∞

1i2 ⍳ 10i20 %% ¯19i12
(↕2) ⍳ 2‿2⥊↕4 %%% 0 WF 2‿2⥊0‿0i1‿1i2‿1i3
