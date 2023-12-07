#include <string>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#ifndef EvaLLVM_h
#define EvaLLVM_h

class EvaLLVM
{
public:
    EvaLLVM()
    {
        moduleInit();
    }

    void exec(const std::string &program)
    {
        // 1. Parse the program
        // auto ast = parser->parser(program);

        // 2. Compile to LLVM IR:
        // compile(ast);

        // Print generated code
        module->print(llvm::outs(), nullptr);

        // 3. Save module IR to file:
        saveModuleToFile("./out.ll");
    }

private:
    void moduleInit()
    {
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("EvaLLVM", *ctx);
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    void saveModuleToFile(const std::string &fileName)
    {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(fileName, errorCode);
        module->print(outLL, nullptr);
    }

    /**
     * Owns & manages core 'global' data of LLVM's core infrastructure,
     * including type & constant unique tables
     */
    std::unique_ptr<llvm::LLVMContext> ctx;

    /**
     * Modules are top level containers of all other IR objects
     * Modules contain
     * - List of global variables
     * - List of functions
     * - List of libraries (or other modules) this module depends on
     * - Symbol table
     * - Various data about the target's characteristics
     *
     * A module maintains GlobalList object that holds all
     * constant references to global variables in the module
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * Provides uniform API for creating instructions and inserting them
     * into a basic block: Either at the end of a BasicBlock or
     * at a specific iterator location in a block
     */
    std::unique_ptr<llvm::IRBuilder<>> builder;
};

#endif