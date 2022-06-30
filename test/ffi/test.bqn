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
f ↩ "lib.so" •FFI ⟨"","incI32s","&i32:c8",   "i32"⟩ ⋄ •Show ⊑F ⟨"helloworld", 2⟩
f ↩ "lib.so" •FFI ⟨"","incI32s",">𝕨&i32:c8",">i32"⟩ ⋄ •Show ⊑ "helloworld" F 2

f ↩ "lib.so" •FFI ⟨"", "incI32s",  "&i32:c8",  "i32"⟩ ⋄ •Show ⊑F ⟨"helloworld", 2⟩
f ↩ "lib.so" •FFI ⟨"", "incI32s",  "&i32:c8", "𝕨i32"⟩ ⋄ •Show ⊑ ⟨2⟩ F ⟨"helloworld"⟩
f ↩ "lib.so" •FFI ⟨"", "incI32s",  "&i32:c8",">𝕨i32"⟩ ⋄ •Show ⊑ 2 F ⟨"helloworld"⟩
f ↩ "lib.so" •FFI ⟨"", "incI32s", ">&i32:c8",">𝕨i32"⟩ ⋄ •Show ⊑ 2 F "helloworld"
f ↩ "lib.so" •FFI ⟨"", "incI32s",">𝕨&i32:c8", ">i32"⟩ ⋄ •Show ⊑ "helloworld" F 2
f ↩ "lib.so" •FFI ⟨"", "incI32s",  "&i32:c8",  "i32"⟩ ⋄ •Show F ⟨"helloworld", 2⟩
f ↩ "lib.so" •FFI ⟨"&","incI32s",  "&i32:c8",  "i32"⟩ ⋄ •Show F ⟨"helloworld", 2⟩

Section "# mutate i32*, i16*, i8*"
f ↩ "lib.so" •FFI ⟨"","incInts","&i32",  "&i16","&i8"⟩ ⋄ •Show F ⥊¨ 10‿20‿30
f ↩ "lib.so" •FFI ⟨"","incInts","&i32", "𝕨&i16","&i8"⟩ ⋄ •Show ⟨⥊20⟩ F ⥊¨ 10‿30
f ↩ "lib.so" •FFI ⟨"","incInts","&i32",">𝕨&i16","&i8"⟩ ⋄ •Show ⟨20⟩ F ⥊¨ 10‿30

Section "# u64 tests"
f ↩ "lib.so" •FFI ⟨"u64",    "ident_u64",">u64:i32"⟩ ⋄ •Show F 1234‿12
f ↩ "lib.so" •FFI ⟨"u64",    "ident_u64",">u64"    ⟩ ⋄ •Show F +´2⋆52‿20
f ↩ "lib.so" •FFI ⟨"u64:i32","ident_u64",">u64"    ⟩ ⋄ •Show F 123456789123456
f ↩ "lib.so" •FFI ⟨"u64:u1", "ident_u64",">u64:c8" ⟩ ⋄ •Show F "hellowor"

Section "# malloc test"
f ↩ "lib.so" •FFI "*:i32"‿"malloc"‿">u64" ⋄ •Show (•internal.Info⋈≠) malloc ← F 123
f ↩ "lib.so" •FFI ""‿"free"‿">*:i32" ⋄ F malloc

Section "# pick item"
f ↩ "lib.so" •FFI "*:i8"‿"pick_ptr"‿">**:i8"‿">𝕨i32" ⋄ •Show @+0 F "helloworfoobarba"-@
f ↩ "lib.so" •FFI "*:c8"‿"pick_ptr"‿">**:c8"‿">𝕨i32" ⋄ •Show   0 F "helloworfoobarba"
f ↩ "lib.so" •FFI ⟨"u64:i8","pick_u64",">*u64:i8",">𝕨i32"⟩ ⋄ •Show @+2 F "000000001234560011122100abacabad"-@
f ↩ "lib.so" •FFI ⟨"u64:i8","pick_u64",">*u64:i8",">𝕨i32"⟩ ⋄ •Show @+3 F "000000001234560011122100abacabad"-@
f ↩ "lib.so" •FFI ⟨"u64",   "pick_u64",">*u64:i8",">𝕨i32"⟩ ⋄ •Show   1 F "000000001234560011122100"-'0'


# erroring:

# "local/lib.so" •FFI ""‿"printArgs"‿"i8"‿"i16:c32"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"
# "local/lib.so" •FFI ""‿"testArgs"‿"i8:c16"‿"i16"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"
# "local/lib.so" •FFI ""‿"testArgs"‿"i8:c32"‿"i16"‿"i32"‿"u8"‿"u16"‿"u32"‿"f32"‿"f64"

# f ↩ "local/lib.so" •FFI "u64"‿"ident_u64"‿">u64:i32" ⋄ •Show F 1234‿12344444
# f ↩ "local/lib.so" •FFI "u64"‿"ident_u64"‿">u64" ⋄ •Show F +´2⋆53‿20