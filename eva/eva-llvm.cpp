#include "./src/EvaLLVM.h"

int main(int argc, char const *argv[])
{
    std::string program = R"(

        // (printf "\nValue: %d\n" 43)

        // (printf "True: %d\n\n" true)

        (var VERSION 42)
        // (printf "Version: %d\n\n" VERSION)
        // (printf "Version: %d\n\n" (var VERSION 43))

        // (var x 42)
        // (begin
        //     (var (x string) "Hello")
        //     (printf "x: %s\n\n" x))
        // (printf "x: %d\n\n" x)
        // (set x 100)
        // (printf "x: %d\n\n" x)

        // Optimized away in IR
        // (var x (+ 32 15))
        // (printf "X: %d\n" x)

        // See addition in IR
        (var z 32)
        (var x (+ z 11))
        (printf "X: %d\n" x)

        // (if (== x 42)
        //     (set x 100)
        //     (set x 200))
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}