# Bitsliced-Primates

This repo contains:
  - A bit sliced implementation of the permutation used in the PRIMATEs family of ciphers (http://primates.ae/) for the 80- and 120-bit security levels. Both of the bit sliced permutations can process eight PRIMATEs encryptions simultanously. 

  - Three new modes of operation for PRIMATEs (APE-BS, HANUMAN-BS and GIBBON-BS), which can process input in eight parallel states to utilize the bit sliced permutation and speed up encryption. 

  - Benchmarking code for the:
    - Three new modes of operation for both security-levels (located in Bitsliced-Primates/Benchmarks/Bitsliced/).
    - Existing reference implementations for both security-levels (located in Bitsliced-Primates/Benchmarks/Reference/).
    - Bit sliced and reference permutation in isolation (located in Bitsliced-Primates/Benchmarks/Other/).
    - Various bit sliced mix columns implementations, we came up with (located in Bitsliced-Primates/Benchmarks/Other/).

  - Code to multiply the PRIMATEs mix columns matrices with themselves to create new matrices, which could be used in the mix columns transformation (located in Bitsliced-Primates/Tools/MixColumnsMatrixCalculator/)

  - My thesis report on the subject of implementing PRIMATEs with bit slicing (located in Bitsliced-Primates/). The newest version of the thesis will always be available here.
  
  
All the code has been written in the language C. It has been tested with the GCC, ICC, and MSVC compilers for the C language and compiles fine with all of them.

Feel free to open an issue, if there are any questions to be had! However, please have read the (relevant parts of the) report before opening any questions. 

EDIT:
I have successfully defended my thesis. I will be uploading the slides in a few days, when I get the opportunity. They might be of interest, although they for the most part repeat the thesis-report. I also had the opportunity of testing GIBBON80-BS on a Skylake CPU (core-i5 mobile CPU). It yielded a 6.5% performance increase over my current results, which are from a Broadwell CPU.
