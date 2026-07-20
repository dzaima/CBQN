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
  •Out "  update-values    Auto-update test results (might want to reformat them afterwards though)"
  •Out "  ignore-messages  Don't warn about incorrect error messages"
  •Out "  bin-search       Binary search for some property. Next argument should be /[01]*/, with a 1 appended every time the property is matched, and a 0 otherwise"
  •Out "  seed=123         Random seed for tests that care (default=1; seed=u will choose a random seed)"
  •Out "  bin=./BQN        For CLI tests, the binary to use"
  •Exit 0
}⍟⊢ (0=≠•args) ∨´ "help"‿"h"‿"?"∊'-'⊸≠⊸/¨•args

⟨EEqual, HasFill⟩ ← •internal
indistinguishable ← {⟨I⇐Indistinguishable⟩: I; EEqual} •internal
withFill ← {⟨R⇐WithFill⟩: R; {𝕩⌾((1¨≠𝕩)⊸/) (≢𝕩)⥊<𝕨}} •internal
FillOf ← HasFill◶'e'‿{⊑1↑0⥊𝕩}

iRepr0 ← {
  LE ← {𝕊:'⟩'∾˜ '⟨'⌾⊑ ∾','∾¨ ⊑∘IRepr0¨ 𝕩}
  GWF ← {𝕊: se←IReprP 𝕨 ⋄ ∾⟨se, ' '⥊˜¬"⟩)""'"∊˜¯1⊑se, "WF", 𝕩⟩}
  
  {
    0≢•Type 𝕩? 0⋈˜ •Repr 𝕩;
    sh←≢𝕩 ⋄ 𝕩⥊↩
    np‿shx‿expr ← (FillOf 𝕩) {
      0  𝕊𝕩: ∧´1=•Type¨𝕩? {sh≡⟨⟩? ⟨1,⟨⟩,"<"∾⊑𝕩⟩; ⟨⟩: 1‿⟨0⟩‿"↕0"; ⟨v⟩: ⟨1,⟨1⟩,"⋈"∾v⟩; ⟨0,≢𝕩,1↓∾'‿'∾¨ 𝕩⟩} •Repr¨ 𝕩;
      ' '𝕊𝕩: ∧´2=•Type¨𝕩? {sh≡⟨⟩? ⟨1,⟨⟩,"<'"∾𝕩∾"'"⟩; ⟨0, ≢𝕩, '"' (⊣∾((1+=)/⊢)∾⊣) 𝕩⟩} 𝕩;
      'e'𝕊⟨⟩: 0‿⟨0⟩‿"⟨⟩";
      𝕨  𝕊⟨⟩: {sh≡⟨1⟩? ⟨1,⟨1⟩,"⋈"∾𝕩⟩; ⟨1,⟨⟩,"<"∾𝕩⟩} IRepr 𝕨;
      'e'𝕊𝕩: nwof←{∧´1=𝕩?1;∧´2=𝕩?1;0} •Type¨ 𝕩 ⋄ ⟨nwof, ≢𝕩, "WOF"⊸∾⍟nwof LE𝕩⟩;
      𝕨𝕊⟨e⟩: 𝕨 Indistinguishable FillOf <e? {sh≡⟨1⟩? ⟨1,⟨1⟩,"⋈"∾𝕩⟩; ⟨1,⟨⟩,"<"∾𝕩⟩} IRepr e;
      𝕨  𝕊𝕩: ⟨⟩≡sh? ⟨1, ⟨⟩, 𝕨 GWF "<"∾ IRepr ⊑𝕩⟩;
      𝕨  𝕊𝕩: ⟨1, ≢𝕩, 𝕨 GWF LE𝕩⟩
    } 𝕩
    {shx≡sh? expr‿np; 1⋈˜ ∾⟨•Repr ⊑⍟(1=≠) sh,"⥊",expr⟩}
  }
}
IRepr ← ⊑IRepr0
IReprP ← {v‿0: v; v‿1: '('∾v∾')'} IRepr0

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
  
  named ← "run"‿"lint"‿"update-messages"‿"slow"‿"ignore-messages"‿"noerr"‿"heapverify"‿"debug"‿"no-catch"‿"seed"‿"update-values"‿"bin"
  i ← named⊐{𝕩↑˜⊑𝕩⊐'='}¨ args
  run    ⇐ ∨´i=0
  lint   ⇐ ∨´i=1
  updMsgs⇐ ∨´i=2
  updVals⇐ ∨´i=10
  slow   ⇐ ∨´i=3
  errm   ⇐¬∨´i=4
  noerr  ⇐ ∨´i=5
  heapverify ⇐ ∨´i=6
  debug      ⇐ ∨´i=7
  noCatch    ⇐ ∨´i=8
  seed ⇐ {
    ⟨⟩: 1;
    ⟨"u"⟩: s←•rand.Range 2⋆31 ⋄ •Out "seed="∾•Repr s ⋄ s;
    ⟨s⟩: •ParseFloat s
  } 5↓¨ (i=9)/args
  bin ⇐ 4↓¨ (i=11)/args
  
  ⟨
    usz
    properFills
    eachFills
    randomizeHeuristics
    knownHeapverify
    knownDebug
    hasTestUtils
  ⟩ ⇐ 32‿0‿0‿0‿0‿0‿0 {(≠𝕨)↑ 𝕩 ∾ (≠𝕩)↓𝕨} {⟨P⇐Properties⟩: @P@; ⟨⟩} •internal
  
  noerr∨↩ heapverify
  updMsgs∧↩ ¬heapverify
  files ⇐ (i=≠named)/args
  run ↩ updVals ∨ updMsgs ∨ ¬∨´lint‿updMsgs
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
  filename ← testname
  filename ".bqn"⊸(∾˜⍟(⊣≢-∘≠⊸↑))↩
  filename "test/cases/"⊸(≠⊸↓⍟(⊣≡≠⊸↑))↩
  lns ← •FLines fullfile ← "cases"•file.At filename
  {
    1≤≠lns?
    "### WHOLE-FILE-TEST" ≡ ⊑lns?
    lns ↩ ⟨"("⟩∾(1↓lns)∾⟨")"⟩
    ;@
  }
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
      '#'≡⊑𝕩? ¬⊑(@+10)∊𝕩? @
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
      
      {
        enabled ⇐ ¬ (¬o.slow) ∧ Flag "%SLOW"
        EnabledBy ← {enabled∧↩𝕩} {⟨Flag "%"∾𝕩, Flag "%!"∾𝕩⟩}⊸{0‿0𝕊𝕩: 1; 1‿0𝕊𝕩: 𝕩; 0‿1𝕊𝕩: ¬𝕩}
        "DEBUG" EnabledBy o.debug
        "HEAPVERIFY" EnabledBy o.heapverify
        "PROPER_FILLS" EnabledBy o.properFills
        "EACH_FILLS" EnabledBy o.eachFills
        "USZ32" EnabledBy o.usz=32
        "USZ64" EnabledBy o.usz=64
        ! ∨´ o.usz=32‿64
        enabled∧↩ ¬ o.noerr ∧ (⊑'⎊'∊str) ∧ ¬Flag "%ALLOW_CATCH"
        
        ln ⇐ i-1
        str ⇐ str # at end to let Flag have updated it
      }
  }
  tests ← SearchRefine @⊸≢¨⊸/ ∾⟜<⟜(Process∘Next)•_while_{𝕊: i≠≠lns} ⟨⟩
  {𝕊: •Out "Narrowed down to test "∾Pos (⊑tests).ln}⍟⊢ (o.search≢@)∧1=≠tests
  
  
  tests { 𝕊ns: ⟨ln,str⟩←ns
    idx ← ⊑str⊐'%'
    {
      idx ≡ ≠str? ⟨ns, "eval", str⟩;
      (i2←idx+1)<≠str? '%'≡i2⊑str? id←'%'≡(i2+1)⊑str ⋄ ⟨ns, {id? "indistinguishable"; "equal"}, idx↑str, ({id? 3; 2}+idx)↓str⟩;
      {𝕊: Fail (Pos ln)∾": Unknown test format"}⍟⊢ '!'≢⊑str
      ⟨ns, "error", (idx+1)↓str, 1↓idx↑str⟩
    }
  }¨↩
  
  { 𝕊 d:
    ln←(⊑d).ln
    ty←1⊑d
    Lint ← {𝕊: •Out ∾⟨"Lint: ", Pos ln, ": ", 𝕨⟩}⍟(¬⊢)
    {
      "eval"≡ty? "No check or assert" Lint (∨´"_eqvar"⍷2⊑d) ∨ ∨´'!'=2⊑d;
      @
    }
  }¨⍟o.lint tests
  
  { 𝕊:
    badCount ← 0
    badMessage ← 0
    currLn ← 0
    dir ← •file.At "testDir"
    •file.CreateDir⍟(¬•file.Exists) dir
    Bad ← {
      ""𝕊𝕩;
      •Out ∾⟨Pos currLn, ": ", 𝕩⟩
      •Out¨ "  "⊸∾¨ 𝕨
      badCount+↩ 1
    }
    ErrMsg ← {𝕊: m←•CurrentError@ ⋄ {1==m? ∧´2=•Type¨m? m; •Repr⎊"(unrepresentable)" m}}
    
    PreEval ← ⊢
    _addPreEval ← {𝕩? PreEval↩PreEval 𝔽; @}
    {𝕊: 1 •internal.Temp o.seed}_addPreEval o.randomizeHeuristics
    
    Eval ← {
      currTestState ↩ @
      PreEval@
      ⟨dir, ∾⟨testname,"_line_",•Repr currLn+1,".bqn"⟩, ⟨helpers,"arg0",1⟩⟩ •BQN 𝕩
    }
    EvalS ← •BQN⎊{𝕊: Bad "Computing expected value errored: "∾{∧´2=•Type¨⥊𝕩?𝕩;•Repr⎊•Fmt 𝕩}•CurrentError@ ⋄ "(could not compute expected value)"}
    withFillState ← ⟨".","",⟨WithFill, ⊑⊸WithFill⊘(!∘0), !˙⊸WithFill⟩⟩
    
    toRun ← tests
    toRun {{𝕩.enabled}∘⊑¨⊸/𝕩}↩
    skipped ← (≠tests)-≠toRun
    results ← {
      ns‿"eval"‿str: {
        Eval _catch_ {𝕊: Bad "Expected success, got error: "∾ErrMsg@} str
        @
      };
      ·‿"error"‿str‿exp: {
        o.noerr? skipped+↩1;
        ok‿got ← (1⋈Eval)⎊(0⋈ErrMsg) str
        exp ↩ EvalS⍟{×≠Trim𝕩} exp
        {
          (got≡exp) ∧ ¬ok? @;
          ok? Bad "Expected error, but didn't get one" ⋄ @;
          ⟨"Expected: "∾exp ⋄ "Got:      "∾got⟩ Bad⍟o.errm "Wrong error message"
          badMessage+↩ 1
          1‿got
        }
      };
      ns‿kind‿str‿exp: ∨´md←kind⊸≡¨"indistinguishable"‿"equal"? {
        ok‿got ← (1⋈Eval)_catch_(0⋈ErrMsg) str
        exp ↩ {md≡1‿0? ∨´ (∨´⍷⟜𝕩)¨ "WF"‿"WOF"? withFillState EvalS "wf‿wf0‿wof←•args ⋄ "∾𝕩; EvalS 𝕩} exp
        eq‿repr ← {md≡0‿1? ⟨EEqual, •Repr⎊•Fmt⟩; ⟨Indistinguishable, IRepr⟩}
        {
          ¬ok? Bad "Expected value, but got error: "∾got ⋄ @;
          got Eq exp? @;
          ⟨"Expected: "∾Repr exp ⋄ "Got:      "∾Repr got⟩ Bad "Wrong value"
          2‿got
        }
      }
    }∘{currLn↩(⊑𝕩).ln ⋄ 𝕩}¨ toRun
    
    {𝕊:
      res‿desc ← ({
        1‿msg: o.updMsgs? (¬⊑(@+10)∊msg) ∧ ¬∨´"TEST FAIL"⍷msg;
        2‿msg: o.updVals;
        0
      }¨ results)⊸/¨ results‿toRun
      0≠≠res?
        •Out ∾⟨"Update ", •Repr≠res, " entr", (1<≠res)⊑"y"‿"ies", " in ", filename, "?"⟩
        {
          ¬⊑"yY"∊˜⊑'n'∾˜•GetLine@? •Out "Not updating.";
          
          [idxs, upd] ← ⍉>res {
            1‿new 𝕊 ⟨⟨ln⟩, "error", str, ·⟩: ln ⋈ ∾⟨"!", •Repr new, " ", ⊑∘⊐⟜'%'⊸↓ ln⊑lns⟩;
            2‿new 𝕊 ⟨⟨ln⟩, "equal", str, ·⟩: ln ⋈ ∾⟨{⋈⁼/" %% "⍷𝕩}⊸↑ ln⊑lns, " %% ", •Repr new⟩;
            2‿new 𝕊 ⟨⟨ln⟩, "indistinguishable", str, ·⟩: ln ⋈ ∾⟨{⋈⁼/" %%% "⍷𝕩}⊸↑ ln⊑lns, " %%% ", IRepr new⟩;
            !𝕨⋈𝕩
          }¨ desc
          
          "Refusing to write over changed file" ! lns ≡ •FLines fullfile
          fullfile •FLines lns {×≠𝕩? ⋈⁼𝕩; 𝕨}¨ (≠lns)↑idxs⊔upd
          •Out "Updated."
        }
      ;@
    }⍟⊢ o.updMsgs ∨ o.updVals
    notes ← ⟨⟩
    notes∾↩ ((¬o.errm)∧×badMessage)⥊< ∾⟨•Repr badMessage, " mismatched error message", (1≠badMessage)/"s"⟩
    notes∾↩ (×skipped)⥊< ∾⟨•Repr skipped, " skipped test", (1≠skipped)/"s"⟩
    notes ↩ {"": 𝕩; ∾⟨" (",2↓𝕩,")"⟩} ∾"; "⊸∾¨ notes
    ran ← (≠tests)-skipped
    {
      0=badCount? •Out filename∾": "∾(•Repr ran)∾" passed!"∾notes;
      •Out ∾⟨•Repr ran-badCount, '/', •Repr ran, notes⟩
      exitCode ↩ 1
    }
  }⍟o.run @
}

