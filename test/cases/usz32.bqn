!"Out of memory" % a←(2⋆20) ⥊ 1e9 ⋄ ≠•internal.Keep {𝕊: a}˘ a
!"⥊: 𝕨 too large" % a←(2⋆20) ⥊ 1e9 ⋄ ≠•internal.Keep a˘ a
!"Out of memory" % ≠•internal.Keep (2⋆25)/(2⋆10)⥊10
!"Out of memory" % k←16384 ⋄ n←k÷˜2⋆32 ⋄ ≠•internal.Keep (n⥊k) / n⥊1
!"Out of memory" % k←64 ⋄ n←k÷˜2⋆32 ⋄ ≠•internal.Keep (n⥊k) / n⥊1 # %SLOW
!"Out of memory" % ≠•internal.Keep ∾˜⍟31 ↕2 # %SLOW
