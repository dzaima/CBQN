f←@
Section ← {•Out 𝕩∾˜@+10}

f ↩ "lib.so" •FFI ""‿"do_nothing" ⋄ •Show F ⟨⟩

Section "# ""a"""
f ↩ "lib.so" •FFI "i32"‿"getType"‿">a" ⋄ •Show F¨ ⟨⟨⟩, 1, '𝕩', +, ¨, ∘, {⇐}⟩
f ↩ "lib.so" •FFI "a"‿"timesTen"‿"a" ⋄ •Show F ⟨↕10⟩
f ↩ "lib.so" •FFI "a"‿"readAtoms"‿"a"‿"a" ⋄ •Show F ⟨÷5 ⋄ '𝕨'⟩
f ↩ "lib.so" •FFI "a"‿"readTyped"‿">a" ⋄ •Show F (10×↕7) + (4⥊<↕10)∾3⥊<@+↕10
f ↩ "lib.so" •FFI "a"‿"getShape"‿">a" ⋄ •Show F¨ {0⥊˜⌽⍟𝕩↕𝕩}¨ ↕5
f ↩ "lib.so" •FFI "a"‿"evalBQN"‿">a" ⋄ •Show F "↕5"
f ↩ "lib.so" •FFI "a"‿"makeArrays" ⋄ •Out∘•Repr¨ F ⟨⟩
f ↩ "lib.so" •FFI "a"‿"makeThree"‿">a" ⋄ •Show F "foo"
f ↩ "lib.so" •FFI "i32"‿"directAccess"‿">a" ⋄ •Show F "Ai32"•internal.Variation ↕10
bind ← "lib.so" •FFI "a"‿"bindAdd"‿">a" ⋄ g ← Bind 4 ⋄ •Show G 123

Section "# namespaces"
f ↩ "lib.so" •FFI "a"‿"getField"‿">𝕨a"‿"a"‿"a" ⋄ •Show {ab⇐1‿2 ⋄ cd⇐3‿4} F ⟨"ab" ⋄ "default"⟩
f ↩ "lib.so" •FFI "a"‿"getField"‿">𝕨a"‿"a"‿"a" ⋄ •Show {ab⇐1‿2 ⋄ cd⇐3‿4} F ⟨"ef" ⋄ "default"⟩
f ↩ "lib.so" •FFI "a"‿"getField"‿"𝕨a"‿"𝕨a"‿"𝕨a" ⋄ •Show ⟨{ab⇐1‿2 ⋄ cd⇐3‿4} ⋄ "ef" ⋄ "default"⟩ F ⟨⟩

Section "# print args"
f ↩ "lib.so" •FFI ""‿"printArgs"‿"i8"‿"i16"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64" ⋄ •Show F ¯123‿¯12323‿¯212312312‿250‿50000‿3123456789‿π∾÷3
f ↩ "lib.so" •FFI ""‿ "noopArgs"‿"i8"‿"i16"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64" ⋄ •Show F ¯123‿¯12323‿¯212312312‿250‿50000‿3123456789‿π∾÷3
f ↩ "lib.so" •FFI ""‿"printPtrArgs"‿"*i8"‿"*i16"‿"*i32"‿"*u8"‿"*u16"‿"*u32"‿"*f32"‿"*f64" ⋄ •Show F ⥊¨¯123‿¯12323‿¯212312312‿250‿50000‿3123456789‿π∾÷3
f ↩ "lib.so" •FFI ""‿"printPtrArgs"‿"&i8"‿"&i16"‿"&i32"‿"&u8"‿"&u16"‿"&u32"‿"&f32"‿"&f64" ⋄ •Show F ⥊¨¯123‿¯12323‿¯212312312‿250‿50000‿3123456789‿π∾÷3

