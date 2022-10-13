u ← ⌊100×(•UnixTime+1|100×•MonoTime)@
r ← •MakeRand •Show u
⟨RandVals, casts⟩ ← ⟨r⟩ •Import "utils.bqn"

# Test •bit._cast first
# Convert unsigned width 𝕗 to 𝕘
_cvtu_ ← {
  𝕗=𝕘 ? 𝕩 ;
  𝕗<𝕘 ? b←2⋆𝕗 ⋄ +⟜(b⊸×)˝⍉∘‿(𝕘÷𝕗)⥊𝕩 ;
        b←2⋆𝕘 ⋄ ⥊⍉>1(-⊸↓-b×↓)⌊∘÷⟜b⍟(↕1+𝕗÷𝕘)𝕩
}
_bitcast ← { fr‿to _𝕣 𝕩:
  S ← {𝕊:𝕩+𝕨×𝕩<0; 𝕊⁼:𝕩-𝕨×𝕩≥𝕨÷2}
  _off ← {(⊑"uci"⊐1⊑𝕗)⊑⟨⊢,-⟜@,(2⋆⊑𝕗)⊸S⟩}
  f‿t ← ⊑¨𝕗
  to _off⁼  f _cvtu_ t  fr _off  𝕩
}

TestCast ← { f‿t 𝕊 len:
  c ← 𝕨 ⊏ casts
  len +↩ (1⌈÷˜´⊑¨c) | -len  # Round up to exact conversion
  (c _bitcast ≡ c •bit._cast) len RandVals f
}
cs ← / 64>⊑¨casts             # TODO floats
cx ← ⊏⟜(32‿'c'⊸≢¨casts)⊸/ cs  # TODO 32-bit char output
ls ← (↕100) ∾ r.Range¨⊸+ ⌊ 100 × ↕∘⌈⌾(1.5⊸⋆⁼) 100
{cs (⋈ !∘TestCast 𝕩˙)⌜ cx}¨ ls
