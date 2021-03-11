#pragma once

#include <memory>
#include <deque>
#include <unordered_map>
#include "antlr4-runtime.h"
#include "CoLaVisitor.h"
#include "cola/ast.hpp"


namespace cola {

	class AstBuilder : public cola::CoLaVisitor { // TODO: should this be a private subclass to avoid misuse?
		private:
			const std::string INIT_NAME = "__init__";
			enum struct Modifier { NONE, INLINE };
			enum struct ExprForm { NOPTR, PTR, DEREF, NESTED };

			using TypeMap = std::unordered_map<std::string, std::reference_wrapper<const Type>>;
			using VariableMap = std::unordered_map<std::string, std::unique_ptr<VariableDeclaration>>;
			using FunctionMap = std::unordered_map<std::string, Function&>;
			using ArgDeclList = std::vector<std::pair<std::string, std::string>>;
			using TypeList = std::vector<std::reference_wrapper<const Type>>;
			std::shared_ptr<Program> _program = nullptr;
			std::deque<VariableMap> _scope;
			TypeMap _types;
			FunctionMap _functions;
			bool _found_init = false;
			bool _inside_loop = false;
			const Function* _currentFunction;

			void pushScope();
			std::vector<std::unique_ptr<VariableDeclaration>> popScope();
			void addVariable(std::unique_ptr<VariableDeclaration> variable);
			bool isVariableDeclared(std::string variableName);
			const VariableDeclaration& lookupVariable(std::string variableName);
			void addFunction(Function& function);
			bool isTypeDeclared(std::string typeName);
			const Type& lookupType(std::string typeName);
			std::unique_ptr<Statement> mk_stmt_from_list(std::vector<cola::CoLaParser::StatementContext*> stmts);
			std::unique_ptr<Statement> mk_stmt_from_list(std::vector<Statement*> stmts); // claims ownership of stmts
			std::vector<std::unique_ptr<Expression>> mk_expr_from_list(std::vector<cola::CoLaParser::ExpressionContext*> exprs);
			Statement* as_command(Statement* stmt);
			Expression* mk_binary_expr(CoLaParser::ExpressionContext* lhs, BinaryExpression::Operator op, CoLaParser::ExpressionContext* rhs);
			void addFunctionArgumentScope(const Function& function, const ArgDeclList& args);
			std::vector<std::unique_ptr<VariableDeclaration>> retrieveFunctionArgumentScope(const ArgDeclList& args);
			antlrcpp::Any handleFunction(Function::Kind kind, std::string name, TypeList returnTypes, ArgDeclList args, CoLaParser::ScopeContext* body);

		public:
			static std::shared_ptr<Program> buildFrom(cola::CoLaParser::ProgramContext* parseTree);
			static std::string iTh(std::size_t number);

			antlrcpp::Any visitProgram(cola::CoLaParser::ProgramContext* context) override;
			antlrcpp::Any visitStruct_decl(cola::CoLaParser::Struct_declContext* context) override;
			antlrcpp::Any visitNameVoid(cola::CoLaParser::NameVoidContext* context) override;
			antlrcpp::Any visitNameBool(cola::CoLaParser::NameBoolContext* context) override;
			antlrcpp::Any visitNameInt(cola::CoLaParser::NameIntContext* context) override;
			antlrcpp::Any visitNameData(cola::CoLaParser::NameDataContext* context) override;
			antlrcpp::Any visitNameIdentifier(cola::CoLaParser::NameIdentifierContext* context) override;
			antlrcpp::Any visitTypeValue(cola::CoLaParser::TypeValueContext* context) override;
			antlrcpp::Any visitTypePointer(cola::CoLaParser::TypePointerContext* context) override;
			antlrcpp::Any visitField_decl(cola::CoLaParser::Field_declContext* context) override;
			antlrcpp::Any visitVarDeclRoot(CoLaParser::VarDeclRootContext* context) override;
			antlrcpp::Any visitVarDeclList(CoLaParser::VarDeclListContext* context) override;
			antlrcpp::Any visitFunctionInterface(CoLaParser::FunctionInterfaceContext* context) override;
			antlrcpp::Any visitFunctionMacro(CoLaParser::FunctionMacroContext* context) override;
			antlrcpp::Any visitFunctionInit(CoLaParser::FunctionInitContext* context) override;
			antlrcpp::Any visitArgDeclList(cola::CoLaParser::ArgDeclListContext* context) override;
			antlrcpp::Any visitTypeList(CoLaParser::TypeListContext* context) override;

