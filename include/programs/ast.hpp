#pragma once
#ifndef PLANKTON_PROGRAMS_AST_HPP
#define PLANKTON_PROGRAMS_AST_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <ostream>
#include <optional>
#include <cassert>
#include "visitors.hpp"

namespace plankton {
    
    struct AstNode {
        explicit AstNode() = default;
        AstNode(const AstNode& other) = delete;
        virtual ~AstNode() = default;

        virtual void Accept(ProgramVisitor& visitor) const = 0;
        virtual void Accept(MutableProgramVisitor& visitor) = 0;
    };

    #define ACCEPT_PROGRAM_VISITOR \
        void Accept(ProgramVisitor& visitor) const override { visitor.Visit(*this); } \
        void Accept(MutableProgramVisitor& visitor) override { visitor.Visit(*this); }

    //
    // Types, Variables
    //
    
    enum struct Sort {
        VOID, BOOL, DATA, PTR, TID
    };

    struct Type final {
        std::string name;
        Sort sort;
        std::map<std::string, std::reference_wrapper<const Type>> fields;
        
        explicit Type(std::string name, Sort sort);
        Type(const Type& other) = delete;
    
        [[nodiscard]] decltype(fields)::const_iterator begin() const;
        [[nodiscard]] decltype(fields)::const_iterator end() const;
        
        [[nodiscard]] bool operator==(const Type& other) const;
        [[nodiscard]] bool operator!=(const Type& other) const;

        [[nodiscard]] const Type& operator[](const std::string& fieldName) const;
        [[nodiscard]] std::optional<std::reference_wrapper<const Type>> GetField(const std::string& fieldName) const;
        
        [[nodiscard]] bool AssignableTo(const Type& other) const;
        [[nodiscard]] bool AssignableFrom(const Type& other) const;
        [[nodiscard]] bool Comparable(const Type& other) const;
        
        static const Type& Bool();
        static const Type& Data();
        static const Type& Null();
        static const Type& Thread();
    };

    struct VariableDeclaration final {
        std::string name;
        const Type& type;
        bool isShared;
        
        explicit VariableDeclaration(std::string name, const Type& type, bool shared);
        VariableDeclaration(const VariableDeclaration&) = delete;
        
        [[nodiscard]] bool operator==(const VariableDeclaration& other) const;
        [[nodiscard]] bool operator!=(const VariableDeclaration& other) const;
    };
    
    //
    // Programs, Functions
    //

    struct Function final : public AstNode {
        enum Kind { API, MACRO, INIT };
        
        std::string name;
        Kind kind;
        std::vector<std::reference_wrapper<const Type>> returnType;
        std::vector<std::unique_ptr<VariableDeclaration>> parameters;
        std::unique_ptr<Scope> body;
        
        explicit Function(std::string name, Kind kind, std::unique_ptr<Scope> body);
        ACCEPT_PROGRAM_VISITOR
    };

    struct Program final : public AstNode {
        std::string name;
        std::vector<std::unique_ptr<Type>> types;
        std::vector<std::unique_ptr<VariableDeclaration>> variables;
        std::unique_ptr<Function> initializer;
        std::vector<std::unique_ptr<Function>> macroFunctions;
        std::vector<std::unique_ptr<Function>> apiFunctions;
        // std::map<std::string, std::string> options;

        explicit Program(std::string name, std::unique_ptr<Function> initializer);
        ACCEPT_PROGRAM_VISITOR
    };
    
    //
    // Expressions
    //
    
    struct Expression : AstNode {
        [[nodiscard]] virtual const Type& GetType() const = 0;
        [[nodiscard]] virtual Sort GetSort() const { return GetType().sort; };
    };
    
    struct ValueExpression : Expression {
    };
    
    struct SimpleExpression : ValueExpression {
    };

    struct VariableExpression final : public SimpleExpression {
        std::reference_wrapper<const VariableDeclaration> decl;
        
        explicit VariableExpression(const VariableDeclaration& decl);
        [[nodiscard]] inline const VariableDeclaration& Decl() const { return decl; }
        [[nodiscard]] const Type& GetType() const override;
        ACCEPT_PROGRAM_VISITOR
    };

    struct TrueValue final : public SimpleExpression {
        explicit TrueValue() = default;
        [[nodiscard]] const Type& GetType() const override;
        ACCEPT_PROGRAM_VISITOR
    };

    struct FalseValue final : public SimpleExpression {
        explicit FalseValue() = default;
        [[nodiscard]] const Type& GetType() const override;
        ACCEPT_PROGRAM_VISITOR
    };

    struct MinValue final : public SimpleExpression {
        explicit MinValue() = default;
        [[nodiscard]] const Type& GetType() const override;
        ACCEPT_PROGRAM_VISITOR
    };

    struct MaxValue final : public SimpleExpression {
        explicit MaxValue() = default;
        [[nodiscard]] const Type& GetType() const override;
        ACCEPT_PROGRAM_VISITOR
    };

    struct NullValue final : public SimpleExpression {
        explicit NullValue() = default;
        [[nodiscard]] const Type& GetType() const override;
        ACCEPT_PROGRAM_VISITOR
    };

    struct Dereference final : public ValueExpression {
        std::unique_ptr<VariableExpression> variable;
        std::string fieldName;
        
        explicit Dereference(std::unique_ptr<VariableExpression> variable, std::string fieldName);
        [[nodiscard]] const Type& GetType() const override;
        ACCEPT_PROGRAM_VISITOR
    };
    
