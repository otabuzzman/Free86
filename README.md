# Free86

An Intel 80386 emulator for Linos, MacOS, and Winos. Compilation requires a C++ or Swift compiler, git, cmake, make, and nmake when on Winos. The emulator boots a Linux kernel into RAM and finally runs a shell, proving that protected mode works perfectly for Linos' needs. It also passes the [Test386 suite](https://github.com/barotto/test386.asm), proving many real mode and protected mode features ok.

**Missing features**
- Full segment limit and rights checks
- Full LOCK prefix not allowed handling
- Task gates, 16 bit interrupt and trap gates
- Test and debug registers

**Build on Linos (also WSL)**
- Install C++, CMake, Swift toolchain
- Run commands
  ```
  git clone https://github.com/otabuzzman/Free86.git ; cd Free86
  
  # build for Linos boot and Test386 suite (see comment on setup below)
  (rm -rf build ; mkdir build ; cd build ; cmake -G "Unix Makefiles" ..)
  
  # compile...
  (cd build ; make)
  # ...and run Linos
  exe/linos
  
  # compile and run with Swift toolchain
  stty -icanon -echo ; swift run -c release linos
  ```

**Build on MacOS**
- Install CMake, Xcode, Xcode Command Line Tools
- Run same commands as for Linos

**Build on Winos**
- Install CMake, nmake, Visual Studio Community /w C++, Swift toolchain
- Run commands
  ```
  git clone https://github.com/otabuzzman/Free86.git ; cd Free86
  
  # build for Linos boot and Test386 suite (see comment on setup below)
  rmdir /s /q build & md build & cd build & cmake -G "NMake Makefiles" .. & cd ..
  
  # compile...
  cd build & nmake & cd ..
  # ...and run Linos
  exe\linos
  
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
  # clone repository into Free86 sibling folder
  git clone https://github.com/barotto/test386.asm.git ; cd test386.asm
  git checkout fde2686 # newer commits fail due to unsupported features
  
  # assemble test suite (see README)
  nasm -i./src/ -f bin src/test386.asm -l test386.lst -o test386.bin
  
  ```
- Run test suite
  ```
  # cd to Free86 sibling folder
  cd ../Free86
  
  # run suite and capture results
  exe/test386 >_out
  
  # run suite with Swift toolchain
  swift run -c release test386 >_out
  
  # compare results with test suite
  diff ../test386.asm/test386-EE-reference.txt _out >_out2
  
  # compare differences with known deviations (none expected)
  diff _out2 test386-EE-reference.diff
  ```

**References**
- Intel 80386 Programmer's Reference Manual ([PDF](https://css.csail.mit.edu/6.858/2019/readings/i386.pdf), [HTML](https://pdos.csail.mit.edu/6.828/2008/readings/i386/toc.htm))
- [Intel 80386 Hardware Reference Manual](https://www.dosdays.co.uk/media/intel/1986_80386_Hardware_Reference_Manual.pdf) (PDF)
- [Intel 80386 DX Microprocessor Data Sheet](https://datasheets.chipdb.org/Intel/x86/386/datashts/23163011.pdf) (PDF)
- [Intel Processor Identification and the CPUID Instruction Application Note](https://ardent-tool.com/CPU/docs/Intel/CPUID/241618-005.pdf) (PDF)
- [IA-32 Architectures Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

**Acknowledgements**
- This repository is based on [CPU-386-cpp](https://github.com/kxkx5150/CPU-80386-cpp)
- CPU-386-cpp probably derived from [jslinux-deobfuscated](https://github.com/levskaya/jslinux-deobfuscated)
- jslinux-deobfuscated makes [JSLinux](https://bellard.org/jslinux/tech.html) from Fabrice Bellard readable
