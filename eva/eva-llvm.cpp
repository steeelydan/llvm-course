#include "./src/EvaLLVM.h"

int main(int argc, char const *argv[])
{
    std::string program = R"(

        (printf "True: %d\n\n" true)

    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}