# build/build replxx singeli native g debug f=-DEACH_FILLS f=-DTEST_CELL_FILLS && build/build replxx singeli native g debug heapverify f=-DTEST_CELL_FILLS OUTPUT=BQNh && ./BQN test/cells.bqn path/to/mlochbaum/BQN/bqn.js ./BQNh
# (replace bqn.js path with "reuse" to use the previous result (requires the test set to not have changed))
{𝕊: •Out "Usage: ./nonHeapverifyCBQN test/cells.bqn path/to/bqn.js heapverifyCBQN" ⋄ •Exit 0}⍟⊢ 2>≠•args
a0 ← ⊑•args
bqnjs‿hCBQN ← •wdpath⊸•file.At¨ •args
bqnjsFile ← •file.At "generated/cells_bqnjsOut.bqn"
hCBQNFile ← •file.At "generated/cells_hCBQNOut.bqn"
validFile ← •file.At "generated/cells_valid.bqn"

cbqni ← {⊑"internal"<⊸∊•listsys? •BQN"•internal"; @}
bqntype ← {cbqni≡@? 2; "harr"≡cbqni.Type ⊢⎉0 ""} # 0: full CBQN; 1: heapverify CBQN; 2: bqn.js

SHRun ← {
  f←•BQN"•SH"
  c‿o‿e ← F 𝕩
  g←•BQN"•term.OutRaw‿•term.ErrRaw {𝕎•ToUTF8𝕩}¨ ⊢" ⋄ G o‿e
  c≢0? •Exit c; @
}

