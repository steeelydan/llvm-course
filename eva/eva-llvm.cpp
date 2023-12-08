#include "./src/EvaLLVM.h"

int main(int argc, char const *argv[])
{
    std::string program = R"(

        (printf "\nValue: %d\n" 43)

    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}