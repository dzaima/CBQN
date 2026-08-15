%DEF test bin←(⊑•args).Bin@ ⋄ Test←{{132: 1; 𝕩}⌾⊑ {(n←2+≠bin)≤≠𝕩? (bin∾": ")≡n↑𝕩? "(self-bin): "∾n↓𝕩; 𝕩}⌾(2⊸⊑) {'␊'¨⌾((𝕩=@+10)⊸/)𝕩}¨⌾(1⊸↓) (𝕨{s𝕊𝕩: stdin⇐s; ⇐}@) •SH ⟨bin⟩∾𝕩} ⋄ lf←@+10

# these tests of course have an approximately 2⋆¯58 chance chance of failing in case random seeds collide
%DEF test_flag (
  %USE test
  TestX ← { 𝕊 val‿exp‿name‿expr:
    vs ← {c‿o‿e ← Test 𝕩∾"-p"‿expr ⋄ o‿e!c≡0 ⋄ o‿e}¨ {@: ⟨⟩; ⟨"-X"∾name∾"="∾𝕩⟩}¨ @‿@ ∾ val
    vs ! exp ≡ ≡⌜˜ ⊑¨ vs
  }
)
%USE test_flag ⋄ TestX ⟨"1"‿"2"‿"2"‿"3"‿"u"‿"u", =⌜˜"AB122345", "rand-seed", "8 •rand.Range 0"⟩
%USE test_flag ⋄ TestX ⟨"1"‿"2"‿"2"‿"3"‿"u"‿"u", =⌜˜"AB122345", "hash-seed", "•Hash¨↕3"⟩
%USE test_flag ⋄ TestX ⟨"1"‿"2"‿"2"‿"3"‿"u"‿"u", =⌜˜ 'a'¨⍟(¬(⊑•args).randomizedHeuristics) "AA122345", "internal-seed", "{𝕊:≠•internal.Type 2/↕⋈6}¨↕100"⟩

(
  %RAND_HEURISTICS
  %USE test
  Get ← {𝕊: •internal.Keep {𝕊:≠•internal.Type 2/↕⋈6}¨ ↕•internal.Keep 100÷2}
  Get¨ ↕100 # warmup
  r0 ← Get@
  k1←1 •internal.Temp @ ⋄ r1 ← Get@
  1 •internal.Temp k1   ⋄ r2 ← Get@
  1 •internal.Temp k1   ⋄ r3 ← Get@
  r4 ← Get@
  1 •internal.Temp 123 ⋄ r5 ← Get@
  1 •internal.Temp 123 ⋄ r6 ← Get@
  exp ← =⌜˜"abbbcdd"
  ! exp ≡ ≡⌜˜r0‿r1‿r2‿r3‿r4‿r5‿r6
)
