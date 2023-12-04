%DEF base (
  ⟨V⇐Variation, U⇐Unshare, LV⇐ListVariations ⋄ CLR⇐ClearRefs, GC⟩ ← •internal
  UseGC ← {𝕊: 𝕨 ⋄ 𝕩 ⋄ GC@ ⋄ @}
  _tvaru ← {
      F _𝕣 x:        {           F 𝕩 V U¨ x ⋄ CLR@}¨ LV 𝕩;
    w F _𝕣 x: (LV 𝕨) {(𝕨 V U¨ w) F 𝕩 V U¨ x ⋄ CLR@}⌜ LV 𝕩
  }
)

%USE base ⋄ UseGC¨_tvaru ↕⋈4
%USE base ⋄ UseGC¨_tvaru˜ ↕⋈4
%USE base ⋄ (↕4)⊸(UseGC¨)_tvaru ↕⋈4
%USE base ⋄ (UseGC¨)⟜(↕4)_tvaru ↕⋈4

%USE base ⋄    UseGC` _tvaru ↕⋈5
%USE base ⋄ 1⊸(UseGC`)_tvaru ↕⋈5
%USE base ⋄       UseGC`_tvaru ↕4‿2
%USE base ⋄ (↕⋈2) UseGC`_tvaru ↕4‿2

%USE base ⋄    UseGC´ _tvaru ↕⋈4
%USE base ⋄ @⊸(UseGC´)_tvaru ↕⋈4

%USE base ⋄    UseGC˝ _tvaru ↕⋈4
%USE base ⋄ @⊸(UseGC˝)_tvaru ↕⋈4
%USE base ⋄        UseGC˝ _tvaru ↕4‿2
%USE base ⋄ (↕⋈2) (UseGC˝)_tvaru ↕4‿2
