lib ← "lib.so"

seed ← •rand.Range 1e9
# seed ↩ 123
•term.ErrRaw ∾⟨"Seed: ", •Repr seed, @+10⟩ ⋄ •term.Flush@
rnd ← •MakeRand seed
randHeuristics‿useCatch ← 3‿7 ⊏ •internal.Properties˜@
1 •internal.Temp⍟randHeuristics seed # set RANDOMIZE_HEURISTICS seed

skipped ← 0
verbose ← 1
_expErr ← {
  useCatch?
  "Expected error" ! 𝕨 0∘𝔽⎊{𝕊: •Out⍟verbose "Error message: "∾•CurrentError@ ⋄ 1} 𝕩;
  skipped+↩1}

·← {
  cases ← ⟨
    ⟨'i', 0, "&"‿"add_int64ptr"‿"&i64"‿"i64"⟩
    ⟨'u', 0, "&"‿"add_int64ptr"‿"&u64"‿"i64"⟩
    ⟨'i', 1, "&"‿"add_i64init"‿"⥊i64"‿"i64"‿"i64"⟩
    ⟨'u', 1, "&"‿"add_u64init"‿"⥊u64"‿"u64"‿"i64"⟩
    ⟨'i', 2, "i64"‿"add_i64ret"‿"i64"‿"i64"⟩
    ⟨'u', 2, "u64"‿"add_u64ret"‿"u64"‿"i64"⟩
    ⟨'i', 3, "{i64,u64}"‿"add_int64structret"‿"{i64,u64}"‿"i64"⟩
    ⟨'u', 3, "{i64,u64}"‿"add_int64structret"‿"{i64,u64}"‿"i64"⟩
    ⟨'i', 4, "{i64,u64}"‿"add_int64structret2"‿"*{i64,u64}"‿"i64"⟩
    ⟨'u', 4, "{i64,u64}"‿"add_int64structret2"‿"*{i64,u64}"‿"i64"⟩
    ⟨'i', 5, "&"‿"add_int64structptr"‿"&{i64,u64}"‿"i64"⟩
    ⟨'u', 5, "&"‿"add_int64structptr"‿"&{i64,u64}"‿"i64"⟩
    ⟨'i', 6, "&"‿"add_int64structptr"‿"&{[1]i64,[1]u64}"‿"i64"⟩
    ⟨'u', 6, "&"‿"add_int64structptr"‿"&{[1]i64,[1]u64}"‿"i64"⟩
  ⟩
  { qual‿kind‿spec:
    f ← lib •FFI spec
    •Out⍟verbose ∾⟨"### overflow check ", qual, ' ', •Repr kind⟩
    uns ← ⊑"iu"⊐qual
    o ← 1
    Call ← kind {
      0 _𝕣 a‿b: ⋈⁼⍟o F ⟨⋈a, b⟩;
      1 _𝕣 a‿b: ⋈⁼⍟o F ⟨1, a, b⟩;
      2 _𝕣 a‿b: F a‿b;
      3 _𝕣 a‿b:  uns⊑   ⍟o F ⟨   a⌾(uns⊸⊑) 2⥊99, b⟩;
      4 _𝕣 a‿b:  uns⊑   ⍟o F ⟨⋈  a⌾(uns⊸⊑) 2⥊99, b⟩;
      5 _𝕣 a‿b:( uns⊑⋈⁼)⍟o F ⟨⋈  a⌾(uns⊸⊑) 2⥊99, b⟩;
      6 _𝕣 a‿b:(⊑uns⊑⋈⁼)⍟o F ⟨⋈⋈¨a⌾(uns⊸⊑) 2⥊99, b⟩;
      !"Invalid kind"
    }
    ! 1000000000000001 ≡ Call 1e15‿1
    ·←{
      uns? Call _expErr ¯1e15‿1;
      ! ¯999999999999999 ≡ Call ¯1e15‿1
    }
    
    # last valid input at that end-point
    min ← {uns? 0; 1-2⋆53}
    max ← ¯1+2⋆53
    
    Call _expErr     ⟨min-1,  8⟩
    Call _expErr     ⟨min,   ¯1⟩
    ! min     = Call ⟨min,    0⟩
    ! max     = Call ⟨max,    0⟩
    Call _expErr     ⟨max,    1⟩
    Call _expErr     ⟨max+1, ¯8⟩
    
    spec2 ← {∨´"&⥊"=⊑𝕩? 1(↑∾'·'∾↓)𝕩; 𝕩}¨⌾(2⊸↓) spec
    {𝕊:
      o ↩ 0
      spec2 {"&":"";𝕩}⌾⊑↩
      f ↩ lib •FFI spec2
      Call¨ ⟨min⋈0, min⋈¯1, min⋈-max⟩
      Call¨ ⟨max⋈0, max⋈ 1, max⋈ max⟩
    }⍟⊢ spec2≢spec
  }¨ cases
}



