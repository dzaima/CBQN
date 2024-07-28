NextIdx ← {i←/𝕩 ⋄ (i∾≠𝕩)⊏˜i⍋𝕨} # for each index 𝕨, find the index of the next 1 in mask 𝕩
UM ← {(≠𝕨)↑/⁼𝕩}
Fail ← {•Out 𝕩 ⋄ •Exit 1}
Trim ← {((∨`⌾⌽∧∨`)𝕩≠' ')/𝕩}

{ 𝕊:
  •Out "Usage: test/run.bqn [options | files]"
  •Out "Options:"
  •Out "  lint             Check that every test either is expected to return a specific value, error, or contains a '!'"
  •Out "  slow             Enable slow tests"
  •Out "  noerr            Disable erroring tests (ones expected to, or which contain ⎊)"
  •Out "  heapverify       Disable tests that aren't runnable in heapverify"
  •Out "  debug            Disable tests that aren't runnable in debug"
  •Out "  no-catch         Disable catching errors in tests not expected to error"
  •Out "  update-messages  Auto-update error messages in tests"
  •Out "  ignore-messages  Don't warn about incorrect error messages"
  •Out "  bin-search       Binary search for some property. Next argument should be /[01]*/, with a 1 appended every time the property is matched, and a 0 otherwise"
  •Exit 0
}⍟⊢ (0=≠•args) ∨´ "help"‿"h"‿"?"∊'-'⊸≠⊸/¨•args

o ← {
  args ← •args
  search ⇐ @
  {
    n←»𝕩
    search ↩ '0'-˜⊑(n/args)∾<""
    ! ∧´search∊0‿1
    search ↩ search∾1
    args ↩ (¬n∨𝕩)/args
  }⍟(∨´) "bin-search"⊸≡¨ args
  named ← "run"‿"lint"‿"update-messages"‿"slow"‿"ignore-messages"‿"noerr"‿"heapverify"‿"debug"‿"no-catch"
  i ← named⊐args
  run    ⇐ ∨´i=0
  lint   ⇐ ∨´i=1
  update ⇐ ∨´i=2
  slow   ⇐ ∨´i=3
  errm   ⇐¬∨´i=4
  noerr  ⇐ ∨´i=5
  heapverify ⇐ ∨´i=6
  debug      ⇐ ∨´i=7
  noCatch    ⇐ ∨´i=8
  noerr∨↩ heapverify
  files ⇐ (i=≠named)/args
  run ↩ update ∨ ¬∨´lint‿update
}

_catch_ ← {o.noCatch? 𝔽; 𝔽⎊𝔾}

searchRefine ← {
  searchI ← 0
  {
    o.search≡@? 𝕩;
    searchI≡≠o.search? 𝕩;
    1≡≠𝕩? 𝕩;
    SearchRefine (⌊2÷˜≠𝕩) ((searchI+↩1) ⊢ searchI⊑o.search)◶↓‿↑ 𝕩
  }
}

