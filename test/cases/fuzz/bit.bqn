### WHOLE-FILE-TEST
# •bit tests
helpers ← ⊑•args
r ← (helpers.GetRand@)
⟨RandVals, casts⟩ ← ⟨r⟩ helpers.Import "utils.bqn"
widths ← ⊑¨casts

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
cs ← / 64>widths              # TODO floats
cx ← ⊏⟜(32‿'c'⊸≢¨casts)⊸/ cs  # TODO 32-bit char output
ls ← (↕100) ∾ r.Range¨⊸+ ⌊ 100 × ↕∘⌈⌾(1.5⊸⋆⁼) 100
{cs (⋈ !∘TestCast 𝕩˙)⌜ cx}¨ ls

# Now other •bit operations
OpArgs ← {_b‿F‿a:
  m←(1<a) ⊑ {𝔽1⊸⊑}‿{𝔽´}  # Adjust to act on argument list 𝕨‿𝕩
  {𝕗 _b _m}‿(F _m)‿a
}
ops ← ¯1 (⊑¨ ⊔ OpArgs∘↓¨) ⟨ # Group by args then int-ness
  #   op fn args int
  •bit._not‿¬‿1‿0
  •bit._and‿∧‿2‿0
  •bit._or ‿∨‿2‿0
  •bit._xor‿≠‿2‿0
  •bit._neg‿-‿1‿1
  •bit._add‿+‿2‿1
  •bit._sub‿-‿2‿1
  •bit._mul‿×‿2‿1
⟩
cres ← (widths ≤ 32) ∧ 'c' ≠ 1⊑¨casts  # Possible results; TODO floats
TestOps ← { 𝕊 len: # Length in bits
  tz ← +´¬∨`2|⌊∘÷⟜2⍟(↕7) len  # Trailing zeros
  
  wxc ← casts ⊏˜ wx ← (2 r.Range ≠)⊸⊏ / cf ← widths≤2⋆tz
  orc ← casts ⊏˜ or ← (2 r.Range ≠)⊸⊏ / cf∧cres
  ow‿·‿xw‿ww ← wd ← (or∾⌽wx) ⊏ widths # •bit._*'s 𝕗
  
  alens0 ← len ÷ ww‿xw # argument lengths
  one ← r.Range 3 # 0/1: make 𝕨/𝕩 extended; 2: keep same-length
  alens1 ← {1≤p←(one⊑ww‿xw)÷˜ow? p; one↩2 ⋄ 𝕩}⌾(one⊸⊑)⍟(one≤1) alens0 # maybe reduce one argument to a single element
  
  args ← alens1 RandVals¨ wx
  _bc ← •bit._cast
  Test ← { c 𝕊 ops:
    cargs ← wxc {𝕨‿c _bc 𝕩}¨ alens0 ⥊¨ args
    mod ← {1:⊢ ; m←2⋆𝕩 ⋄ m⊸|⌾((m÷2)⊸+)} ⊑c
    Test1 ← { _b‿Fn‿a:
      ¬ (1=one)∧a=1? # can't test a monadic function with a should-be-extended arg
        got ← ((2+a)↑wd)_b args
        exp ← ⟨c,1⊑orc⟩_bc Mod Fn cargs
        ! got≡exp
      ;@
    }
    Test1¨ ops
  }
  DropMul ← ¯1⊸↓⍟(32=ow) # Multiplication test may need 64 bits
  ⟨1‿'u',⊑orc⟩ Test¨ 0⊸↑⍟(1=ow)∘DropMul⌾(1⊸⊑) ops
}
(2⋆0‿3‿4‿5‿6) TestOps∘×⌜ ls
