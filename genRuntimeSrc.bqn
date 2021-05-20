#!/usr/bin/env ./BQN
args←•args
"Usage: ./getRuntime.bqn path/to/mlochbaum/BQN"!1≤≠args
path←⊑•args
CC ← {𝕨 •FChars ⟨1,path,"-i",𝕩⟩ •Import "cc.bqn"}
 "src/runtime0"CC"r0"
 "src/runtime1"CC"r1"
 "src/compiler"CC"c"
"src/formatter"CC"f"