exitCode ← 0
Run ← { 𝕊 testname:
  lns ← •FLines fullfile ← "cases"•file.At filename ← testname∾".bqn"
  defk‿defv ← ⟨"IS_HEAPVERIFY"⟩‿⟨"heapverify←"∾•Repr o.heapverify⟩
  i ← 0
  Pos ← {∾⟨filename, ":", •Repr 1+𝕩⟩}
  Next ← {𝕊: (i+↩1) ⊢ i⊑lns}
  Process ← {
    "%DEF "≡5↑𝕩?
      defk‿defv∾¨↩<¨ {(Process 1↓𝕩).str}⌾(1⊸⊑) ⊑∘⊐⟜' '⊸(↑⋈↓) 5↓𝕩
      @
    ;
      "":@
    ;
      '#'≡⊑𝕩? @
    ;
      "("≡𝕩?
        Process ∾ ∾⟜(@+10)¨ ¯1↓ ∾⟜<⟜Next•_while_{")"≢¯1⊑𝕩} ⟨Next@⟩
    ;
      # insert defs
      si ← /(≠𝕩)↑"%USE"⍷𝕩
      mi ← si+5
      ei ← mi NextIdx ¬𝕩∊'_'∾∾"0aA"+⟜↕¨10‿26‿26
      g ← (+`+´ 𝕩⊸UM¨ si‿mi‿ei) ⊔ 𝕩
      pre ← ⊑g
      [·,uk,post] ← ⍉↑‿3⥊1↓g
      {Fail (Pos i)∾": Unknown def "∾⊑𝕩⊏uk}⍟≠ /¬uk∊defk
      str ← pre ∾ ∾((defk⊐uk)⊏defv) ∾¨ post
      
      Flag ← {
        m ← (≠str)↑ 𝕩⍷str
        str /˜↩ ¬ ∨⟜»⍟(¯1+≠𝕩) m
        ∨´m
      }
      
      {ln⇐i-1 ⋄ fast⇐¬Flag"%SLOW" ⋄ debug⇐¬Flag"%NDEBUG" ⋄ heapverify⇐¬Flag"%NHEAPVERIFY" ⋄ str⇐str}
  }
  tests ← SearchRefine @⊸≢¨⊸/ ∾⟜<⟜(Process∘Next)•_while_{𝕊: i≠≠lns} ⟨⟩
  {𝕊: •Out "Narrowed down to test "∾Pos (⊑tests).ln}⍟⊢ (o.search≢@)∧1=≠tests
  
  
  tests { 𝕊ns: ⟨ln,str⟩←ns
    idx ← ⊑str⊐'%'
    {
      idx ≡ ≠str? ⟨ns, "eval", str⟩;
      (i2←idx+1)<≠str? '%'≡i2⊑str? ⟨ns, "equal", idx↑str, (2+idx)↓str⟩;
      {𝕊: Fail (Pos ln)∾": Unknown test format"}⍟⊢ '!'≢⊑str
      ⟨ns, "error", (idx+1)↓str, 1↓idx↑str⟩
    }
  }¨↩
  
  { 𝕊 d:
    ln←(⊑d).ln
    ty←1⊑d
    Lint ← {𝕊: •Out ∾⟨"Lint: ", Pos ln, ": ", 𝕨⟩}⍟(¬⊢)
    {
      "eval"≡ty? "No check or assert" Lint ∨´'!'=2⊑d;
      @
    }
  }¨⍟o.lint tests
  
  { 𝕊:
    badCount ← 0
    badMessage ← 0
    currLn ← 0
    skipped ← 0
    dir ← •file.At "testDir"
    •file.CreateDir⍟(¬•file.Exists) dir
    Bad ← {
      ""𝕊𝕩;
      •Out ∾⟨Pos currLn, ": ", 𝕩⟩
      •Out¨ "  "⊸∾¨ 𝕨
      badCount+↩ 1
    }
    ErrMsg ← {𝕊: m←•CurrentError@ ⋄ {1==m? ∧´2=•Type¨m? m; •Repr⎊"(unrepresentable)" m}}
    Eval ← {⟨dir, ∾⟨testname,"_line_",•Repr currLn+1,".bqn"⟩, ⟨"arg0",1⟩⟩ •BQN 𝕩}
    EvalS ← •BQN⎊{𝕊: Bad "Bad comparison value" ⋄ "(bad)"}
    toRun ← tests
    toRun {{𝕩.fast      }∘⊑¨⊸/𝕩}⍟(¬o.slow)↩
    toRun {{𝕩.debug     }∘⊑¨⊸/𝕩}⍟o.debug↩
    toRun {{𝕩.heapverify}∘⊑¨⊸/𝕩}⍟o.heapverify↩
    skipped+↩ (≠tests)-≠toRun
    results ← {
      "eval"‿str: {
        o.noerr? ⊑'⎊'∊str? skipped+↩1;
        Eval _catch_ {𝕊: Bad "Expected success, got error: "∾ErrMsg@} str
        @
      };
      "error"‿str‿exp: {
        o.noerr? skipped+↩1;
        ok‿got ← (1⋈Eval)⎊(0⋈ErrMsg) str
        exp ↩ EvalS⍟{×≠Trim𝕩} exp
        {
          (got≡exp) ∧ ¬ok? @;
          ok? Bad "Expected error, but didn't get one" ⋄ @;
          ⟨"Expected: "∾exp ⋄ "Got:      "∾got⟩ Bad⍟o.errm "Wrong error message"
          badMessage+↩ 1
          got
        }
      };
      "equal"‿str‿exp: {
        o.noerr? ⊑'⎊'∊str? skipped+↩1;
        ok‿got ← (1⋈Eval)_catch_(0⋈ErrMsg) str
        exp ↩ EvalS exp
        {
          ¬ok? Bad "Expected value, but got error: "∾got;
          got •internal.EEqual exp? @;
          ⟨"Expected: "∾•Repr exp ⋄ "Got:      "∾•Repr⎊•Show got⟩ Bad "Wrong value"
          @
        }
        @
      }
    }∘{currLn↩(⊑𝕩).ln ⋄ 1↓𝕩}¨ toRun
    
    {𝕊:
      res‿desc ← ({@≡𝕩? 0; (¬⊑(@+10)∊𝕩) ∧ ¬∨´"TEST FAIL"⍷𝕩}¨ results)⊸/¨ results‿toRun
      0≠≠res?
        •Out ∾⟨"Update ", •Repr≠res, " entr", (1<≠res)⊑"y"‿"ies", " in ", filename, "?"⟩
        {
          ¬⊑"yY"∊˜⊑'n'∾˜•GetLine@? •Out "Not updating.";
          [idxs, upd] ← ⍉>res {new𝕊⟨⟨ln⟩, "error", str, ·⟩: ln ⋈ ∾⟨"!", •Repr new, " ", ⊑∘⊐⟜'%'⊸↓ ln⊑lns⟩}¨ desc
          "Refusing to write over changed file" ! lns ≡ •FLines fullfile
          fullfile •FLines lns {×≠𝕩? ⋈⁼𝕩; 𝕨}¨ (≠lns)↑idxs⊔upd
          •Out "Updated."
        }
      ;@
    }⍟⊢ o.update
    skipm ← (×skipped)/∾⟨" (", •Repr skipped, " skipped)"⟩
    ran ← (≠tests)-skipped
    {
      0=badCount? •Out filename∾": "∾(•Repr ran)∾" passed!"∾skipm;
      •Out ∾⟨•Repr ran-badCount, '/', •Repr ran, skipm⟩ ∾ ((¬o.errm)∧×badMessage)/∾⟨"; ", •Repr badMessage, " mismatched error message", (1≠badMessage)/"s"⟩∾skipm
      exitCode ↩ 1
    }
  }⍟o.run @
}
Run¨ SearchRefine o.files

•Exit⍟(0⊸≠)⍟(¬o.heapverify) exitCode