f ↩ "lib.so" •FFI "f32"‿"printU64s"‿"u64:c8"‿"*u64:c8" ⋄ •Show F ⟨"hellowor", "aaaaaaaa12345678texttext"⟩
f ↩ "lib.so" •FFI "f32"‿"printU64s"‿"u64:u1"‿"*u64:u1" ⋄ •Show F ⟨64↑∾×∘↕¨↕12, 192↑∾×∘↕¨↕21⟩

! 3 ≡ •Type "lib.so" •FFI ""‿"printArgs"‿"i8"‿"i16:c8"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"
! 3 ≡ •Type "lib.so" •FFI ""‿"printArgs"‿"i8"‿"i16:c16"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"
! 3 ≡ •Type "lib.so" •FFI ""‿"printArgs"‿"i8:c8"‿"i16"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"

Section "# read pointers"
f ↩ "lib.so" •FFI "i32"‿"multiplyI32Ptrs"‿"*i32"‿"*i32"‿"i32" ⋄ •Show F ⟨↕10 ⋄ •Show ⌽↕10 ⋄ •Show 10⟩
f ↩ "lib.so" •FFI "f32"‿"sumF32Arr"‿"*f32:i32"‿"i32" ⋄ •Show F ⟨1065353216‿1073741824‿1077936128‿1082130432‿1084227584‿1086324736‿1088421888‿1090519040‿1091567616‿1092616192,10⟩

Section "# mutate i32*"
f ↩ "lib.so" •FFI ⟨"","incI32s","&u32",      "i32"⟩ ⋄ •Show F ⟨1e8×20+↕10 ⋄ 10⟩
f ↩ "lib.so" •FFI ⟨"","incI32s","&i32:u1",   "i32"⟩ ⋄ •Show ⊑F ⟨1‿0‿1‿0‿1‿0‿1‿0‿0‿1‿0‿1‿0‿0‿1‿1‿0‿1‿1‿0‿0‿0‿0‿1‿0‿1‿0‿1‿0‿0‿1‿0‿0‿0‿1‿0‿0‿0‿1‿0‿0‿0‿1‿1‿1‿0‿1‿1‿1‿1‿1‿1‿0‿0‿1‿1‿0‿0‿1‿0‿0‿1‿0‿0, 2⟩
f ↩ "lib.so" •FFI ⟨"","incI32s","&i32:c8",   "i32"⟩ ⋄ •Show ⊑F ⟨"hello, world", 2⟩
f ↩ "lib.so" •FFI ⟨"","incI32s",">𝕨&i32:c8",">i32"⟩ ⋄ •Show ⊑ "hello, world" F 2

f ↩ "lib.so" •FFI ⟨"", "incI32s",  "&i32:c8",  "i32"⟩ ⋄ •Show ⊑F ⟨"hello, world", 2⟩
f ↩ "lib.so" •FFI ⟨"", "incI32s",  "&i32:c8", "𝕨i32"⟩ ⋄ •Show ⊑ ⟨2⟩ F ⟨"hello, world"⟩
f ↩ "lib.so" •FFI ⟨"", "incI32s",  "&i32:c8",">𝕨i32"⟩ ⋄ •Show ⊑ 2 F ⟨"hello, world"⟩
f ↩ "lib.so" •FFI ⟨"", "incI32s", ">&i32:c8",">𝕨i32"⟩ ⋄ •Show ⊑ 2 F "hello, world"
f ↩ "lib.so" •FFI ⟨"", "incI32s",">𝕨&i32:c8", ">i32"⟩ ⋄ •Show ⊑ "hello, world" F 2
f ↩ "lib.so" •FFI ⟨"", "incI32s",  "&i32:c8",  "i32"⟩ ⋄ •Show F ⟨"hello, world", 2⟩
f ↩ "lib.so" •FFI ⟨"&","incI32s",  "&i32:c8",  "i32"⟩ ⋄ •Show F ⟨"hello, world", 2⟩

