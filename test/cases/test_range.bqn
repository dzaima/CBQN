≢(⟨⟨0‿9‿1,↕10⟩ ⋄ ⟨¯5‿¯3‿1 ⋄ ¯5‿¯3‿¯4‿¯4‿¯4‿¯4‿¯4⟩⟩∾{⟨⟨¯2,𝕩-3,1⟩,¯2+↕𝕩⟩}¨1+↕100) {exp‿a𝕊𝕩: exp (⋈!≡) •internal.Temp 𝕩•internal.Variation a}⌜ "Ai8"‿"Ai16"‿"Ai32"‿"Af64"
T←⋈ ! ⊣ ≡ ·•internal.Temp "Ab"•internal.Variation⊢ ⋄ ≠{𝕊n: 1‿1‿1 T 𝕩⥊1 ⋄ 0‿0‿1 T 𝕩⥊0 ⋄ 0‿1 {0‿1‿1 T (¬𝕨)⌾(𝕩⊸⊑) n⥊𝕨}¨⍟(n>1) 2•rand.Range n}¨1+↕500
{! 0≡2⊑•internal.Temp (2⋆63)⌾(𝕩⊸⊑) 10⥊0}¨ ↕10
⟨2⋆63, 1.5, 0÷0⟩ {! 0≡2⊑•internal.Temp 𝕨⌾(𝕩⊸⊑) 20⥊0}⌜ ↕20
≠{! 0‿0‿1 ≡ •internal.Temp 𝕩↑(𝕩⥊0)∾10⥊0÷0}¨ 1+↕10
