# setup
(
  •file.CreateDir⍟(¬•file.Exists) "empty-dir"
  LinkTo ← {•file.Exists 𝕩? @; ! 0=⊑•SH ⟨"ln", "-s", •file.At 𝕨, •file.At 𝕩⟩}
  "empty-dir" LinkTo "link-to-empty-dir"
  "nonexistent-file" LinkTo "link-to-nonexistent-file"
)
%DEF _noPath {𝔽⎊{𝕊: !(¬·≠`⊸>'"'⊸=)⊸/ •CurrentError@}}

! 10≤≠•FBytes "/proc/cpuinfo"
!"Attempting to read a directory as a file: """"" % •FBytes %USE _noPath "."
!"Attempting to read a directory as a file: """"" % •FBytes %USE _noPath "/"
!"Attempting to read a directory as a file: """"" % •FBytes %USE _noPath "/dev"
!"Attempting to read a directory as a file: """"" % •FBytes %USE _noPath "empty-dir"
!"Attempting to read a directory as a file: """"" % •FBytes %USE _noPath "link-to-empty-dir"
!"Couldn't open file for reading: """"" % •FBytes %USE _noPath "nonexistent-file"
!"Couldn't open file for reading: """"" % •FBytes %USE _noPath "link-to-nonexistent-file"