Section "# mutate i32*, i16*, i8*"
f ↩ "lib.so" •FFI ⟨"","incInts","&i32",  "&i16","&i8"⟩ ⋄ •Show F ⥊¨ 10‿20‿30
f ↩ "lib.so" •FFI ⟨"","incInts","&i32", "𝕨&i16","&i8"⟩ ⋄ •Show ⟨⥊20⟩ F ⥊¨ 10‿30
f ↩ "lib.so" •FFI ⟨"","incInts","&i32",">𝕨&i16","&i8"⟩ ⋄ •Show ⟨20⟩ F ⥊¨ 10‿30

Section "# mutate all element types"
f ↩ "lib.so" •FFI ⟨"","incNativeElements"⟩∾ "&"⊸∾¨ "f32"‿"f64"∾˜ ⥊"i"‿"u" ∾⌜ "8"‿"16"‿"32" ⋄ •Show F 1‿2⊸⥊¨ 0.5⊸+⌾(¯2⊸↑) 10+↕8
f ↩ "lib.so" •FFI ⟨"","incNativeElements"⟩∾ "&"⊸∾¨ "f32"‿"f64"∾˜ ⥊"i"‿"u" ∾⌜ "8"‿"16"‿"32" ⋄ •Show F 8⥊< 1‿2⥊0
f ↩ "lib.so" •FFI ⟨"","incNativeElements"⟩∾ "&"⊸∾¨ "f32"‿"f64"∾˜ ⥊"i"‿"u" ∾⌜ "8"‿"16"‿"32" ⋄ •Show F 1‿2⊸⥊¨ ¯100‿¯30000‿¯2e9‿200‿60000‿4e9 ∾ 1e38‿1e300
f ↩ "lib.so" •FFI ⟨"","incInt64","&u64:i16","&i64:i16"⟩ ⋄ •Show F 2⥊< 2‿4⥊¯1‿¯1‿6‿0

Section "# u64 & i64 tests"
f ↩ "lib.so" •FFI ⟨"u64",    "ident_u64",">u64:i32"⟩ ⋄ •Show F 1234‿12
f ↩ "lib.so" •FFI ⟨"u64",    "ident_u64",">u64"    ⟩ ⋄ •Show F +´2⋆52‿20
f ↩ "lib.so" •FFI ⟨"u64",    "ident_u64",">u64"    ⟩ ⋄ •Show F ¯1+2⋆53
f ↩ "lib.so" •FFI ⟨"u64:i32","ident_u64",">u64"    ⟩ ⋄ •Show F 123456789123456
f ↩ "lib.so" •FFI ⟨"u64:u1", "ident_u64",">u64:c8" ⟩ ⋄ •Show F "hellowor"

f ↩ "lib.so" •FFI ⟨"i64:u1", "ident_i64",">i64:c8" ⟩ ⋄ •Show F "hellowor"
f ↩ "lib.so" •FFI ⟨"i64", "ident_i64",">i64"⟩ ⋄ •Show F   ¯1+2⋆53
f ↩ "lib.so" •FFI ⟨"i64", "ident_i64",">i64"⟩ ⋄ •Show F - ¯1+2⋆53

Section "# malloc test"
f ↩ "lib.so" •FFI "*:i32"‿"malloc"‿">u64" ⋄ •Show (•internal.Type⋈≠) malloc ← F 123
f ↩ "lib.so" •FFI ""‿"free"‿">*:i32" ⋄ F malloc

Section "# pick item"
f ↩ "lib.so" •FFI "*:i8"‿"pick_ptr"‿">**:i8"‿">𝕨i32" ⋄ •Show @+0 F "helloworfoobarba"-@
f ↩ "lib.so" •FFI "*:c8"‿"pick_ptr"‿">**:c8"‿">𝕨i32" ⋄ •Show   0 F "helloworfoobarba"
f ↩ "lib.so" •FFI ⟨"u64:i8","pick_u64",">*u64:i8",">𝕨i32"⟩ ⋄ •Show @+2 F "000000001234560011122100abacabad"-@
f ↩ "lib.so" •FFI ⟨"u64:i8","pick_u64",">*u64:i8",">𝕨i32"⟩ ⋄ •Show @+3 F "000000001234560011122100abacabad"-@
f ↩ "lib.so" •FFI ⟨"u64",   "pick_u64",">*u64:i8",">𝕨i32"⟩ ⋄ •Show   1 F "000000001234560011122100"-'0'

