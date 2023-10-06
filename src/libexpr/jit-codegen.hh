#pragma once

#include "nixexpr.hh"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

namespace nix::jit {

/**
 * Code-generation on nix ASTs.
 */
class NixCodeGen
{
private:

    llvm::LLVMContext & llvmCtx;
    llvm::IRBuilder<> & builder;
    llvm::Value * genInt(const ExprInt & integer);
    llvm::Value * genInt(Value v);
    llvm::Value * genFloat(Value v);
    llvm::Value * genFloat(const ExprFloat & fp);
    llvm::Value * genCall(ExprCall & Call, nix::SymbolTable & symbols);

public:

    NixCodeGen(llvm::LLVMContext & llvmCtx, llvm::IRBuilder<> & builder)
        : llvmCtx(llvmCtx)
        , builder(builder)
    {
    }

    /**
     * Generate code from an expression.
     * \param e the expression
     * \returns the generated llvm IR, if it fails, return nullptr
     */
    llvm::Value * gen(const Expr * e);
};

} // namespace nix::jit
