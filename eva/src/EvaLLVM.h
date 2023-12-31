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

#define GEN_BINARY_OP(Op, varName)             \
    do                                         \
    {                                          \
        auto op1 = generate(exp.list[1], env); \
        auto op2 = generate(exp.list[2], env); \
                                               \
        return builder->Op(op1, op2, varName); \
    } while (false) // Not executed, but generates scope

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
        varsBuilder = std::make_unique<llvm::IRBuilder<>>(*ctx);
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

                if (auto localVar = llvm::dyn_cast<llvm::AllocaInst>(value))
                {
                    // Load local var onto stack
                    return builder->CreateLoad(localVar->getAllocatedType(), localVar, varName.c_str());
                }
                else if (auto globalVar = llvm::dyn_cast<llvm::GlobalVariable>(value))
                {
                    // Load global variable onto stack
                    // LLVM IR: looks like
                    // @VERSION = global i32 43, align 4
                    // ...
                    // %VERSION = load i32, i32* @VERSION, align 4
                    return builder->CreateLoad(globalVar->getInitializer()->getType(), globalVar, varName.c_str());
                }
                // Functions
                else
                {
                    return value;
                }
            }

        case ExpType::LIST:
            auto tag = exp.list[0];

            if (tag.type == ExpType::SYMBOL)
            {
                auto op = tag.string;

                if (op == "+")
                {
                    GEN_BINARY_OP(CreateAdd, "tmpadd");
                }
                else if (op == "-")
                {
                    GEN_BINARY_OP(CreateSub, "tmpsub");
                }
                else if (op == "*")
                {
                    GEN_BINARY_OP(CreateMul, "tmpmul");
                }
                else if (op == "/")
                {
                    GEN_BINARY_OP(CreateSDiv, "tmpdiv");
                }
                else if (op == ">")
                {
                    GEN_BINARY_OP(CreateICmpUGT, "tmpcmp");
                }
                else if (op == "<")
                {
                    GEN_BINARY_OP(CreateICmpULT, "tmpcmp");
                }
                else if (op == "==")
                {
                    GEN_BINARY_OP(CreateICmpEQ, "tmpcmp");
                }
                else if (op == "!=")
                {
                    GEN_BINARY_OP(CreateICmpNE, "tmpcmp");
                }
                else if (op == ">=")
                {
                    GEN_BINARY_OP(CreateICmpUGE, "tmpcmp");
                }
                else if (op == "<=")
                {
                    GEN_BINARY_OP(CreateICmpULE, "tmpcmp");
                }
                // (if <cond> <then> <else>)
                else if (op == "if")
                {
                    auto cond = generate(exp.list[1], env);

                    // Appended right away
                    auto thenBlock = createBB("then", fn);

                    // Else & if-end block: appended later to
                    // handle nested if expressions
                    auto elseBlock = createBB("else");
                    auto ifEndBlock = createBB("ifend");

                    builder->CreateCondBr(cond, thenBlock, elseBlock);

                    builder->SetInsertPoint(thenBlock);
                    auto thenResult = generate(exp.list[2], env);
                    builder->CreateBr(ifEndBlock);
                    // Restore the block to handle nested if-expressions
                    // This is needed for phi instruction
                    thenBlock = builder->GetInsertBlock();

                    // Append block to the function now
                    fn->getBasicBlockList().push_back(elseBlock);
                    builder->SetInsertPoint(elseBlock);
                    auto elseResult = generate(exp.list[3], env);
                    builder->CreateBr(ifEndBlock);

                    // Restore the block for phi instruction:
                    elseBlock = builder->GetInsertBlock();

                    fn->getBasicBlockList().push_back(ifEndBlock);
                    builder->SetInsertPoint(ifEndBlock);

                    // Result of the if expression is phi
                    auto phi = builder->CreatePHI(thenResult->getType(), 2, "tmpif");

                    phi->addIncoming(thenResult, thenBlock);
                    phi->addIncoming(elseResult, elseBlock);

                    return phi;
                }
                else if (op == "while")
                {
                    auto condBlock = createBB("cond", fn);
                    builder->CreateBr(condBlock);

                    auto bodyBlock = createBB("body");
                    auto loopEndBlock = createBB("loopend");

                    builder->SetInsertPoint(condBlock);
                    auto cond = generate(exp.list[1], env);

                    builder->CreateCondBr(cond, bodyBlock, loopEndBlock);

                    fn->getBasicBlockList().push_back(bodyBlock);
                    builder->SetInsertPoint(bodyBlock);
                    generate(exp.list[2], env);
                    builder->CreateBr(condBlock);

                    fn->getBasicBlockList().push_back(loopEndBlock);
                    builder->SetInsertPoint(loopEndBlock);

                    return builder->getInt32(0);
                }
                else if (op == "def")
                {
                    return compileFunction(exp, env);
                }
                // Variable declaration & init: (var x (+ y 10))
                // Typed: (var (x number) 42)
                else if (op == "var")
                {
                    auto varNameDecl = exp.list[1];
                    auto init = generate(exp.list[2], env);

                    auto varName = extractVarName(varNameDecl);
                    auto varType = extractVarType(varNameDecl);
                    auto varBinding = allocVar(varName, varType, env);

                    // Store on stack
                    return builder->CreateStore(init, varBinding);
                }
                else if (op == "set")
                {
                    auto value = generate(exp.list[2], env);

                    auto varName = exp.list[1].string;
                    auto varBinding = env->lookup(varName);

                    builder->CreateStore(value, varBinding);

                    return value;
                }
                // Blocks: (begin <expression>)
                else if (op == "begin")
                {
                    auto blockEnv = std::make_shared<Environment>(std::map<std::string, llvm::Value *>{}, env);

                    llvm::Value *blockResult;

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        blockResult = generate(exp.list[i], blockEnv);
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
                // Function calls
                else
                {
                    auto callable = generate(exp.list[0], env);

                    std::vector<llvm::Value *> args{};

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        args.push_back(generate(exp.list[i], env));
                    }

                    auto fn = (llvm::Function *)callable;

                    return builder->CreateCall(fn, args);
                }
            }
        }

        // Unreachable
        return builder->getInt32(0);
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

    std::string extractVarName(const Exp &exp)
    {
        return exp.type == ExpType::LIST ? exp.list[0].string : exp.string;
    }

    // Default: i32
    llvm::Type *extractVarType(const Exp &exp)
    {
        return exp.type == ExpType::LIST ? getTypeFromString(exp.list[1].string) : builder->getInt32Ty();
    }

    llvm::Type *getTypeFromString(const std::string &type_)
    {
        if (type_ == "number")
        {
            return builder->getInt32Ty();
        }

        if (type_ == "string")
        {
            // aka char*
            return builder->getInt8Ty()->getPointerTo();
        }

        // Default
        return builder->getInt32Ty();
    }

    bool hasReturnType(const Exp &fnExp)
    {
        return fnExp.list[3].type == ExpType::SYMBOL && fnExp.list[3].string == "->";
    }

    llvm::FunctionType *extractFunctionType(const Exp &fnExp)
    {
        auto params = fnExp.list[2];
        auto returnType = hasReturnType(fnExp) ? getTypeFromString(fnExp.list[4].string) : builder->getInt32Ty();

        std::vector<llvm::Type *> paramTypes{};

        for (auto &param : params.list)
        {
            auto paramType = extractVarType(param);
            paramTypes.push_back(paramType);
        }

        return llvm::FunctionType::get(returnType, paramTypes, false);
    }

    /**
     * Untyped: (def square (x) (* x x)) - i32 by default
     * Typed: (def square ((x number)) -> number (* x x))
     */
    llvm::Value *compileFunction(const Exp &fnExp, Env env)
    {
        auto fnName = fnExp.list[1].string;
        auto params = fnExp.list[2];
        auto body = hasReturnType(fnExp) ? fnExp.list[5] : fnExp.list[3];

        // Save current fn
        auto prevFn = fn;
        auto prevBlock = builder->GetInsertBlock();

        // Override fn to compile body
        auto newFn = createFunction(fnName, extractFunctionType(fnExp), env);
        fn = newFn;

        auto index = 0;
        auto fnEnv = std::make_shared<Environment>(std::map<std::string, llvm::Value *>{}, env);

        for (auto &arg : fn->args())
        {
            auto param = params.list[index++];
            auto argName = extractVarName(param);

            arg.setName(argName);

            // Allocate a local variable per argument to make arguments mutable
            auto argBinding = allocVar(argName, arg.getType(), fnEnv);
            builder->CreateStore(&arg, argBinding);
        }

        builder->CreateRet(generate(body, fnEnv));

        builder->SetInsertPoint(prevBlock);
        // Restore
        fn = prevFn;

        return newFn;
    }

    llvm::Value *allocVar(const std::string &name, llvm::Type *type_, Env env)
    {
        // Explicitly put stuff at the entry point of
        // our current function, regardless of where
        // the main builder is
        varsBuilder->SetInsertPoint(&fn->getEntryBlock());

        auto varAlloc = varsBuilder->CreateAlloca(type_, 0, name.c_str());

        env->define(name, varAlloc);

        return varAlloc;
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

    /**
     * Currently compiling function
     */
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
     * Extra builder for var declaration.
     * Always prepends to the beginning of the function
     * entry block.
     */
    std::unique_ptr<llvm::IRBuilder<>> varsBuilder;

    /**
     * Provides uniform API for creating instructions and inserting them
     * into a basic block: Either at the end of a BasicBlock or
     * at a specific iterator location in a block
     */
    std::unique_ptr<llvm::IRBuilder<>> builder;
};

#endif