Section "# structs"
s1 ← "{u8,i32,i16,u64:i16,f64}" ⋄ s2 ← ∾"{"‿s1‿","‿s1‿"}"
f ↩ "lib.so" •FFI ⟨"i16", "thirdMember", ">"∾s1⟩ ⋄ •Show F ⟨200, 2e9, ¯30000, 1‿2‿3‿4, 3.25⟩
f ↩ "lib.so" •FFI ⟨s1, "incMembers", ">"∾s1⟩ ⋄ •Show F ⟨200, 2e9, ¯30000, 1‿2‿3‿4, 3.25⟩
f ↩ "lib.so" •FFI ⟨"&", "incMany", "&"∾s2, "u64"⟩ ⋄ •Show¨ F ⟨0‿3‿6+3⥊<0‿1+2⥊<⟨200, 2e9, ¯30000, 1‿2‿3‿4, 3.25⟩, 3⟩

{
  calloc ← @•FFI"*:i32"‿"calloc"‿"usize"‿"usize" ⋄ mem ← Calloc 3‿4
  f ← "lib.so" •FFI "i32"‿"arrayRefOp"‿">&{*:i32,u64}"
  {𝕊: ⟨sum, ⟨arr‿sz⟩⟩ ← F ⟨mem‿3⟩ ⋄ •Show sum ∾ arr-mem}¨ ↕2
}

Section "# self-ffi"
•term.Flush@
f ↩ @ •FFI ⟨"i32", "putchar", ">i32"⟩ ⋄ F¨ 10∾˜"text"-@
•term.Flush@
f ↩ @ •FFI ⟨"a", "bqn_makeChar", ">u32"⟩ ⋄ •Show F 120169

Section "# array types"

f ↩ "lib.so"•FFI"i32"‿"arrarg"‿">[3]i32" ⋄ •Show F ⟨1,2,3⟩
f ↩ "lib.so"•FFI"i32"‿"ptrToArr"‿">*[2]i32" ⋄ •Show F ⟨0‿0, 0‿0, 0‿0, 10‿20, 3‿4⟩

as1 ← "{[1]i32}"
f ↩ "lib.so"•FFI ⟨"i32", "arrstruct1", as1, "*"∾as1⟩ ⋄ •Show F ⟨⋈⋈10, ⟨⋈⋈20⟩⟩

as2 ← "{[1]i32,[2]"∾as1∾",[3]f64}"
f ↩ "lib.so"•FFI ⟨as2, "arrstruct2", as2, "*"∾as2⟩ ⋄ •Out •Repr F ⟨⟨⟨4⟩ ⋄ ⋈∘⋈¨1‿2 ⋄ 1.2‿2.3‿3.4⟩, ⟨⟨⟨5⟩ ⋄ ⋈∘⋈¨10‿20 ⋄ 5.1‿6.1‿7.1⟩⟩⟩
f ↩ "lib.so"•FFI ⟨"",  "arrstruct2Inc", ">&·"∾as2⟩ ⋄ •Show F arrStruct2v←(⊢⋈50⊸+) ⟨⟨10⟩ ⋄ ⋈∘⋈¨11‿12 ⋄ 13‿14‿15⟩
f ↩ "lib.so"•FFI ⟨"&", "arrstruct2Inc", ">&"∾as2⟩ ⋄ •Out •Repr F arrStruct2v
f ↩ "lib.so"•FFI ⟨"", "add_int64structarr", "&·{[1]i64,[1]u64}", "i64"⟩ ⋄ •Show F ⟨5⥊<2⥊<⋈1e14, ¯1e15⟩

