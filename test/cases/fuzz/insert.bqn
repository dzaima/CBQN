%DEF rand r ← (⊑•args).GetRand@

%USE rand ⋄ ≢{! (+˝ ≡ +○•internal.Keep˝) (1‿0+𝕩)r.Range 0}¨ ↕100‿100
%USE rand ⋄ ≢{! (⌊˝ ≡ ⌊○•internal.Keep˝) (1‿0+𝕩)r.Range 0}¨ ↕100‿100
%USE rand ⋄ ≢{! (⌈˝ ≡ ⌈○•internal.Keep˝) (1‿0+𝕩)r.Range 0}¨ ↕100‿100