bufStart‿bufSize ← {⋈⁼⌾(1⊸⊑) 𝕏 ⥊1} lib •FFI "*"‿"alloc_test_mem_4096"‿"⥊u64"
end ← ((bufStart.Cast "i8").Add bufSize).Cast"i8"

WidthOf ← {
  a ← bufStart.Cast 𝕩
  ((a.Add 1).Cast"i8").Sub bufStart.Cast"i8"
}

SNaNBytes ← {64: ¯8↑¯9‿¯1; 32: ¯4↑¯72‿¯1}
ValidateSNaNs ← {
  ! 𝕩 •internal.Indistinguishable •internal.Validate 𝕩
  ! ∧´ 𝕩≠𝕩
}

_mcd ← {f←@•FFI"*"‿"memcpy"‿𝕗‿"*i8"‿"usize" ⋄ 1⊑F}
·←{n←rnd.Range 100 ⋄ ValidateSNaNs "&f32"_mcd ⟨↕n, n‿4⥊SNaNBytes 32, n×4⟩}
·←{n←rnd.Range 100 ⋄ ValidateSNaNs "&f64"_mcd ⟨↕n, n‿8⥊SNaNBytes 64, n×8⟩}
·←{n←rnd.Range 100 ⋄ ValidateSNaNs "⥊f64"_mcd ⟨n,  n‿8⥊SNaNBytes 64, n×8⟩}
·←{n←rnd.Range 100 ⋄ ValidateSNaNs¨"⥊{f64}"_mcd ⟨n,  n‿8⥊SNaNBytes 64, n×8⟩}
·←{n←rnd.Range 100 ⋄ ! a ≡ "&f64:i8"_mcd ⟨0⥊˜n×8, a←⥊n‿8⥊SNaNBytes 64, n×8⟩}
·←{n←rnd.Range 100 ⋄ ! a ≡ "⥊f64:i8"_mcd ⟨n,      a←⥊n‿8⥊SNaNBytes 64, n×8⟩}
·←{n←rnd.Range 100 ⋄ ! a ≡ "&f32:i8"_mcd ⟨0⥊˜n×4, a←⥊n‿4⥊SNaNBytes 32, n×4⟩}
·←{n←rnd.Range 100 ⋄ ! a ≡ "⥊f32:i8"_mcd ⟨n,      a←⥊n‿4⥊SNaNBytes 32, n×4⟩}

! 1‿2‿4‿8 ≡ WidthOf¨ "i8"‿"i16"‿"i32"‿"i64"
! 1‿2‿4‿8 ≡ WidthOf¨ "u8"‿"u16"‿"u32"‿"u64"
! 1‿2‿4‿8 ≡ WidthOf¨ "u8:u1"‿"u16:c8"‿"i32:u1"‿"u64:i32"
! (5×1‿2‿4‿8) ≡ WidthOf¨ "[5]u8"‿"[5]u16"‿"[5]u32"‿"[5]u64"
! (7×1‿2‿4‿8) ≡ WidthOf¨ "[7]u8:u1"‿"[7]u16:c8"‿"[7]i32:u1"‿"[7]u64:i32"
! 4‿8‿4‿40 ≡ WidthOf¨ "f32"‿"f64"‿"f32:u1"‿"[5]f64:i8"

TYW ← •ParseFloat 1⊸↓
convs ← ⋈⟜TYW¨ "u1"‿"i8"‿"i16"‿"i32"‿"c8"‿"c16"‿"c32"
ArrTy ← {∾⟨"[",•Repr 𝕨,"]",𝕩⟩}
FillOf ← •internal.HasFill◶'e'‿{⊑1↑0⥊𝕩}