GetTests ← { 𝕊:
  
  tests ← ⟨⟩
  AddTests ← {tests∾↩ 𝕩 ⋄ @}
  
  Prod ← {⥊ (<⟨⟩) <⊸∾⌜´ 𝕩}
  FOk ← { w‿F‿g‿x: # filter out tests that'd immediately error due to bad shapes, or would 
      g ↩ (@≡g)⊑g‿¯1
      r ← =¨w‿x
      s ← ≢¨w‿x
      k ← 0⌈r-0⌈(⊢+r×0⊸>) 1↓⌽3⥊⌽⥊g
      # •Show k
      ok ← ≡´ (⌊´k) ↑¨ s
      # {𝕊: ! w 0∘(F⎉g)⎊1 x}⍟¬ ok
      ok ∧ k≢0‿0
  }¨⊸/
  
  # generating random numerical arrays:
  # )r N←{⟨⟩:"1";•Repr⊑⍟(1=≠)𝕩} ⋄ J←{(⊣∾𝕨∾⊢)´𝕩} ⋄ •Out '⟨'∾'⟩'∾˜','J ("⥊"J≢⋈○N⥊)¨ 15↑ ⍷ {𝕊: ((1+•rand.Range 4)•rand.Range 4)•rand.Range 10}¨ ↕100
  # xs ↩ ⟨0,↕0⟩ ∾ 
  
  { 𝕊: # F⎉k x
    fs ← ⟨<,⊏,⊑,≍,⥊,«,»,⍉,×˝,¨,1˙,∞,'a'˙,'𝕨',"ab"‿2˙,⟨⟩,<"ab"‿1,2‿3,↕2‿3‿1⟩
    xs ← ⟨
      1, <2,
      2‿3, 2‿3‿4⥊↕24, 1‿1‿1‿1⥊4,
      ↕0, 0‿2⥊"", 4‿0⥊0, 0‿2‿3‿4‿5⥊0,
      1‿1‿2‿1⥊2‿'a', 2‿2⥊"abcd", ↕2‿3,
    ⟩
    ks ← ⟨@, ¯∞, ¯1, 0‿∞‿∞, 10‿1, ⟨2⟩, 3, 4, 5, ∞⟩
    AddTests Prod ⟨fs, ks, xs⟩
  }⍟⊢ 1

  { 𝕊: # w ⋈⎉k x
    xs ← ⟨0, 2‿2⥊"a"‿1⟩∾ {𝕩⥊↕×´⥊𝕩}¨ ⟨1, 2‿3, 2‿0, 2‿3‿4, 2‿2‿3‿4, 3‿4⟩
    ks ← ⟨¯∞, ¯1, 0, 1, 2, 3, 4, ∞‿0‿1, ¯∞‿2‿1, ∞, 0‿∞, ∞‿0, 1‿∞, ∞‿1, 2‿∞, ∞‿2⟩
    AddTests Prod ⟨xs, ⋈, ks, xs⟩
  }⍟⊢ 1
  
  { 𝕊: # w ⊏⎉k x / w ⊑⎉k x
    ws ← ⟨0, 1, 2, 3, ¯1, ¯2, <1⟩
    xs ← ⟨2‿2⥊"a"‿1‿"b"‿"cd", 3‿3‿3⥊'a'+↕27, 3‿6⟩∾ {𝕩⥊↕×´⥊𝕩}¨ ⟨2‿3, 2‿0, 2‿3‿4, 2‿2‿3‿4, 3‿4⟩
    ks ← ⟨0, 1, 2, 3, 4, 0‿1, 0‿2⟩
    AddTests FOk Prod ⟨ws, ⊏‿⊑, ks, xs⟩
    AddTests FOk Prod ⟨0‿100‿¯100‿∞, ⊏‿⊑, 0‿1‿2, xs∾⟨0,<4⟩⟩
  }⍟⊢ 1
  
  { 𝕊: # w ≡⎉k x
    xs ← ⟨0, 2‿3⥊@+↕6⟩∾ {𝕩⥊↕×´⥊𝕩}¨ ⟨1, 2‿3, 0‿3, 2‿0‿3, 4‿2‿3, 2‿4‿2‿3, 3‿1⟩
    ks ← ⟨¯∞, ¯1, 0, 1, 2, 3, 4, 0‿1, 2‿1⟩
    AddTests FOk Prod ⟨xs, ≡, ks, xs⟩
    xs2 ← ⟨2‿3⥊175, [¯81‿¯81‿¯81, 0‿0‿0]⟩∾{𝕩-˜𝕩+2‿3⥊'¯'}¨ 0‿100‿100000
    AddTests FOk Prod ⟨xs2, ≡, ks, xs2⟩
  }⍟⊢ 1
  
  { 𝕊: # w ⍉⎉k x
    ws ← ⟨0, 1, 2, 3, 4, ¯1, 1‿0⟩
    xs ← ⟨2‿2‿2⥊"a"‿1‿"b", 3‿3‿3⥊'a'+↕27, 3‿6⟩∾ {𝕩⥊↕×´⥊𝕩}¨ ⟨2‿3, 2‿0, 2‿3‿4, 2‿2‿3‿4, 3‿4, 3‿2‿1‿2‿2⟩
    ks ← ⟨0, 1, 2, 3, 4, 5, 0‿1, 0‿2⟩
    AddTests FOk Prod ⟨ws, ⍉, ks, xs⟩
  }⍟⊢ 1
  
  { 𝕊: # w »⎉k x / w «⎉k x
    ws ← ⟨0, 1, 'a', "ab", <"h"‿2⟩
    xs ← ⟨2‿2‿2⥊"a"‿1‿"b", 3‿3‿3⥊'a'+↕27, 3‿6⟩∾ {𝕩⥊↕×´⥊𝕩}¨ ⟨2‿3, 2‿0, 2‿3‿4, 2‿2‿3‿4, 3‿4, 3‿2‿1‿2‿2⟩
    ks ← ⟨0, 1, 2, 3, 4, @, 0‿1, 0‿2⟩
    AddTests FOk Prod ⟨ws, «‿», ks, xs⟩
  }⍟⊢ 1
  
  { 𝕊: # w ↑⎉k x / w ↓⎉k x
    ws ← ⟨0,1,2,¯1,¯2,10,1‿1⟩
    xs ← ⟨2‿2‿2⥊"a"‿1‿"b", 3‿3‿3⥊'a'+↕27, 3‿6⟩∾ {𝕩⥊↕×´⥊𝕩}¨ ⟨2‿3, 0‿2, 2‿0, 2‿3‿4, 2‿2‿3‿4, 3‿4, 3‿2‿1‿2‿2⟩
    ks ← ⟨0, 1, 2, 3, 4, @, 0‿1, 0‿2, 1‿¯1⟩
    AddTests FOk Prod ⟨ws, ↑‿↓, ks, xs⟩
  }⍟⊢ 1
  
  { 𝕊: # w -⎉k x
    xs ← {𝕩⥊↕×´⥊𝕩}¨ ⟨2‿3‿4, 2‿3, 3‿4, 2, 3, 4, 2‿2‿3‿4, 3‿2‿1‿2‿2⟩
    ks ← ⟨0, 1, 2, 3, 4, ¯1, ¯2, 0‿1, 0‿2, 1‿0, 2‿0, 2‿1, 1‿2⟩
    AddTests FOk Prod ⟨xs, -, ks, xs⟩
    AddTests Prod ⟨xs, -, ∞, xs⟩ # test regular leading axis behavior
  }⍟⊢ 1
  
  tests
}

