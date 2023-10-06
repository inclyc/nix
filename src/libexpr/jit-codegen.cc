#include "nixexpr.hh"
#include "jit-codegen.hh"
#include "value.hh"

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>

namespace nix::jit {

using llvm::APFloat;
using llvm::APInt;
using llvm::ConstantFP;
using llvm::ConstantInt;

llvm::Value * NixCodeGen::genInt(const ExprInt & integer)
{
    return genInt(integer.v);
}

llvm::Value * NixCodeGen::genInt(Value v)
{
    // Integers in nix language are always 64-bit and signed
    assert(v.type() == nInt);
    APInt i(64, v.integer, /*isSigned=*/true);
    return ConstantInt::get(llvmCtx, i);
}

llvm::Value * NixCodeGen::genFloat(Value v)
{
    assert(v.type() == nFloat);
    return ConstantFP::get(llvmCtx, APFloat(v.fpoint));
}

llvm::Value * NixCodeGen::genFloat(const ExprFloat & fp)
{
    return genFloat(fp.v);
}

llvm::Value * NixCodeGen::gen(const Expr * e)
{
    if (!e)
        return nullptr;

    // Firstly we need to dispatch types of the expression
    // TODO: use switch-case stuff here
    if (const auto * integer = dynamic_cast<const ExprInt *>(e)) {
        return genInt(*integer);
    }

    return nullptr;
}

/// We implict cast integers to float types if they are different
llvm::Type * implictExt(llvm::Value *& LHS, llvm::Value *& RHS, llvm::IRBuilder<> & builder)
{
    if (LHS->getType() != RHS->getType()) {
        bool LHSInt = LHS->getType()->isIntegerTy();
        llvm::Value *& intSide = LHSInt ? LHS : RHS;
        llvm::Type * Ty = LHSInt ? RHS->getType() : LHS->getType();
        intSide = builder.CreateSIToFP(intSide, Ty);
    }
    return LHS->getType();
}

llvm::Value * NixCodeGen::genCall(ExprCall & call, nix::SymbolTable & symbols)
{
    if (auto * fun = dynamic_cast<nix::ExprVar *>(call.fun)) {
        // the function is actually an variable.
        std::string sym = symbols[fun->name];
        if (sym == "__sub") {
            // A - B

            assert(call.args.size() == 2);

            llvm::Value * LHS = gen(call.args[0]);
            llvm::Value * RHS = gen(call.args[1]);

            if (!LHS || !RHS)
                return nullptr;

            llvm::Type * ty = implictExt(LHS, RHS, builder);

            if (ty->isIntegerTy())
                return builder.CreateSub(LHS, RHS, "sub");
            if (ty->isFloatingPointTy())
                return builder.CreateFSub(LHS, RHS, "fsub");

        } else if (sym == "__mul") {
            // A * B

            assert(call.args.size() == 2);

            llvm::Value * LHS = gen(call.args[0]);
            llvm::Value * RHS = gen(call.args[1]);

            if (!LHS || !RHS)
                return nullptr;

            llvm::Type * ty = implictExt(LHS, RHS, builder);

            if (ty->isIntegerTy())
                return builder.CreateMul(LHS, RHS, "mul");
            if (ty->isFloatingPointTy())
                return builder.CreateFMul(LHS, RHS, "fmul");

        } else if (sym == "__div") {
            // A / B

            assert(call.args.size() == 2);

            llvm::Value * LHS = gen(call.args[0]);
            llvm::Value * RHS = gen(call.args[1]);

            if (!LHS || !RHS)
                return nullptr;

            llvm::Type * ty = implictExt(LHS, RHS, builder);

            if (ty->isIntegerTy())
                return builder.CreateSDiv(LHS, RHS, "div");
            if (ty->isFloatingPointTy())
                return builder.CreateFSub(LHS, RHS, "fdiv");
        } else if (sym == "__lessThan") {
            // A < B

            assert(call.args.size() == 2);

            llvm::Value * LHS = gen(call.args[0]);
            llvm::Value * RHS = gen(call.args[1]);

            if (!LHS || !RHS)
                return nullptr;

            llvm::Type * ty = implictExt(LHS, RHS, builder);

            if (ty->isIntegerTy())
                return builder.CreateICmpSLE(LHS, RHS, "icmp.sle");
            if (ty->isFloatingPointTy())
                return builder.CreateFCmpOLE(LHS, RHS, "fcmp.ole");
        }
    }
    return nullptr;
}

} // namespace nix::jit