			antlrcpp::Any visitBlockStmt(cola::CoLaParser::BlockStmtContext* context) override;
			antlrcpp::Any visitBlockScope(cola::CoLaParser::BlockScopeContext* context) override;
			antlrcpp::Any visitScope(cola::CoLaParser::ScopeContext* context) override;
			antlrcpp::Any visitStmtIf(cola::CoLaParser::StmtIfContext* context) override;
			antlrcpp::Any visitStmtWhile(cola::CoLaParser::StmtWhileContext* context) override;
			antlrcpp::Any visitStmtDo(cola::CoLaParser::StmtDoContext* context) override;
			antlrcpp::Any visitStmtChoose(cola::CoLaParser::StmtChooseContext* context) override;
			antlrcpp::Any visitStmtLoop(cola::CoLaParser::StmtLoopContext* context) override;
			antlrcpp::Any visitStmtAtomic(cola::CoLaParser::StmtAtomicContext* context) override;
			antlrcpp::Any visitStmtCom(cola::CoLaParser::StmtComContext* context) override;

			antlrcpp::Any visitCmdSkip(cola::CoLaParser::CmdSkipContext* context) override;
			antlrcpp::Any visitCmdAssign(cola::CoLaParser::CmdAssignContext* context) override;
			antlrcpp::Any visitCmdMalloc(cola::CoLaParser::CmdMallocContext* context) override;
			antlrcpp::Any visitCmdAssume(cola::CoLaParser::CmdAssumeContext* context) override;
			antlrcpp::Any visitCmdAssert(cola::CoLaParser::CmdAssertContext* context) override;
			antlrcpp::Any visitCmdCall(cola::CoLaParser::CmdCallContext* context) override;
			antlrcpp::Any visitCmdContinue(cola::CoLaParser::CmdContinueContext* context) override;
			antlrcpp::Any visitCmdBreak(cola::CoLaParser::CmdBreakContext* context) override;
			antlrcpp::Any visitCmdReturnVoid(CoLaParser::CmdReturnVoidContext* context) override;
			antlrcpp::Any visitCmdReturnSingle(CoLaParser::CmdReturnSingleContext* context) override;
			antlrcpp::Any visitCmdReturnList(CoLaParser::CmdReturnListContext* context) override;
			antlrcpp::Any visitCmdCas(cola::CoLaParser::CmdCasContext* context) override;
			antlrcpp::Any visitArgList(cola::CoLaParser::ArgListContext* context) override;
			antlrcpp::Any visitCasSingle(CoLaParser::CasSingleContext* context) override;
			antlrcpp::Any visitCasMultiple(CoLaParser::CasMultipleContext* context) override;

			antlrcpp::Any visitValueNull(cola::CoLaParser::ValueNullContext* context) override;
			antlrcpp::Any visitValueTrue(cola::CoLaParser::ValueTrueContext* context) override;
			antlrcpp::Any visitValueFalse(cola::CoLaParser::ValueFalseContext* context) override;
			antlrcpp::Any visitValueNDet(cola::CoLaParser::ValueNDetContext* context) override;
			antlrcpp::Any visitValueEmpty(cola::CoLaParser::ValueEmptyContext* context) override;
			antlrcpp::Any visitValueMin(cola::CoLaParser::ValueMinContext* context) override;
			antlrcpp::Any visitValueMax(cola::CoLaParser::ValueMaxContext* context) override;
			antlrcpp::Any visitExprValue(cola::CoLaParser::ExprValueContext* context) override;
			antlrcpp::Any visitExprBinaryEq(cola::CoLaParser::ExprBinaryEqContext* context) override;
			antlrcpp::Any visitExprBinaryNeq(cola::CoLaParser::ExprBinaryNeqContext* context) override;
			antlrcpp::Any visitExprBinaryLt(cola::CoLaParser::ExprBinaryLtContext* context) override;
			antlrcpp::Any visitExprBinaryLte(cola::CoLaParser::ExprBinaryLteContext* context) override;
			antlrcpp::Any visitExprBinaryGt(cola::CoLaParser::ExprBinaryGtContext* context) override;
			antlrcpp::Any visitExprBinaryGte(cola::CoLaParser::ExprBinaryGteContext* context) override;
			antlrcpp::Any visitExprBinaryAnd(cola::CoLaParser::ExprBinaryAndContext* context) override;
			antlrcpp::Any visitExprBinaryOr(cola::CoLaParser::ExprBinaryOrContext* context) override;
			antlrcpp::Any visitExprIdentifier(cola::CoLaParser::ExprIdentifierContext* context) override;
			antlrcpp::Any visitExprParens(cola::CoLaParser::ExprParensContext* context) override;
			antlrcpp::Any visitExprDeref(cola::CoLaParser::ExprDerefContext* context) override;
			antlrcpp::Any visitExprCas(cola::CoLaParser::ExprCasContext* context) override;
			antlrcpp::Any visitExprNegation(cola::CoLaParser::ExprNegationContext* context) override;

			antlrcpp::Any visitOption(cola::CoLaParser::OptionContext* context) override;

			antlrcpp::Any visitDefContains(CoLaParser::DefContainsContext* context) override;
			antlrcpp::Any visitDefInvariant(CoLaParser::DefInvariantContext* context) override;
	};

} // namespace cola
