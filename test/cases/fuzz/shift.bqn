(
  # bitarr »˘ / «˘
  _test ← {𝔽_𝕣𝕩:
    a←(1+2•rand.Range 100)•rand.Range 2
    v←•rand.Range 2
    v‿a ! (v 𝔽○•internal.Keep˘ a) ≡ v 𝔽˘ a
  }
  » _test¨ ↕10000
  « _test¨ ↕10000
)
