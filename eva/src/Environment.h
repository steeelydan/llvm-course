#ifndef Environment_h
#define Environment_h

#include <map>
#include <string>
#include "llvm/IR/Value.h"
#include "./Logger.h"

class Environment : public std::enable_shared_from_this<Environment>
{
public:
    Environment(std::map<std::string, llvm::Value *> record, std::shared_ptr<Environment> parent) : record_(record), parent_(parent)
    {
    }

    // Create variable
    llvm::Value *define(const std::string &name, llvm::Value *value)
    {
        record_[name] = value;

        return value;
    }

    // Access variable
    llvm::Value *lookup(const std::string &name)
    {
        return resolve(name)->record_[name];
    }

private:
    // Traverse environment chain
    std::shared_ptr<Environment> resolve(const std::string &name)
    {
        if (record_.count(name) != 0)
        {
            return shared_from_this();
        }

        if (parent_ == nullptr)
        {
            DIE << "Variable \"" << name << "\" is not defined.";
        }

        return parent_->resolve(name);
    }

    // Bindings storage
    std::map<std::string, llvm::Value *> record_;

    // Parent environment link
    std::shared_ptr<Environment> parent_;
};

#endif