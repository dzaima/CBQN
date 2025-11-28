%DEF fill Fill←•internal.HasFill◶'e'‿{⊑1↑0⥊𝕩}
%DEF eqall EqAll ← {!∘≡⟜(⊑𝕩)¨ 𝕩 ⋄ ⊑𝕩}

!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 4↑{⇐}
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 4↑{+}
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 4↑{¨}
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 4↑{∘}
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 4↑@‿1
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 2‿2↑@‿1
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 2‿2↑@‿1‿@
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 3↑2‿2⥊@‿1
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % 3‿3↑2‿2⥊@‿1
!"𝕨↑𝕩: Fill element of 𝕩 needed for overtaking" % ¯4↑@‿1

!"»𝕩: Fill element of 𝕩 not known" % » 2‿2⥊{+}
!"«𝕩: Fill element of 𝕩 not known" % «˘2‿2⥊0‿' '

!"𝕨⥊𝕩: Fill element of 𝕩 needed for overtaking" % 2‿↑ ⥊ @‿1‿@

!">𝕩: Fill element of empty 𝕩 not known" % >0⥊{⇐}
!">𝕩: Fill element of empty 𝕩 not known" % >0‿0⥊{⇐}
