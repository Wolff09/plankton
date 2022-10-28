#pragma once
#ifndef PLANKTON_PARSER_BUILDER_HPP
#define PLANKTON_PARSER_BUILDER_HPP

#include <memory>
#include "PlanktonParser.h"
#include "parser/parse.hpp"
#include "programs/ast.hpp"
#include "logics/ast.hpp"
#include "engine/config.hpp"

namespace plankton {
    
    struct AstBuilder {
        static ParsingResult BuildFrom(std::istream& input, bool spuriousCasFails); // may return NULL config
        
        // declarations (lookup / creation)
        [[nodiscard]] const Type& TypeByName(const std::string& name) const;
        [[nodiscard]] const Type* TypeByNameOrNull(const std::string& name) const;
        [[nodiscard]] const VariableDeclaration& VariableByName(const std::string& name) const;
        [[nodiscard]] const VariableDeclaration* VariableByNameOrNull(const std::string& name) const;
        [[nodiscard]] const Function& FunctionByName(const std::string& name) const;
        [[nodiscard]] const Function* FunctionByNameOrNull(const std::string& name) const;
        void AddDecl(std::unique_ptr<Type> type);
        void AddDecl(std::unique_ptr<VariableDeclaration> variable);
        void AddDecl(std::unique_ptr<Function> function);
        void AddDecl(PlanktonParser::VarDeclContext& context, bool shared);
        void AddDecl(PlanktonParser::VarDeclListContext& context, bool shared);
        void PushScope();
        std::vector<std::unique_ptr<VariableDeclaration>> PopScope();
        
        // program
        void PrepareMake(PlanktonParser::ProgramContext& context);
        std::unique_ptr<Program> MakeProgram(PlanktonParser::ProgramContext& context);
        std::unique_ptr<Function> MakeFunction(PlanktonParser::FunctionContext& context);
        std::string MakeBaseTypeName(PlanktonParser::TypeContext& context) const;
        
        // config
        std::unique_ptr<ParsedSolverConfig> MakeConfig(PlanktonParser::ProgramContext& context);
        std::unique_ptr<ImplicationSet> MakeInvariant(PlanktonParser::InvariantContext& context, const Formula& eval);
        
        // expressions
        std::unique_ptr<ValueExpression> MakeExpression(PlanktonParser::ExpressionContext& context);
        std::unique_ptr<Dereference> MakeExpression(PlanktonParser::DerefContext& context);
        std::unique_ptr<SimpleExpression> MakeExpression(PlanktonParser::ValueContext& context);
        std::unique_ptr<SimpleExpression> MakeExpression(PlanktonParser::SimpleExpressionContext& context);
        std::pair<std::unique_ptr<ValueExpression>, bool> MakeExpression(PlanktonParser::SimpleConditionContext& context);
        std::unique_ptr<BinaryExpression> MakeExpression(PlanktonParser::BinaryConditionContext& context);
        
        // statements / commands
        std::unique_ptr<Scope> MakeScope(PlanktonParser::ScopeContext& context);
        std::unique_ptr<Scope> MakeScope(PlanktonParser::BlockContext& context);
        std::unique_ptr<Statement> MakeStatement(PlanktonParser::StatementContext& context);
        std::unique_ptr<Statement> MakeCommand(PlanktonParser::CommandContext& context);
        std::unique_ptr<Statement> MakeChoice(PlanktonParser::StmtChooseContext& context);
        std::unique_ptr<Statement> MakeChoice(PlanktonParser::StmtIfContext& context);
        std::unique_ptr<UnconditionalLoop> MakeLoop(PlanktonParser::StmtLoopContext& context);
        std::unique_ptr<UnconditionalLoop> MakeLoop(PlanktonParser::StmtWhileContext& context);
        std::unique_ptr<UnconditionalLoop> MakeLoop(PlanktonParser::StmtDoContext& context);
        std::unique_ptr<Atomic> MakeAtomic(PlanktonParser::StmtAtomicContext& context);
        std::unique_ptr<Statement> MakeAssignment(PlanktonParser::CmdAssignStackContext& context);
        std::unique_ptr<Statement> MakeAssignment(PlanktonParser::CmdAssignHeapContext& context);
        std::unique_ptr<Statement> MakeAssignment(PlanktonParser::CmdAssignConditionContext& context);
        std::unique_ptr<Statement> MakeMalloc(PlanktonParser::CmdMallocContext& context);
        std::unique_ptr<Statement> MakeAssume(PlanktonParser::LogicConditionContext& context);
        std::unique_ptr<Statement> MakeAssert(PlanktonParser::LogicConditionContext& context);
        std::unique_ptr<Statement> MakeAssertFlow(PlanktonParser::CmdAssertFlowContext& context);
        std::unique_ptr<Statement> MakeStub(PlanktonParser::CmdStubContext& context);
        std::unique_ptr<Statement> MakeJoin(PlanktonParser::CmdJoinContext& context);
        std::unique_ptr<Statement> MakeAssumeFlow(PlanktonParser::CmdAssumeFlowContext& context);
        std::unique_ptr<Statement> MakeCall(PlanktonParser::CmdCallContext& context);
        std::unique_ptr<Statement> MakeReturn(PlanktonParser::CmdReturnListContext& context);
        std::unique_ptr<Statement> MakeReturn(PlanktonParser::CmdReturnExprContext& context);
        std::unique_ptr<Statement> MakeCas(PlanktonParser::CmdCasContext& context);
        std::unique_ptr<Statement> MakeLock(PlanktonParser::CmdLockContext& context);
        std::unique_ptr<Statement> MakeUnlock(PlanktonParser::CmdUnlockContext& context);

        // parsing configuration
        [[nodiscard]] bool SpuriousCasFail() const;

        private:
            bool prepared = false;
            bool spuriousCasFails;
            std::deque<std::unique_ptr<Type>> _types;
            std::deque<std::unique_ptr<Function>> _functions;
            std::deque<std::deque<std::unique_ptr<VariableDeclaration>>> _variables;
            
            explicit AstBuilder(bool spuriousCasFails);
    };
    
}

#endif //PLANKTON_PARSER_BUILDER_HPP
