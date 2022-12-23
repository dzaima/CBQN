sz←"u64" ⋄ ssz←"i64" ⋄ nfds←sz

libc_fork ← @ •FFI "i32"‿"fork"
libc_pipe ← @ •FFI "i32"‿"pipe"‿">&i32"
libc_read ← @ •FFI ssz‿"read"‿"i32"‿"&i8"‿sz
libc_write ← @ •FFI ssz‿"write"‿"i32"‿"*i8"‿sz
libc_close ← @ •FFI "i32"‿"close"‿">i32"
libc_poll ← @ •FFI "i32"‿"poll"‿"&i16"‿nfds‿"i32"

Pipe ← { 𝕊: readfd‿writefd ⇐
  ⟨c, readfd‿writefd⟩ ← Libc_pipe 0‿0
  ! c=0
  Write ⇐ {Libc_write ⟨writefd, 𝕩, ≠𝕩⟩}
  Read ⇐ {n‿b ← Libc_read ⟨readfd, 𝕩⥊0, 𝕩⟩ ⋄ !n≥0 ⋄ n↑b}
}

BiDiFork ← { 𝕊:
  p1 ← Pipe@
  p2 ← Pipe@
  pid ⇐ Libc_fork@
  ! pid≥0
  parent ⇐ pid=0
  CloseWR ← {! ¬∨´ Libc_close¨ {𝕨.writefd‿𝕩.readfd}´ ⌽⍟parent 𝕩}
  CloseWR p1‿p2
  r‿w ⇐ ⌽⍟parent p1‿p2
  Read‿readfd ⇐ r
  Write‿writefd ⇐ w
  Close ⇐ CloseWR∘p2‿p1
}

FFn ← { 𝕊 fn:
  f ← BiDiFork@
  {f.parent? Fn f ⋄ f.Close@ ⋄ •Exit 0; f}
}

WriteAll ← {p𝕊x: {𝕩-p.Write 𝕩-⊸↑x}•_while_× ≠x}
_readAll ← {p E _𝕣 n: ⊑⍟(¬=) {v←p.Read n-≠𝕩 ⋄ 0<≠v? 𝕩∾v; <E"eof"}•_while_(=∧n≠≠) ↕0}

WriteSz ← {
  𝕨 WriteAll (32‿8•bit._cast ⋈≠𝕩)∾𝕩
}
_readSz ← { 𝔽_𝕣 s:
  r ← s <∘𝔽 _readAll 4
  {
    =r? n ← ⊑8‿32•bit._cast r ⋄ s !_readAll n;
    ⊑r
  }
}

Serialize‿Deserialize ← •Import "serialize.bqn"

c_make ← 1
c_request ← 2
c_take ← 3
c_one ← 4

control ← FFn {{ 𝕊 control:
  pids‿procs ← ⋈˜⟨⟩
  {𝕊:
    ty‿cmd ← Deserialize •Exit _readSz control
    
    ty◶⟨ !
      { 𝕊program: # c_make
        p ← FFn {{ 𝕊 p:
          f ← ⟨•file.Parent program, •file.Name program⟩ •BQN •FChars program
          {𝕊: p WriteSz Serialize F Deserialize •Exit _readSz p}•_while_ 1@
        }}
        control WriteAll 32‿8•bit._cast ⋈p.pid
        pids∾↩ <p.pid
        procs∾↩ <p
      }
      { 𝕊pid‿data: # c_request
        p ← (⊑pids⊐pid)⊑procs
        p WriteSz data
      }
      { 𝕊pid: # c_take
        p ← (⊑pids⊐pid)⊑procs
        control WriteSz !_readSz p
      }
      { 𝕊limit‿sel: # c_one
        ms ← limit×1e3
        "poll time limit too large" ! (ms≡∞) ∨ ms<2⋆31
        p ← (pids⊐sel)⊏procs
        fds ← {𝕩.readfd}¨ p
        ret‿res ← Libc_poll ⟨⥊ ∾⟜1‿0˘ 32‿16•bit._cast ≍˘fds ⋄ ≠fds ⋄ ∞⊸≡◶⌊‿¯1 ms⟩
        ! ret≥0
        control WriteSz Serialize {¯1¨⌾((¬𝕩∊0‿1)⊸/)𝕩} ¯1⊏˘∘‿4⥊res
      }
    ⟩ cmd
  }•_while_ 1@
}}
AskControl ← {control WriteSz Serialize 𝕨‿𝕩}

Spawn ⇐ {
  c_make AskControl 𝕩
  pid ← ⊑8‿32•bit._cast control !_readAll 4
  {
    pid⇐pid
    Request ⇐ {c_request AskControl pid⋈Serialize 𝕩}
    Take ⇐ {𝕊: c_take AskControl pid ⋄ Deserialize !_readSz control}
  }
}

# 𝕩: list of spawn results to wait on
# 𝕨: if present, maximum time to wait; 0 to return immediately
# result: boolean mask of which have finished (if 𝕨 isn't given, guaranteed to have at least one `1`)
WaitForOne ⇐ {
  c_one AskControl ⟨𝕨⊣∞ ⋄ {𝕩.pid}¨ 𝕩⟩
  Deserialize !_readSz control
}