getFill ← {bqntype≢2? cbqni•ns.Has"hasfill"? {cbqni.HasFill 𝕩? ⊑1↑0⥊𝕩; '!'}; {1 ⊑∘↑⎊'!' 0⥊𝕩}}
Info ← {⟨𝕩, GetFill 𝕩⟩}
RunTests ← {
    f‿k‿x:   (Info F{k≡@? 𝔽˘; 𝔽⎉k})⎊"err" x;
  w‿f‿k‿x: w (Info F{k≡@? 𝔽˘; 𝔽⎉k})⎊"err" x
}¨


{
  "0"≡a0? # bqn.js
     •Out⍟(bqntype≠2) "Warning: doesn't seem like the first argument is bqn.js!"
    res ← RunTests GetTests@
    bqnjsFile •FChars •Repr res
  ;
  "1"≡a0? # heapverify
    •Out⍟(bqntype≠1) "Warning: doesn't seem like the second argument is CBQN with heapverify!"
    tests ← GetTests@
    valid ← '0'-˜•FChars validFile
    ok ← valid≠2
    HTests ← { 𝕊:
      okTests ← ok/tests
      hasFill ← 0=ok/valid
      cbqni.Temp 2 ⋄ f1 ← RunTests okTests /˜ hasFill
      cbqni.Temp 1 ⋄ f0 ← RunTests okTests /˜ ¬hasFill
      cbqni.Temp 0
      (⍋⍋hasFill) ⊏ f0∾f1
    }
    hCBQNFile •FChars •Repr HTests⌾(ok⊸/) "err"¨ tests
  ;
  # base
    •Out⍟(bqntype≠0) "Warning: doesn't seem like the running binary is CBQN without heapverify!"
    
    tests ← GetTests@
    •Out ∾⟨"Running ", •Repr ≠tests, " tests on base CBQN.."⟩
    cfe‿fo ← <˘⍉>{cbqni.Temp@ ⋄ (cbqni.Temp@) ⋈ ⊑RunTests ⋈𝕩}¨ tests
    validFile •FChars '0'+ 2⌊ cfe + 2×"err"⊸≡¨ fo # 2: errored; 1: invalid fill; 0: valid fill
    
    { 𝕊:
      •Out "Calling bqn.js.."
      •file.Remove⍟•file.Exists bqnjsFile
      SHRun ⟨bqnjs,  •file.At "cells.bqn", "0", ""⟩
      {•file.Exists bqnjsFile?@; •Out "bqn.js didn't produce the result file!" ⋄ •Exit 1}
    }⍟⊢ "reuse"≢a0
    
    •file.Remove⍟•file.Exists hCBQNFile
    •Out "Calling heapverify build.."
    SHRun ⟨hCBQN, •file.At "cells.bqn", "1", ""⟩
    
    •Out "Comparing.."
    bo ← •Import bqnjsFile
    ho ← •Import hCBQNFile
    
    hCount ← +´ "err"⊸≢¨ bo
    badH←0 ⋄ hBadFill←0 ⋄ hSuperset←0
    badB←0 ⋄ bBadFill←0 ⋄ bSuperset←0
    
    hBadEmpty←0
    hBadButBase←0
    
    FRepr ← {0≢≠⥊𝕩? •Repr 𝕩; '!'≢f←GetFill 𝕩? ∾⟨•Repr≢𝕩, "⥊", "<"⊸∾⍟(0≠≡f) •Repr f⟩; •Repr 𝕩}
    FTest ← { test‿exp‿got:
      ! exp≢got
      ∾⟨
        {F‿k‿x←¯3↑𝕩 ⋄ ∾⟨{3≡≠𝕩?""; "("∾") "∾˜FRepr⊑𝕩}𝕩 , •Repr f, {k≡@?"˘";"⎉"∾•Repr k}, " ", FRepr x⟩} test
        ": "
        {
          exp≡"err"? "expected error, but got "∾•Repr ⊑got;
          got≡"err"? "expected result, but got error";
          {𝕨∾"; "∾𝕩}´ @⊸≢¨⊸/ "result"‿"result fill" {m𝕊e‿g: {e≡g? @; ∾⟨"expected ",m," to be ",FRepr e,", got ",FRepr g⟩}}¨ <˘⍉>⟨exp,got⟩
        }
      ⟩
    }
    
    { 𝕊 t‿f‿b‿h:
      {
        b≡f?@;
        b≢○⊑f? badB+↩1 ⋄ •Out ∾⟨"base: incrorrect result: ",FTest t‿b‿f⟩;
        '!'≡1⊑b? bSuperset+↩1;
        bBadFill+↩1 ⋄ •Out ∾⟨"base: bad fill: ",FTest t‿b‿f⟩
      }
      {
        b≡h?@; {∨´0=≠∘⥊¨ (¯3↓t)∾¯1↑t? hBadEmpty+↩1;@}
        f≡h? hBadButBase+↩1;
        b≢○⊑h? badH+↩1 ⋄ •Out ∾⟨"heapverify: incrorrect result: ",FTest t‿b‿h⟩;
        '!'≡1⊑b? hSuperset+↩1;
        hBadFill+↩ 1 ⋄ •Out ∾⟨"heapverify: bad fill: ",FTest t‿b‿h⟩
        
        # ∨´0=≠∘⥊¨ (¯3↓t)∾¯1↑t? hBadEmpty+↩1;
        # badH+↩1 ⋄ •Out ∾⟨"heapverify: incrorrect result: ",FTest t‿b‿h⟩
      }
    }¨ <˘⍉>⟨tests, fo, bo, ho⟩
    
    •Out ∾⟨
      "Base: ",
      •Repr ≠tests, " tests: "
      •Repr badB, " wrong results, "
      •Repr bBadFill, " wrong fills, "
      •Repr bSuperset, " superset fills"
    ⟩
    hBadTot ← hBadFill
    •Out ∾⟨
      "Heapverify: "
      •Repr hCount, " tests: "
      •Repr hBadButBase, " wrong results equal to base, "
      •Repr badH, " wrong results, "
      •Repr hBadFill, " wrong fills, "
      •Repr hSuperset, " superset fills; "
      •Repr hBadEmpty, " wrong results had empty input(s)"
    ⟩
    •Exit⍟(∨´0≠badH‿badB) 1
    •Out "Passed!"
}