manyArgs0 ← "{f32,*i8,[5]i8}"
f ↩ "lib.so"•FFI "&"‿"manyargs"‿manyArgs0‿"*[7]i8"‿"[4]i8"‿"&i8" ⋄ •Show F ⟨⟨31, 10+↕10, 5+↕5⟩, ⟨14+↕7, 21+↕7⟩, 40+↕4, 100+↕13⟩ # ⟨100, 31, 10,19, 5,9 ⋄ 14,20, 21,27, 40,43, 112⟩
f ↩ "lib.so"•FFI  ""‿"manyargs"‿manyArgs0‿"*[7]i8"‿"[4]i8"‿"&i8" ⋄ •Show F ⟨⟨31, 10+↕10, 5+↕5⟩, ⟨14+↕7, 21+↕7⟩, 40+↕4, 100+↕13⟩

Section "# nested"
{
  f ← "lib.so"•FFI "i32"‿"callWithinMutated"‿"a"‿"&i32"
  g ← @ •FFI "*:i8"‿"memcpy"‿"&i8"‿"*i8"‿"usize"
  Fn ← {
    ! 𝕩≡fn
    •Show 1⊑G ⟨5⥊0, 3+↕5, 4⟩
    "hello"
  }
  •Show F ⟨fn, ↕10⟩
}

Section "# Struct of pointers"
{
  malloc ← "lib.so" •FFI ⟨"*u8", "malloc", ">"∾"usize"⟩
  p ← Malloc 100
  sop ← "{*,*i32,[2]*}"
  f ← "lib.so"•FFI ⟨sop, "operateOnStructOfPtrs", sop, "[2]"∾sop⟩
  •Show {(𝕩.Cast "u8").Sub p}⚇0 F {(p.Add 𝕩).Cast ""}⚇0 ⟨1,2,3‿4⟩⊸+⚇0 ⟨10, 20‿30⟩
  
  f ↩ "lib.so" •FFI "&"‿"updatePointerWithinPointer"‿">&{i32,*}"
  •Show {x‿y: x ⋈ (y.Cast "u8").Sub p}¨ F {⟨𝕩×1000, p.Add 𝕩×100⟩}¨ ↕5
  
  f ↩ "lib.so" •FFI "&"‿"updatePointerWithinPointer"‿">&{i32,*u8}"
  •Show {x‿y: x ⋈ y.Sub p}¨ F {⟨𝕩×1000, (p.Add 𝕩×100).Cast ""⟩}¨ ↕5
  
  f ↩ "lib.so" •FFI ""‿"mutateStructOfPtrs2"‿">{&i32,i8,{i16,⥊i8,&{i64,u64}}}"
  •Show F ⟨10×1+↕3, ¯100, ⟨¯50, 10, ¯10‿1⊸×¨ 1e10×1+↕3⟩⟩
}

Section "# 𝕨/𝕩/>"
{
  _case ← {
    int ← "lib.so" •FFI ⟨"i32",(⊑𝕗)∾"_i32"⟩ ∾ ∾⟜"i32"¨ 1↓𝕗
    r0 ← 𝕨 Int 𝕩
    ptr ← "lib.so" •FFI ⟨"i32",(⊑𝕗)∾"_i32p"⟩ ∾ ∾⟜"*i32"¨ 1↓𝕗
    r1 ← 𝕨 Ptr○(⋈⚇0) 𝕩
    •Show ⟨𝕨0⊘1@, 𝕗, r0, r1⟩
  }
  
  Auto ← { name 𝕊 args:
    { 𝕊 onX:
      wc‿xc ← 2↑/⁼onX
      { 𝕊 noW‿vW‿wX:
        sig ← ⟨name⟩ ∾ onX ⊏ (vW‿wX⥊¨'>') ∾¨ "𝕨"‿"𝕩"
        {
          noW?                 sig _case ⋈⁼⍟wX onX/args
          ; (⋈⁼⍟vW onX¬⊸/args) sig _case ⋈⁼⍟wX onX/args
        }
      }¨ ⥊↕ 1+⟨wc=0, wc=1, xc=1⟩
    }¨ ⥊↕ (≠args)⥊2
  }
  "inc" Auto ⟨50⟩ # 51
  "sub" Auto 99‿9 # 90
  "pack" Auto 9‿8‿7 # 987
}

