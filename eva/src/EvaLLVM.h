#ifndef EvaLLVM_h
#define EvaLLVM_h

#include <string>
#include <regex>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "./parser/EvaParser.h"

using syntax::EvaParser;

class EvaLLVM
{
public:
    EvaLLVM()
        : parser(std::make_unique<EvaParser>())
    {
        moduleInit();
        setupExternalFunctions();
    }

    void exec(const std::string &program)
    {
        auto ast = parser->parse(program);

        compile(ast);

        module->print(llvm::outs(), nullptr);

        saveModuleToFile("./out.ll");
    }

private:
    void moduleInit()
    {
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("EvaLLVM", *ctx);
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    std::unique_ptr<EvaParser> parser;

    void saveModuleToFile(const std::string &fileName)
    {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(fileName, errorCode);
        module->print(outLL, nullptr);
    }

    void compile(const Exp &ast)
    {
        fn = createFunction("main", llvm::FunctionType::get(builder->getInt32Ty(), false));

        gen(ast);

        builder->CreateRet(builder->getInt32(0));
    }

    llvm::Value *gen(const Exp &exp)
    {

        switch (exp.type)
        {
        case ExpType::NUMBER:
            return builder->getInt32(exp.number);

        case ExpType::STRING:
        {
            auto re = std::regex("\\\\n");
            auto str = std::regex_replace(exp.string, re, "\n");

            return builder->CreateGlobalStringPtr(str);
        }

        case ExpType::SYMBOL:
            // Boolean
            if (exp.string == "true" || exp.string == "false")
            {
                return builder->getInt1(exp.string == "true" ? true : false);
            }
            // TODO

        case ExpType::LIST:
            auto tag = exp.list[0];

            if (tag.type == ExpType::SYMBOL)
            {
                auto op = tag.string;

                if (op == "printf")
                {
                    auto printFn = module->getFunction("printf");
                    std::vector<llvm::Value *> args{};

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        args.push_back(gen(exp.list[i]));
                    }

                    return builder->CreateCall(printFn, args);
                }
            }
        }

        // Unreachable
        builder->getInt32(0);
    }

    /**
     *  External functions from libc++
     */
    void setupExternalFunctions()
    {
        auto bytePtrTy = builder->getInt8Ty()->getPointerTo();

        module->getOrInsertFunction("printf",
                                    llvm::FunctionType::get(/* return type */ builder->getInt32Ty(), /* format arg */ bytePtrTy, /* vararg */ true));
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

    llvm::Function *fn;

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