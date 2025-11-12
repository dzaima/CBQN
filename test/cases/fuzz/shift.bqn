(
  # bitarr »˘ / «˘
  r ← (⊑•args).GetRand@
  _test ← {𝔽_𝕣𝕩:
    a ← (1+2 r.Range 100) r.Range 2
    v ← r.Range 2
    v‿a ! (v 𝔽○•internal.Keep˘ a) ≡ v 𝔽˘ a
  }
  » _test¨ ↕10000
  « _test¨ ↕10000
)
