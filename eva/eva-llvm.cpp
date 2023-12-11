#include "./src/EvaLLVM.h"

int main(int argc, char const *argv[])
{
    std::string program = R"(

        // (printf "\nValue: %d\n" 43)
        // (printf "True: %d\n\n" true)
        (var VERSION 42)
        // (printf "Version: %d\n\n" VERSION)

    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}