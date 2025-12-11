What's up. Somehow this repo doesn't have a README. Here is a small one so you can reproduce the steps
I took to generate the .prf.unfold file from the program equivalence.c which tests for equivalence
between Montgomery modulo multiplication and typical (affine) modulo multiplication. Do this:

1. cbmc equivalence.c --dimacs --outfile miter.cnf 
    - This generates miter.cnf as the .cnf file consisting of the miter harness found in equivalence.c 
    - equivalence.c is well commented and hopefully does not need explaining. It is a 4-bit multiplier.
    - NB: It is critical that you remove the garbage output lines at the bottom of miter.cnf
2. picosat -T proof.TRACECHECK miter.cnf
    - This generates the TraceCheck of the miter using PicoSAT. Essentially, this is the first look
    we get at the resolution proof we are proving in ZK.
3. python3 prover_backend/Sort.py proof.TRACECHECK > proof.prf
    - This does two things. It sorts the TraceCheck and it puts them in .prf format (indices labeled).
    - For an 8-bit multiplier, this hangs forever (literally hours) on my M1 Macbook. Maybe if you have
    a beefier computer you can remove the MASK statements in equivalence.c and check if it returns.
4. python3 prover_backend/unfold_proof.py proof.prf
    - This unfolds the .prf to get you the .prf.unfold which you use as your prooffile in the ZKUNSAT
    main program