Section "# SNaNs"
{
  •Show •internal.Validate {𝕏⟨⟩} "lib.so" •FFI "f32"‿"ret_snan32"
  •Show •internal.Validate {𝕏⟨⟩} "lib.so" •FFI "f64"‿"ret_snan64"
}

Section "# size_t"
{
  f ↩ "lib.so" •FFI "usize"‿"size_type"‿"&isize"‿"usize"‿"isize"
  •Show F ⟨¯1e5‿¯10, 1e8, ¯5⟩
}

Section "# globals"
{
  •Show "lib.so" •foreign.Value "i32"‿"globalI32"
  f0‿f1‿f2 ← "lib.so" •foreign.Value manyArgs0‿"globalStruct"
  •Show f0‿f2
  •Show f1.ReadList 4
  •Show ("lib.so" •foreign.Pointer "i64"‿"globalArr").ReadList 4
  fns ← "lib.so" •foreign.Pointer "*"‿"globalFnPointers"
  •Show (•foreign.Function ⟨"i64", fns.Read 0, "i64", "i64"⟩) {𝕎𝕩} 111111111111111‿¯11
  •Show (•foreign.Function ⟨"i64", fns.Read 1, "i64", "i64"⟩) {𝕎𝕩} 123456789123‿1000
}

# erroring:

# "lib.so" •FFI ""‿"printArgs"‿"i8"‿"i16:c32"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"
# "lib.so" •FFI ""‿"testArgs"‿"i8:c16"‿"i16"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"
# "lib.so" •FFI ""‿"testArgs"‿"i8:c32"‿"i16"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"

# f ↩ "lib.so" •FFI "u64"‿"ident_u64"‿">u64:i32" ⋄ •Show F 1234‿12344444
# f ↩ "lib.so" •FFI "u64"‿"ident_u64"‿">u64" ⋄ •Show F +´2⋆53‿20

# f ↩ "lib.so"•FFI"i32"‿"arrarg"‿">[3]i32" ⋄ •Show F ⟨1,2⟩
# f ↩ "lib.so"•FFI ⟨as2, "arrstruct2", as2, "*"∾as2⟩ ⋄ •Show F ⟨⟨⟨4⟩ ⋄ ⋈∘⋈¨1‿2 ⋄ 1.2‿2.3‿3.4‿4⟩, ⟨⟨⟨5⟩ ⋄ ⋈∘⋈¨10‿20 ⋄ 5.1‿6.1‿7.1⟩⟩⟩
# f ↩ "lib.so"•FFI ⟨as2, "arrstruct2", as2, "*"∾as2⟩ ⋄ •Show F ⟨⟨⟨4⟩ ⋄ ⋈∘⋈¨1‿2 ⋄ 1.2‿2.3‿3.4⟩, ⟨⟨⟨5⟩ ⋄ ⋈∘⋈¨10‿20 ⋄ 5.1‿6.1‿7.1‿5⟩⟩⟩
# f ↩ "lib.so"•FFI ⟨as2, "arrstruct2", as2, "*"∾as2⟩ ⋄ •Show F ⟨⟨⟨4⟩ ⋄ ⋈∘⋈¨1‿2 ⋄ 1.2‿2.3‿3.4⟩, ⟨⟨⟨5⟩ ⋄ ⋈∘⋈¨10‿20 ⋄ 5.1‿6.1‿7.1,5⟩⟩⟩