currTestState ← @
GetTestState ← @⊸≡◶⊢‿{ 𝕊:
  currTestState ↩ {
    rand ⇐ •MakeRand o.seed
  }
} {𝕊: currTestState}

helpers ← {
  ⟨TV⇐Variation, CLR⇐ClearRefs, ListVariations⟩ ← •internal
  _tvar0_ ⇐ {
      F _𝕣_ LV x: (CLR@) ⊢        {         F 𝕩 TV x}¨ LV 𝕩;
    w F _𝕣_ LV x: (CLR@) ⊢ (LV 𝕨) {(𝕨 TV w) F 𝕩 TV x}⌜ LV 𝕩
  }
  
  shouldNotError ⇐ o.noerr
  slow ⇐ o.slow
  randomizedHeuristics ⇐ o.randomizeHeuristics
  
  EqvarEq ← Indistinguishable
  Classify ← {(≢𝕩)⥊ ⊐EqvarEq⌜˜ ⥊𝕩}
  
  _eqvar0_ ⇐ { LV ← 𝔾
    r ← 𝕨 𝔽 _tvar0_ 𝔾 𝕩
    class ← {·𝕊·: 𝕨+2×𝕩; 𝕊·: 𝕩} c0←(≢r) {HasFill 𝕩? 'h' = 1⊸⊑¨ LV 𝕩; 𝕨⥊0}¨ args←𝕨⋈𝕩
    (⊑r)⊸("failed eqvar eequal" ! EEqual)¨ 1↓⥊r
    {
      r0 ← ⊑𝕩
      {
        r0 EqvarEq 𝕩?
          1;
        
        classes ← {
          ⟨a⟩: (@+10)∾ •Fmt "type"‿"class" ∾˘ a ≍ Classify r;
          wc‿xc:
            wv‿xv ← LV¨ args
            wc‿xc+↩'A'
            "row=type of 𝕨, col=type of 𝕩:" ∾ ∾⟨@+10,' ',' '⟩⊸∾¨ ⟨∾⟨"\|",xc," ==",∾' '⊸∾¨xv⟩⟩ ∾ (wc⋈¨wv) {c‿v𝕊𝕩: c ∾ '|' ∾ 𝕩 ∾ ' ' ∾ v}¨ <˘ '0'+Classify r
        } c0
        ! "TEST FAIL: _eqvar0_ failed: "∾classes
      }¨ 𝕩
    }¨ class ⊔ r
    ¯1⊑⥊r # take last as that'll be one with the correct fill
  }
  
  GetRand ⇐ {𝕊: •MakeRand (GetTestState@).rand.Range 1e9}
  
  AtRoot ⇐ {𝕊: •file.At 𝕩}
  Import ⇐ {𝕊: 𝕨 •Import •file.At 𝕩}
  
  _eqvar ⇐ {𝔽 _eqvar0_ ListVariations}
  _eqvarv_ ⇐ {𝔽 _eqvar0_ (𝕘⊸ListVariations)}
  
  Bin ⇐ {𝕊: "This test needs bin=... specified" !1=≠o.bin ⋄ ⋈⁼o.bin}
}

Run¨ SearchRefine o.files

•Exit⍟(0⊸≠)⍟(¬o.heapverify) exitCode