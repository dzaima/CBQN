⟨Variation⟩ ← •internal

r ← ⊑•args

# types: u1,i8,i16,i32,f64,c8,c16,c32
maxF64 ⇐ +´2⋆1023-↕53
min ← 0∾(-2⋆7‿15‿31)∾¯0.5∾0‿0‿0
max ← 2∾( 2⋆7‿15‿31)∾ 0.5∾(2⋆8‿16)∾1114112
szs ← max-min
chr ← 0‿0‿0‿0‿0‿1‿1‿1
specF64 ⇐ ∾⟜- π‿2∾ ÷⟜0⊸∾ 1‿0
specI32 ⇐ "Ai32" •internal.Variation ∧ (<⟜(2⋆31)∧≥⟜(-2⋆31))⊸/ ∾⟜- ≥⟜0⊸/ ∧⥊(¯4+↕8) +⌜ 2⋆↕32
vars ⇐ "Ab"‿"Ai8"‿"Ai16"‿"Ai32"‿"Af64"‿"Ac8"‿"Ac16"‿"Ac32"
casts ⇐ 1‿8‿16‿32‿64‿8‿16‿32⋈¨"uiiifccc"
specInts ⇐ 7‿15({(<⟜(2⋆𝕨)∧≥⟜(-2⋆𝕨))⊸/𝕩}¨⟜<∾⋈∘⊢)specI32 # TODO 8-bit integer spec is kinda stupid
spec0 ← ⟨0‿1⟩∾specInts∾⟨specF64⟩∾@+∾⟜(⌽1114111-↕16)⌾(2⊸⊑) (<⟜1114111∧≥⟜0)⊸/¨ specInts
spec0 (5/¯128‿¯127‿¯126‿¯2‿¯1‿0‿1‿2‿125‿126‿127)⊸∾⌾(1⊸⊑)↩
spec ⇐ vars Variation¨ spec0
! spec0 ≡○((4≠↕8)⊸/) spec

SubRange ⇐ {
  sz ← 𝕩-𝕨
  l ← 1 + r.Range 4⌊⍟(0=r.Range 5) sz
  0‿l+𝕨+r.Range 1+sz-l
}
# •Show¨ ⍷∧{𝕊:¯3 SubRange 3}¨↕1000
# •Show¨ ∾⟜(-˜´)¨ ∧{𝕊:¯1000 SubRange 1000}¨↕100

Variate ← { sp𝕊𝕩:
  ((0=r.Range 5)◶{𝕩⥊sp⊑˜r.Range ≠sp}‿{sp⊏˜𝕩 r.Range ≠sp})∘≠⌾((0=(≠𝕩) r.Range 2⋆0⌈10-˜r.Range 20)⊸/) 𝕩
}
RandVals ⇐ {
  # TODO special code for 𝕩≡0
  # TODO bit-based randomization
  𝕩≡4? "Af64" Variation specF64 Variate (2⋆1024×r.Range 0) × (¯2+4×r.Range 0) + 𝕨 r.Range 0;
  s‿e ← (𝕩⊑min) SubRange 𝕩⊑max
  (𝕩⊑vars) Variation (𝕩⊑spec) Variate @+⍟(𝕩⊑chr) s+𝕨 r.Range e-s
}

! "bitarr" ≡ •internal.Type 10 RandVals 0
! "i8arr"  ≡ •internal.Type 10 RandVals 1
! "i16arr" ≡ •internal.Type 10 RandVals 2
! "i32arr" ≡ •internal.Type 10 RandVals 3
! "f64arr" ≡ •internal.Type 10 RandVals 4
! "c8arr"  ≡ •internal.Type 10 RandVals 5
! "c16arr" ≡ •internal.Type 10 RandVals 6
! "c32arr" ≡ •internal.Type 10 RandVals 7
