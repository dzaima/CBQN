eltype2bit ← ⟨1,8,16,32,64,8‿'c',16‿'c',32‿'c'⟩
eltypeWidth ← ⟨1,1,2,4,8,1,2,4⟩

SStep ← •Type◶⟨
  {
    𝕩 •internal.Squeeze↩
    eltype ← •internal.ElType𝕩
    ×∘≠¨⊸/ ⟨
      ⟨0, eltype⟩
      32‿8•bit._cast ⋈=𝕩
      64‿8•bit._cast ≢𝕩
      {
        eltype=8? SStep¨ ⥊𝕩;
        ⟨eltype⊑eltype2bit, 8⟩•bit._cast (8×·⌈≠÷8˙)⊸↑⍟(0=eltype) ⥊𝕩
      }𝕩
    ⟩
  }
  { ⟨⟨1⟩, 64‿8•bit._cast⋈𝕩⟩ }
  { ⟨⟨2⟩, 32‿8•bit._cast⋈𝕩⟩ }
  {𝕊:!"Cannot serialize functions"}
  {𝕊:!"Cannot serialize 1-modifiers"}
  {𝕊:!"Cannot serialize 2-modifiers"}
  {𝕊:!"Cannot serialize namespaces"}
⟩

Serialize ⇐ {
  lists ← ⟨⟩
  {0=•Type⊑𝕩? 𝕊¨𝕩; lists∾↩<𝕩 ⋄ 1} SStep 𝕩
  ∾lists
}
Deserialize ⇐ {
  s←𝕩
  off←0
  Next ← {r←𝕩↑off↓s ⋄ off+↩𝕩 ⋄ r}
  Read ← { 𝕊:
    type ← ⊑Next 1
    type◶⟨
      {𝕊:
        et ← ⊑Next 1
        rank ← ⊑8‿32•bit._cast Next 4
        shape ← 8‿64•bit._cast Next rank×8
        shape⥊{
          et=8? Read¨ ↕×´shape;
          ⟨8, et⊑eltype2bit⟩•bit._cast Next (⌈÷⟜8)⍟(0=et) (et⊑eltypeWidth)×´shape
        }
      }
      {𝕊: ⊑⟨8, 64    ⟩•bit._cast Next 8}
      {𝕊: ⊑⟨8, 32‿'c'⟩•bit._cast Next 4}
    ⟩@
  }
  r ← Read@
  off=≠s? r;
  ! "invalid input: data after end"
}