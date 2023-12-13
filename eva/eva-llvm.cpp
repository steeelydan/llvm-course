#include "./src/EvaLLVM.h"

int main(int argc, char const *argv[])
{
    std::string program = R"(

        // (printf "\nValue: %d\n" 43)

        // (printf "True: %d\n\n" true)

        // (var VERSION 42)
        // (printf "Version: %d\n\n" VERSION)
        // (printf "Version: %d\n\n" (var VERSION 43))

        // (var x 39)
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
        // (var z 32)
        // (var x (+ z 11))
        // (printf "X: %d\n" x)

        // (printf "Is X == 42?: %d\n" (== x 42))
        // (printf "Is X > 42?: %d\n" (> x 42))

        // (if (== x 42)
        //     (set x 100)
        //     (set x 200))

        // (printf "X: %d\n" x)

        // (if (!= x 42)
        //     (if (> x 42)
        //         (set x 300)
        //         (set x 200))
        //     (set x 100))

        // (printf "X: %d\n" x)

        // (var x 10)

        // (while (> x 0)
        //     (begin
        //         (set x (- x 1))
        //         (printf "%d " x)
        //         )
        //     )

        (def square (x) (* x x))
        (square 2)

        (def sum ((a number) (b number)) -> number (+ a b))
        (sum 2 3)

        (printf "\n(+ (square 2) (sum 2 3)): %d\n" (+ (square 2) (sum 2 3)))
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}