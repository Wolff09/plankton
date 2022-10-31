#pragma once
#ifndef PLANKTON_ENGINE_EFFECT_HPP
#define PLANKTON_ENGINE_EFFECT_HPP

#include <ostream>
#include "programs/ast.hpp"
#include "logics/ast.hpp"


namespace plankton {

    struct SymbolicHeapEquality;
    struct SymbolicHeapFlow;

    struct EffectVisitor {
        virtual void Visit(const SymbolicHeapEquality& obj) = 0;
        virtual void Visit(const SymbolicHeapFlow& obj) = 0;
    };

    struct SymbolicHeapExpression {
        virtual ~SymbolicHeapExpression() = default;
        virtual void Accept(EffectVisitor& visitor) const = 0;
    };

    struct SymbolicHeapEquality : public SymbolicHeapExpression {
        BinaryOperator op;
        std::unique_ptr <SymbolicVariable> lhsSymbol;
        std::string lhsFieldName;
        std::unique_ptr<SymbolicExpression> rhs;

        explicit SymbolicHeapEquality(BinaryOperator op, const SymbolDeclaration& lhsSymbol, std::string lhsField, std::unique_ptr<SymbolicExpression> rhs);
        void Accept(EffectVisitor& visitor) const override { visitor.Visit(*this); }
    };

    struct SymbolicHeapFlow : public SymbolicHeapExpression {
        std::unique_ptr <SymbolicVariable> symbol;
        bool isEmpty;

        explicit SymbolicHeapFlow(const SymbolDeclaration& symbol, bool empty);
        void Accept(EffectVisitor& visitor) const override { visitor.Visit(*this); }
    };

    struct HeapEffect final {
        std::unique_ptr <SharedMemoryCore> pre; // memory before update
        std::unique_ptr <SharedMemoryCore> post; // memory after update
        std::unique_ptr <Formula> context; // stack context of pre/post (not a frame!)
        std::vector <std::unique_ptr<SymbolicHeapExpression>> preHalo; // stack context beyond pre
        std::vector <std::unique_ptr<SymbolicHeapExpression>> postHalo; // stack context beyond post

        explicit HeapEffect(std::unique_ptr <SharedMemoryCore> pre, std::unique_ptr <SharedMemoryCore> post, std::unique_ptr <Formula> context);

        explicit HeapEffect(std::unique_ptr <SharedMemoryCore> pre, std::unique_ptr <SharedMemoryCore> post, std::unique_ptr <Formula> context,
                            std::vector <std::unique_ptr<SymbolicHeapExpression>> preHalo, std::vector <std::unique_ptr<SymbolicHeapExpression>> postHalo);

        explicit HeapEffect(std::unique_ptr <SharedMemoryCore> pre, std::unique_ptr <SharedMemoryCore> post, std::unique_ptr <Formula> context,
                            std::deque <std::unique_ptr<SymbolicHeapExpression>> preHalo, std::deque <std::unique_ptr<SymbolicHeapExpression>> postHalo);
    };

    std::unique_ptr<SymbolicHeapExpression> Copy(const SymbolicHeapExpression& obj);
    std::unique_ptr<SymbolicHeapEquality> Copy(const SymbolicHeapEquality& obj);
    std::unique_ptr<SymbolicHeapFlow> Copy(const SymbolicHeapFlow& obj);

    std::ostream& operator<<(std::ostream& out, const SymbolicHeapExpression& object);
    std::ostream& operator<<(std::ostream& out, const SymbolicHeapEquality& object);
    std::ostream& operator<<(std::ostream& out, const SymbolicHeapFlow& object);
    std::ostream& operator<<(std::ostream& out, const HeapEffect& object);

}

#endif //PLANKTON_ENGINE_EFFECT_HPP