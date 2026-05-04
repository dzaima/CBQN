%DEF eqvar ⟨_eqvar⟩←⊑•args
%DEF arithdTest %USE eqvar ⋄ Eq ← ⊢!•internal.Indistinguishable ⋄ ArithdTest ← {𝕨𝕊w‿F‿x: el←10⥊𝕨 ⋄ 𝕨 Eq r←w F x ⋄ el Eq w⊸F _eqvar 10⥊x ⋄ el Eq F⟜x _eqvar 10⥊w ⋄ el Eq w F _eqvar○(10⊸⥊) x ⋄ r}
(0÷0) •internal.Indistinguishable √¯1 %% 1
(10⥊0÷0) •internal.Indistinguishable √10⥊¯1 %% 1
%USE arithdTest ⋄ (0÷0) ArithdTest¨ ⟨⟨¯1,⋆⁼,2⟩, ¯0.5‿⋆‿¯0.5, 3‿√‿¯2, ⟨¯∞,⋆⁼,∞⟩, ⟨¯∞,⋆⁼,6⟩⟩

