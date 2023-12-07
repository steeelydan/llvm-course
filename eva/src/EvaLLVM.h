#include <string>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

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
        compile();

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

    void compile()
    {
        fn = createFunction("main", llvm::FunctionType::get(builder->getInt32Ty(), false));

        auto result = gen();

        auto i32Result = builder->CreateIntCast(result, builder->getInt32Ty(), true);

        builder->CreateRet(i32Result);
    }

    llvm::Value *gen()
    {
        return builder->getInt32(42);
    }

    /**
     * Functions consist of:
     * - Function type: Param types / return type / varargs?
     * - Blocks; always there: entry block
     *     - Optimization takes place here
     * - Control flow blocks: Branch instructions: Conditionals, jumps
     */
    llvm::Function *createFunction(const std::string &fnName, llvm::FunctionType *fnType)
    {
        // Function prototype might already be defined
        auto fn = module->getFunction(fnName);

        if (fn == nullptr)
        {
            fn = createFunctionProto(fnName, fnType);
        }

        createFunctionBlock(fn);

        return fn;
    }

    llvm::Function *createFunctionProto(const std::string &fnName, llvm::FunctionType *fnType)
    {
        auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);

        verifyFunction(*fn);

        return fn;
    }

    void createFunctionBlock(llvm::Function *fn)
    {
        auto entry = createBB("entry", fn);
        // Emit code exactly into this block
        builder->SetInsertPoint(entry);
    }

    /**
     * If `fn` is passed, block is automatically appended to
     * the parent function.
     * Otherwise, block should later be appended manually via
     * fn->getBasicBlockList().push_back(block)
     */
    llvm::BasicBlock *createBB(std::string name, llvm::Function *fn = nullptr)
    {
        return llvm::BasicBlock::Create(*ctx, name, fn);
    }

    llvm::Function* fn;

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