{ 𝕊 baseType:
  n ← rnd.Range 2⋆4+rnd.Range 5 # max of 255, times max elt size of 8, is below the 4096B limit
  # n ↩ 0
  nnz ← 1⌈n
  •Out⍟verbose ∾⟨"### ", baseType⟩ # ∾" n="∾•Repr n
  
  bw ← WidthOf baseType
  hasConvs ← baseType≢"bool"
  isBasePtr ← baseType≡"*"
  
  basePtrNZ ← (end.Cast baseType).Sub nnz
  {𝕊:
    (basePtrNZ.Cast "i8").WriteList (8×nnz)⥊10
    basePtrNZ.ReadList _expErr nnz
    (basePtrNZ.Cast baseType).Read _expErr¨ ⟨0, rnd.Range nnz, nnz-1⟩
    (basePtrNZ.Cast nnz ArrTy baseType).Read _expErr 0
  }⍟⊢ ⊑baseType<⊸∊"i64"‿"u64"
  
  # string tests; these don't use baseType
  ·←{
    base ← (end.Cast "i8:c8").Sub n
    base.WriteList n⥊'a'
    n •foreign.ReadBytesTo0 _expErr base
    n •foreign.ReadCharsTo0 _expErr base
  }
  ·←{
    baseNZ ← (end.Cast "i8:c8").Sub nnz
    chars ← "1¨⍉𝕨"⊏˜¯1+ {-⟜» ⍷ 𝕩⌊ +` 1+ 𝕩 rnd.Range 4} nnz-1
    bytes ← •ToUTF8 chars
    baseNZ.WriteList bytes∾@
    ! bytes ≡    •foreign.ReadBytesTo0 baseNZ.Cast "i8:c8"
    ! bytes ≡    •foreign.ReadBytesTo0 baseNZ.Cast ""
    ! bytes ≡ nnz•foreign.ReadBytesTo0 baseNZ.Cast "i8"
    ! chars ≡ nnz•foreign.ReadCharsTo0 baseNZ.Cast "u8"
    ! chars ≡    •foreign.ReadCharsTo0 baseNZ.Cast "u8:c8"
  }
  
  lptr ← (end.Cast baseType).Sub n
  
  ! (1+hasConvs) ≤ +´ { 𝕊 conv‿convw:
    hc ← conv≢@
    convType ← baseType ∾ {hc? ':'∾conv; ""}
    fac ← bw÷convw÷8
    fac≥1?
      •Out⍟verbose ∾⟨"  conv ", {@:"none";𝕩}conv, ": ", convType, " fac=", •Repr fac⟩
      styq‿styw ← {"bool": 'u'‿1; "*": 'p'‿@; (⊑⋈TYW)𝕩} hc ⊑ baseType‿conv
      
      ends ← {
        'i': {styw=64? -⊸⋈ ¯1+2⋆53; ⟨-a←2⋆styw-1, a-1⟩} ⋈¨ 1;
        'u': ⟨0, ¯1+2⋆53⌊styw⟩ ⋈¨ 1;
        'c': (@+⟨0, 1114111⌊¯1+2⋆styw⟩) ⋈¨ ⟨0, styw≠32⟩;
        (@‿{⇐}) ⋈¨ 2
      } styq
      ·←{
        ptr ← basePtrNZ.Cast convType
        ¯1‿1 { 𝕨 𝕊 v0‿k:
          kn0‿kn2 ← 0‿2≠k
          ptr.Write⍟kn2 fac ⥊⍟hc v0
          ptr.WriteList⍟kn2 fac⥊v0
          v1 ← 𝕨 +⍟(1=k) v0
          ptr.Write _expErr⍟kn0 fac ⥊⍟hc v1
          ptr.WriteList _expErr⍟kn0 fac⥊v1
        }¨ ends
        ptr.Write _expErr⍟(isBasePtr∧¬hc) ⟨⟩
        ptr.WriteList _expErr⍟(isBasePtr∧¬hc) ⋈⟨⟩
      }
      
      data ← {
        cap ← {
          styq='f'? styw=32? 2⋆24;
          styq='c'? styw=32? 1114111;
          styq='p'? 32;
          (¯1+2⋆53) ⌊ 2⋆styw
        }
        d0 ← n‿fac rnd.Range {rnd.Range 2? rnd.Range∘≠⊸⊑ 1‿2‿64⌊cap; cap}
        # d0 ← n‿fac⥊cap-1
        {
          'i': d0 - ⌊cap÷2;
          'c': d0 + @;
          'f': d0 × 2⊸×⍟(¯50 + (≢d0) rnd.Range 100) 1;
          'u': d0;
          'p': (bufStart.Cast "i8").Add¨ d0
        } styq
      }
      
      match ← {styq='p'? ∧´ {0=(𝕨.Cast "i8").Sub 𝕩.Cast "i8"}¨; ≡}
      
      ! 1=≠⍷ { 𝕊 name‿obj:
        # •Out⍟verbose "    "∾name
        { 𝕊 ⟨m, Write⇐W, Read⇐R⟩:
          {𝕊:
            (lptr.Cast "i8").WriteList (bw×n×fac) ⥊ SNaNBytes styw
            ValidateSNaNs Read n
          }⍟⊢ styq='f'
          {
            data1 ← 𝕩 •internal.Variation data
            data2 ← {m∨¬hc? ⥊𝕩; <˘𝕩} data1
            lptr.WriteList n⥊{isBasePtr? bufStart; 0}
            memcpy1 ← @•FFI ⟨"*", "memcpy", "⥊"∾convType, "*"∾convType, "usize"⟩
            memcpy2 ← @•FFI ⟨"*", "memcpy", "&"∾convType, "&"∾convType, "usize"⟩
            Write data2
            ! data2 Match Read n
            {! data1 Match○⥊ 𝕩}¨ 1↓ Memcpy1 ⟨n,     data1, n×bw⟩
            {! data1 Match○⥊ 𝕩}¨ 1↓ Memcpy2 ⟨data1, data1, n×bw⟩
            ! data ≡ data1
          }¨ "f" •internal.ListVariations data
          (lptr.Cast "i8").ReadList n×bw
        ;@:⟨⟩} obj
      }¨ ⟨
        "each" ⋈{    m⇐0, p←lptr.Cast convType                  ⋄ W ⇐ ↕∘≠ p.Write¨ ⊢ ⋄ R ⇐ p.Read¨ ↕}
        "list" ⋈{    m⇐1, p←lptr.Cast convType                  ⋄ W ⇐ p.WriteList    ⋄ R ⇐ p.ReadList}
        "arr"  ⋈{×n? m⇐1, p←lptr.Cast n ArrTy convType          ⋄ W ⇐ p.Write        ⋄ R ⇐   p.Read∘0; @}
        "starr"⋈{×n? m⇐1, p←lptr.Cast '{'∾'}'∾˜n ArrTy convType ⋄ W ⇐ p.Write ⋈      ⋄ R ⇐ ⊑ p.Read∘0; @}
      ⟩
      
      evens ← (lptr.Cast 2 ArrTy convType).Field 0
      en ← ⌈n÷2
      allc ← lptr.Cast convType
      
      # assumes written data by the previous test
      evenmask ← n ⥊ 1‿0
      evendata ← ⥊ evenmask / data
      ! evendata Match ∾⍟hc evens.Read¨ ↕en
      ! evendata Match evens.ReadList en
      
      ins ← en↑⌽data
      evens.WriteList ⥊ins # (↕en⌊3) evens.Write¨ {¬hc? ⥊𝕩; <˘∘‿fac⥊𝕩} ins
      ! (⥊ins⌾(evenmask⊸/) data) Match allc.ReadList n
      
      {𝕊:
        d ← {isBasePtr? 1; 1+fac} ↑ ⥊data
        allc.WriteList _expErr d
        evens.WriteList _expErr d
      }⍟⊢ fac>1
      
      fills ← {FillOf 𝕨.ReadList 𝕩}´¨ (allc⊸⋈¨ 0‿n) ∾ evens⊸⋈¨ 0‿en
      {𝕊: fills∾↩ FillOf¨ ⟨allc.Read 0, evens.Read 0⟩}⍟⊢ n≠0
      ! {styq='c'?' ';styq='p'?'e';0} ≡ ⋈⁼⍷fills
      
      1
    ;0
  }¨ ∾⟜convs⍟hasConvs ⟨@⋈bw×8⟩
}¨ "*"‿"bool"‿"f32"‿"f64" ∾ ⥊ 'i'‿'u' ∾⟜•Repr⌜ 8‿16‿32‿64

{𝕏 bufStart‿bufSize} lib •FFI ""‿"free_test_mem_4096"‿"*"‿"u64"

•Out "Passed!" ∾ (×skipped)/∾⟨" (", •Repr skipped," skipped)"⟩
