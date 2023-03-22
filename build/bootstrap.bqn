"bootstrap.bqn: requires an argument of the path to mlochbaum/BQN" ! 1=≠•args
path ← •wdpath •file.At ⊑•args

prims ← 1⊑¨•primitives # list of primitive values
g ← 3 ↓ (•Type¨ prims) ⊔ •primitives # ⟨fns, 1-modifiers, 2-modifiers⟩, each being a pair of character & value
b2 ← •Import path∾"/src/bootstrap/boot2.bqn"                   # step 1: load boot2 with native compiler
b1 ← •CompObj prims B2 •FChars path∾"/src/bootstrap/boot1.bqn" # step 2: load boot1 with boot2 + •CompObj
c ← •CompObj ⟨prims, (⊑¨¨g)¨⟩ B1 •FChars path∾"/src/c.bqn"     # step 3: load c.bqn with boot1 + •CompObj
1 •CompObj c                                                   # step 4: replace global compiler with the gotten c.bqn
⟨path, "build/bytecodeLocal"⟩ •Import "genRuntime"             # step 5: generate runtime