    enum struct BinaryOperator {
        EQ, NEQ, LEQ, LT, GEQ, GT
    };
    
    BinaryOperator Symmetric(BinaryOperator op);
    BinaryOperator Negate(BinaryOperator op);
    
    struct BinaryExpression final : public Expression {
        BinaryOperator op;
        std::unique_ptr<ValueExpression> lhs;
        std::unique_ptr<ValueExpression> rhs;
    
        explicit BinaryExpression(BinaryOperator op_, std::unique_ptr<ValueExpression> lhs_,
                                  std::unique_ptr<ValueExpression> rhs_);
        [[nodiscard]] const plankton::Type& GetType() const override;
        ACCEPT_PROGRAM_VISITOR
    };
    
    //
    // Statements
    //
    
    struct Statement : public AstNode {
    };

    struct Scope final : public Statement {
        std::vector<std::unique_ptr<VariableDeclaration>> variables;
        std::unique_ptr<Statement> body;
        
        explicit Scope(std::unique_ptr<Statement> body);
        ACCEPT_PROGRAM_VISITOR
    };

    struct Atomic final : public Statement {
        std::unique_ptr<Scope> body;
        
        explicit Atomic(std::unique_ptr<Scope> body);
        ACCEPT_PROGRAM_VISITOR
    };
    
    struct Sequence final : public Statement {
        std::unique_ptr<Statement> first;
        std::unique_ptr<Statement> second;
        
        explicit Sequence(std::unique_ptr<Statement> first, std::unique_ptr<Statement> second);
        ACCEPT_PROGRAM_VISITOR
    };

    struct UnconditionalLoop final : public Statement {
        std::unique_ptr<Scope> body;
        
        explicit UnconditionalLoop(std::unique_ptr<Scope> body);
        ACCEPT_PROGRAM_VISITOR
    };

    struct Choice final : public Statement {
        std::vector<std::unique_ptr<Scope>> branches;
        
        explicit Choice();
        explicit Choice(std::unique_ptr<Scope> branch1, std::unique_ptr<Scope> branch2);
        ACCEPT_PROGRAM_VISITOR
    };
    
    //
    // Commands
    //
    
    struct Command : public Statement {
    };

    struct Skip final : public Command {
        explicit Skip();
        ACCEPT_PROGRAM_VISITOR
    };

    struct Fail final : public Command {
        explicit Fail();
        ACCEPT_PROGRAM_VISITOR
    };

    struct Break final : public Command {
        explicit Break();
        ACCEPT_PROGRAM_VISITOR
    };

    struct Return final : public Command {
        std::vector<std::unique_ptr<SimpleExpression>> expressions;
        
        explicit Return();
        explicit Return(std::unique_ptr<SimpleExpression> expression);
        ACCEPT_PROGRAM_VISITOR
    };

    struct Assume final : public Command {
        std::unique_ptr<BinaryExpression> condition;
        
        explicit Assume(std::unique_ptr<BinaryExpression> condition);
        ACCEPT_PROGRAM_VISITOR
    };
    
    struct Malloc final : public Command {
        std::unique_ptr<VariableExpression> lhs;
        
        explicit Malloc(std::unique_ptr<VariableExpression> lhs);
        ACCEPT_PROGRAM_VISITOR
    };

    struct Macro final : public Command {
        // lhs must not occur twice
        std::vector<std::unique_ptr<VariableExpression>> lhs;
        std::reference_wrapper<const Function> function;
        std::vector<std::unique_ptr<SimpleExpression>> arguments;

        explicit Macro(const Function& function);
        [[nodiscard]] const Function& Func() const;
        ACCEPT_PROGRAM_VISITOR
    };

    struct AcquireLock final : public Command {
        std::unique_ptr<Dereference> lock;

        explicit AcquireLock(std::unique_ptr<Dereference> lock);
        ACCEPT_PROGRAM_VISITOR
    };

    struct ReleaseLock final : public Command {
        std::unique_ptr<Dereference> lock;

        explicit ReleaseLock(std::unique_ptr<Dereference> lock);
        ACCEPT_PROGRAM_VISITOR
    };
    
    template<typename L, typename R>
    struct Assignment : public Command {
        // lhs must not occur twice
        std::vector<std::unique_ptr<L>> lhs;
        std::vector<std::unique_ptr<R>> rhs;
        
        explicit Assignment();
        explicit Assignment(std::unique_ptr<L> lhs, std::unique_ptr<R> rhs);
    };
    
    struct VariableAssignment final : public Assignment<VariableExpression, ValueExpression> {
        using Assignment::Assignment;
        ACCEPT_PROGRAM_VISITOR
    };
    
    struct MemoryWrite final : public Assignment<Dereference, SimpleExpression> {
        using Assignment::Assignment;
        ACCEPT_PROGRAM_VISITOR
    };
    
    
    //
    // Output
    //
    
    std::ostream& operator<<(std::ostream& stream, const Type& object);
    std::ostream& operator<<(std::ostream& stream, const Sort& object);
    std::ostream& operator<<(std::ostream& stream, const AstNode& object);
    std::ostream& operator<<(std::ostream& stream, const VariableDeclaration& object);
    std::ostream& operator<<(std::ostream& stream, const BinaryOperator& object);

} // namespace plankton

#endif //PLANKTON_PROGRAMS_AST_HPP
