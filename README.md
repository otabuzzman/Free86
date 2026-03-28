# Free86

An Intel 80386 emulator for Linos, MacOS, and Winos. Compilation requires a C++ or Swift compiler, git, cmake, make, and when on Winos nmake. The emulator boots a Linux kernel into RAM and finally runs a shell, proving that protected mode works perfectly for Linos' needs. It also passes the [Test386 suite](https://github.com/barotto/test386.asm), proving many real mode and protected mode features ok. However, there are also some additional 80486 functions required by Linos, including the XADD, CPUID, and RDTSC instructions. Therefore, the name was chosen which sounds like a _three_ when pronounced, but also reflects the freedom of implementation.

**Missing features**
- Full segment limit and rights checks
- Full LOCK prefix not allowed handling
- Task gates, 16 bit interrupt and trap gates
- Test and debug registers

**Branches**
- `master`: Original repository fixed for Visual Studio Code
- `test386`: Fixed to additionally pass Test386 suite
- `refactor`: Rework of code structure and naming conventions

**Build on Linos (also WSL)**
- Install C++, CMake, Swift toolchain
- Run commands
  ```
  git clone https://github.com/otabuzzman/Free86.git ; cd Free86
  
  # build for Linos boot
  cmake -DNO_SDL=ON -DTEST386=OFF -G "Unix Makefiles" .
  
  # build for Test386 suite (see comment on setup below)
  cmake -DNO_SDL=ON -DTEST386=ON -G "Unix Makefiles" .
  
  # compile...
  make
  # ...and run 
  exe/free86
  
  # compile and run with Swift toolchain
  swift run -c release linos
  ```

**Build on MacOS**
- Install CMake, Xcode, Xcode Command Line Tools
- Run same commands as for Linos

**Build on Winos**
- Install CMake, nmake, Visual Studio Community /w C++, Swift toolchain
- Run commands
  ```
  git clone https://github.com/otabuzzman/Free86.git ; cd Free86
  
  # build for Linos boot
  cmake -DNO_SDL=ON -DTEST386=OFF -G "NMake Makefiles" .
  
  # build for Test386 suite (see comment on setup below)
  cmake -DNO_SDL=ON -DTEST386=ON -G "NMake Makefiles" .
  
  # compile...
  nmake
  # ...and run 
  exe\free86
  
  # compile and run with Swift toolchain
  swift run -c release linos
  ```

**Build on Winos/ Cygwin**
- Install development tools, cmake
- Run same commands as for Linos

**Test386**
- Install [NASM](https://www.nasm.us/pub/nasm/releasebuilds/) (Netwide Assembler)
- In the Test386 repo’s src/configuration.asm, set OUT_PORT to a non-zero value
- Run commands
  ```
  # clone repository beside Free86 folder
  https://github.com/barotto/test386.asm ; cd test386
  
  # assemble test suite (see README)
  nasm -i./src/ -f bin src/test386.asm -l test386.lst -o test386.bin
  
  ```
- Run test suite
  ```
  # cd to Free86 sibling folder
  cd ../Free86
  
  # run suite and capture results
  exe/free86 >test386-EE-reference.txt
  
  # run suite with Swift toolchain
  swift run -c release test386 >test386-EE-reference.txt
  
  # compare results
  diff ../test386.asm/test386-EE-reference.txt test386-EE-reference.txt
  ```
  Results comparison should look as in file `test386-EE-reference.diff`. Differences of ROL and RCL instructions are due to undefined OF bit.

**Acknowledgements**
- Parent repository [CPU-386-cpp](https://github.com/kxkx5150/CPU-80386-cpp) of this fork
- CPU-386-cpp might derive from [jslinux-deobfuscated](https://github.com/levskaya/jslinux-deobfuscated)
- jslinux-deobfuscated makes [JSLinux](https://bellard.org/jslinux/tech.html) from Fabrice Bellard readable
