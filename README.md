# LLVM

Dmitry Soshnikov: Programming Language with LLVM (https://www.udemy.com/course/programming-language-with-llvm/)

## Humble Beginnings

-   First LLVM file: `clang++ -S -emit-llvm hello.cpp`
-   Generate binary from LLVM file: `clang++ -o hello hello.ll`
-   Generate bitcode: `llvm-as hello.ll`
    -   Display in terminal: `hexdump -C hello.bc`
    -   Disassemble bitcode: `llvm-dis hello.bc -o hello-dis.ll`
-   Generate native assembly: `clang++ -S hello.ll`
-   LLVM interpreter: `lli hello.ll`
-   Minimal LLVM program: `minimum.ll`

## Eva

-   Compile:
    ```bash
    clang++ -o eva-llvm `llvm-config --cxxflags --ldflags --system-libs --libs core` eva-llvm.cpp
    ```
