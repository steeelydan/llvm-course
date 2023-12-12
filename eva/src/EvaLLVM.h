#ifndef EvaLLVM_h
#define EvaLLVM_h

#include <string>
#include <regex>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "./parser/EvaParser.h"
#include "./Environment.h"

using syntax::EvaParser;
using Env = std::shared_ptr<Environment>;

class EvaLLVM
{
public:
    EvaLLVM()
        : parser(std::make_unique<EvaParser>())
    {
        moduleInit();
        setupExternalFunctions();
        setupGlobalEnvironment();
    }

    void exec(const std::string &program)
    {
        auto ast = parser->parse("(begin " + program + ")");

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

    Env GlobalEnv;

    void setupGlobalEnvironment()
    {
        std::map<std::string, llvm::Value *> globalObject{
            {"VERSION", builder->getInt32(44)}};

        std::map<std::string, llvm::Value *> globalRec{};

        for (auto &entry : globalObject)
        {
            globalRec[entry.first] = createGlobalVar(entry.first, (llvm::Constant *)entry.second);
        }

        GlobalEnv = std::make_shared<Environment>(globalRec, nullptr);
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

    void saveModuleToFile(const std::string &fileName)
    {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(fileName, errorCode);
        module->print(outLL, nullptr);
    }

    void compile(const Exp &ast)
    {
        fn = createFunction("main", llvm::FunctionType::get(builder->getInt32Ty(), false), GlobalEnv);

        generate(ast, GlobalEnv);

        builder->CreateRet(builder->getInt32(0));
    }

    llvm::Value *generate(const Exp &exp, Env env)
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
            else
            {
                auto varName = exp.string;
                auto value = env->lookup(varName);

                if (auto globalVar = llvm::dyn_cast<llvm::GlobalVariable>(value))
                {
                    // Load global variable onto stack
                    // LLVM IR: looks like
                    // @VERSION = global i32 43, align 4
                    // ...
                    // %VERSION = load i32, i32* @VERSION, align 4
                    return builder->CreateLoad(globalVar->getInitializer()->getType(), globalVar, varName.c_str());
                }

                // Todo local var
            }

        case ExpType::LIST:
            auto tag = exp.list[0];

            if (tag.type == ExpType::SYMBOL)
            {
                auto op = tag.string;

                // Variable declaration & init: (var x (+ y 10))
                if (op == "var")
                {
                    auto varName = exp.list[1].string;
                    auto init = generate(exp.list[2], env);

                    return createGlobalVar(varName, (llvm::Constant *)init)->getInitializer();
                }
                // Blocks: (begin <expression>)
                else if (op == "begin")
                {
                    llvm::Value *blockResult;

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        // Todo: local env for blocks
                        blockResult = generate(exp.list[i], env);
                    }

                    return blockResult;
                }
                // printf(): (printf "Value: %d" 42)
                else if (op == "printf")
                {
                    auto printFn = module->getFunction("printf");
                    std::vector<llvm::Value *> args{};

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        args.push_back(generate(exp.list[i], env));
                    }

                    return builder->CreateCall(printFn, args);
                }
            }
        }

        // Unreachable
        builder->getInt32(0);
    }

    llvm::GlobalVariable *createGlobalVar(const std::string &name, llvm::Constant *init)
    {
        module->getOrInsertGlobal(name, init->getType());
        auto variable = module->getNamedGlobal(name);
        variable->setAlignment(llvm::MaybeAlign(4));
        variable->setConstant(false);
        variable->setInitializer(init);

        return variable;
    }

    /**
     * Functions consist of:
     * - Function type: Param types / return type / varargs?
     * - Blocks; always there: entry block
     *     - Optimization takes place here
     * - Control flow blocks: Branch instructions: Conditionals, jumps
     */
    llvm::Function *createFunction(const std::string &fnName, llvm::FunctionType *fnType, Env env)
    {
        // Function prototype might already be defined
        auto fn = module->getFunction(fnName);

        if (fn == nullptr)
        {
            fn = createFunctionProto(fnName, fnType, env);
        }

        createFunctionBlock(fn);

        return fn;
    }

    llvm::Function *createFunctionProto(const std::string &fnName, llvm::FunctionType *fnType, Env env)
    {
        auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);

        verifyFunction(*fn);

        env->define(fnName, fn);

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