#include "programs/ast.hpp"

#include <deque>
#include "programs/util.hpp"

using namespace plankton;
using CheckedWrite = CompareAndSwap::CheckedWrite;
using ConditionalScope = IfElifElse::ConditionalScope;


template<typename T>
SyntacticSugar<T>::SyntacticSugar(std::unique_ptr<T> equiv) : desugared(std::move(equiv)) {
    assert(desugared);
}

inline std::unique_ptr<ComplexExpression> NegateComplexExpression(const ComplexExpression& expr) {
    auto copy = CopyAll(expr.expressions);
    std::unique_ptr<ComplexExpression> result(nullptr);
    if (auto andExpr = dynamic_cast<const AndExpression*>(&expr)) result = std::make_unique<OrExpression>(std::move(copy));
    else if (auto orExpr = dynamic_cast<const OrExpression*>(&expr)) result = std::make_unique<AndExpression>(std::move(copy));
    else throw std::logic_error("Failed to desugar: unexpected expression '" + plankton::ToString(expr) + "'."); // TODO: better error handling
    assert(result);
    for (auto& elem : result->expressions) elem->op = plankton::Negate(elem->op);
    return result;
}

inline std::unique_ptr<Scope> MkSkipScope() {
    return std::make_unique<Scope>(std::make_unique<Skip>());
}


//
// Expressions
//

ComplexExpression::ComplexExpression(std::unique_ptr<BinaryExpression> expression) : SyntacticSugar(nullptr) {
    assert(expression);
    expressions.push_back(std::move(expression));
}

ComplexExpression::ComplexExpression(std::unique_ptr<BinaryExpression> expression, std::unique_ptr<BinaryExpression> other) : SyntacticSugar(nullptr) {
    assert(expression);
    assert(other);
    expressions.push_back(std::move(expression));
    expressions.push_back(std::move(other));
}

ComplexExpression::ComplexExpression(std::vector<std::unique_ptr<BinaryExpression>> expressions) : SyntacticSugar(nullptr) {
    assert(AllNonNull(expressions));
    MoveInto(std::move(expressions), expressions);
}


//
// Expressions desugaring
//

struct CaseAnalysis {
    using Conjunction = std::vector<std::unique_ptr<BinaryExpression>>;
    using Disjunction = std::vector<Conjunction>;
    Disjunction trueCase; // disjunction of conjunctions
    Disjunction falseCase; // disjunction of conjunctions
};

inline CaseAnalysis::Disjunction MakeParts(const ComplexExpression& expr) {
    CaseAnalysis::Disjunction result;
    for (std::size_t index = 0; index < expr.expressions.size(); ++index) {
        result.emplace_back();
        for (std::size_t curr = 0; curr <= index; ++curr) {
            result.back().push_back(plankton::Copy(*expr.expressions.at(curr)));
        }
    }
    return result;
}

inline CaseAnalysis MakeCaseAnalysis(const AndExpression& expr) {
    // a & b & c  ==>  {true} a & b & c --- {false} !a, a & !b, a & b & !c
    CaseAnalysis result;
    result.trueCase.push_back(CopyAll(expr.expressions));
    result.falseCase = MakeParts(expr);
    for (auto& elem : result.falseCase) {
        elem.back()->op = plankton::Negate(elem.back()->op);
    }
    return result;
}

inline CaseAnalysis MakeCaseAnalysis(const OrExpression& expr) {
    // a | b | c  ==>  {true} a, !a & b, !a & !b & c --- {false} !a & !b & !c
    CaseAnalysis result;
    result.trueCase = MakeParts(expr);
    for (auto& elem : result.trueCase) {
        assert(!elem.empty());
        for (auto it = std::next(elem.rbegin()); it != elem.rend(); ++it) {
            (**it).op = plankton::Negate((**it).op);
        }
    }
    result.falseCase.push_back(CopyAll(expr.expressions));
    for (auto& elem : result.falseCase.front()) {
        elem->op = plankton::Negate(elem->op);
    }
    return result;
}

inline CaseAnalysis MakeCaseAnalysis(const ComplexExpression& expr) {
    if (auto andExpr = dynamic_cast<const AndExpression*>(&expr)) return MakeCaseAnalysis(*andExpr);
    if (auto orExpr = dynamic_cast<const OrExpression*>(&expr)) return MakeCaseAnalysis(*orExpr);
    throw std::logic_error("Failed to desugar: unexpected expression '" + plankton::ToString(expr) + "'."); // TODO: better error handling
}

inline std::unique_ptr<Statement> DesugarConjunction(const CaseAnalysis::Conjunction& conjunction) {
    auto result = MakeVector<std::unique_ptr<Statement>>(conjunction.size());
    for (const auto& elem : conjunction) {
        result.push_back(std::make_unique<Assume>(plankton::Copy(*elem)));
    }
    if (result.empty()) return nullptr;
    if (result.size() == 1) return std::move(result.front());
    return std::make_unique<Sequence>(std::move(result));
}

inline std::unique_ptr<Statement> DesugarDisjunction(const CaseAnalysis::Disjunction& disjunction) {
    auto result = std::make_unique<Choice>();
    for (const auto& elem : disjunction) {
        if (auto cond = DesugarConjunction(elem)) {
            result->branches.push_back(std::make_unique<Scope>(std::move(cond)));
        }
    }
    if (result->branches.empty()) return nullptr;
    if (result->branches.size() == 1) return std::move(result->branches.front()->body);
    return result;
}

struct DesugaredExpression {
    std::unique_ptr<Statement> trueFilter;
    std::unique_ptr<Statement> falseFilter;
};

inline DesugaredExpression DesugarComplexExpression(const ComplexExpression& expr) {
    auto desugared = MakeCaseAnalysis(expr);
    DesugaredExpression result;
    result.trueFilter = DesugarDisjunction(desugared.trueCase);
    result.falseFilter = DesugarDisjunction(desugared.falseCase);
    return result;
}


//
// Statements
//

inline std::unique_ptr<Scope> MakeBranch(const Scope& blueprint, std::unique_ptr<ComplexExpression> condition, std::vector<std::unique_ptr<ComplexExpression>> prependConditions = {}) {
    auto result = plankton::Copy(blueprint);
    std::vector<std::unique_ptr<Statement>> sequence;
    for (auto&& elem : prependConditions) {
        sequence.push_back(std::make_unique<ComplexAssume>(std::move(elem)));
    }
    if (condition) {
        sequence.push_back(std::make_unique<ComplexAssume>(std::move(condition)));
    }
    sequence.push_back(std::move(result->body));
    result->body = std::make_unique<Sequence>(std::move(sequence));
    return result;
}

inline std::unique_ptr<Choice> DesugarIfThenElse(const ComplexExpression& condition, const Scope& ifCode, const Scope& elseCode) {
    auto trueBranch = MakeBranch(ifCode, plankton::Copy(condition));
    auto falseBranch = MakeBranch(elseCode, NegateComplexExpression(condition));
    return std::make_unique<Choice>(std::move(trueBranch), std::move(falseBranch));
}

IfThenElse::IfThenElse(std::unique_ptr<ComplexExpression> expression, std::unique_ptr<Scope> ifBranch_, std::unique_ptr<Scope> elseBranch_)
        : SyntacticSugarStatement(DesugarIfThenElse(*expression, *ifBranch_, *elseBranch_)),
          condition(std::move(expression)), ifBranch(std::move(ifBranch_)), elseBranch(std::move(elseBranch_)) {}

IfThenElse::IfThenElse(std::unique_ptr<ComplexExpression> expression, std::unique_ptr<Scope> ifBranch)
        : IfThenElse(std::move(expression), std::move(ifBranch), MkSkipScope()) {}

IfThenElse::IfThenElse(std::unique_ptr<BinaryExpression> expression, std::unique_ptr<Scope> ifBranch, std::unique_ptr<Scope> elseBranch)
        : IfThenElse(std::make_unique<AndExpression>(std::move(expression)), std::move(ifBranch), std::move(elseBranch)) {}

IfThenElse::IfThenElse(std::unique_ptr<BinaryExpression> expression, std::unique_ptr<Scope> ifBranch)
        : IfThenElse(std::make_unique<OrExpression>(std::move(expression)), std::move(ifBranch)) {}

inline std::unique_ptr<Choice> DesugarIfElifElse(const std::vector<ConditionalScope>& conditionalBranches, const Scope& elseBranch) {
    auto result = std::make_unique<Choice>();
    std::vector<std::unique_ptr<ComplexExpression>> additional;
    for (const auto& [cond, scope] : conditionalBranches) {
        result->branches.push_back(MakeBranch(*scope, plankton::Copy(*cond), CopyAll(additional)));
        additional.push_back(NegateComplexExpression(*cond));
    }
    result->branches.push_back(MakeBranch(elseBranch, nullptr, std::move(additional)));
    return result;
}

IfElifElse::IfElifElse(std::vector<ConditionalScope> conditionalBranches_, std::unique_ptr<Scope> elseBranch_)
        : SyntacticSugarStatement(DesugarIfElifElse(conditionalBranches_, *elseBranch_)),
          conditionalBranches(std::move(conditionalBranches_)), elseBranch(std::move(elseBranch_)) {}

IfElifElse::IfElifElse(std::vector<ConditionalScope> conditionalBranches) : IfElifElse(std::move(conditionalBranches), MkSkipScope()) {}

//
// Loop
//

inline std::unique_ptr<Statement> DesugarWhileLoop(const ComplexExpression& condition, const Scope& bodyCode) {
    // while (cond) { vars; stmts; }  ==>  while (*) { vars; assume(cond); stmts; } assume(!cond);
    auto enter = std::make_unique<ComplexAssume>(plankton::Copy(condition));
    auto exit = std::make_unique<ComplexAssume>(NegateComplexExpression(condition));
    auto body = plankton::Copy(bodyCode);
    body->body = std::make_unique<Sequence>(std::move(enter), std::move(body->body));
    auto loop = std::make_unique<NondeterministicLoop>(std::move(body));
    return std::make_unique<Sequence>(std::move(loop), std::move(exit));
}

WhileLoop::WhileLoop(std::unique_ptr<ComplexExpression> condition_, std::unique_ptr<Scope> body_)
        : SyntacticSugarStatement(DesugarWhileLoop(*condition_, *body_)), condition(std::move(condition_)), body(std::move(body_)) {}

WhileLoop::WhileLoop(std::unique_ptr<BinaryExpression> condition, std::unique_ptr<Scope> body)
        : WhileLoop(std::make_unique<AndExpression>(std::move(condition)), std::move(body)) {}

inline std::unique_ptr<Statement> DesugarDoWhileLoop(const ComplexExpression& condition, const Scope& bodyCode) {
    // do { vars; stmts; } while (cond);  ==>  { vars; stmts; while (*) { assume(cond); stmts; } } assume(!cond);
    auto enter = std::make_unique<ComplexAssume>(plankton::Copy(condition));
    auto exit = std::make_unique<ComplexAssume>(NegateComplexExpression(condition));
    auto scope = plankton::Copy(bodyCode);
    auto loopBody = std::make_unique<Sequence>(std::move(enter), plankton::Copy(*scope->body));
    auto loop = std::make_unique<NondeterministicLoop>(std::make_unique<Scope>(std::move(loopBody)));
    scope->body = std::make_unique<Sequence>(std::move(scope->body), std::move(loop));
    return std::make_unique<Sequence>(std::move(scope), std::move(exit));
}

DoWhileLoop::DoWhileLoop(std::unique_ptr<ComplexExpression> condition_, std::unique_ptr<Scope> body_)
        : SyntacticSugarStatement(DesugarDoWhileLoop(*condition_, *body_)), condition(std::move(condition_)), body(std::move(body_)) {}

DoWhileLoop::DoWhileLoop(std::unique_ptr<BinaryExpression> condition, std::unique_ptr<Scope> body)
        : DoWhileLoop(std::make_unique<AndExpression>(std::move(condition)), std::move(body)) {}


//
// Assume, assert
//

inline std::unique_ptr<Statement> DesugarCheck(const ComplexExpression& condition, bool failOnFalse) {
    auto desugar = DesugarComplexExpression(condition);
    auto branch1 = std::make_unique<Scope>(std::move(desugar.trueFilter));
    auto branch2 = std::make_unique<Scope>(std::move(desugar.falseFilter));
    if (failOnFalse)
        branch2->body = std::make_unique<Sequence>(std::move(branch2->body), std::make_unique<Fail>());
    return std::make_unique<Choice>(std::move(branch1), std::move(branch2));
}

ComplexAssume::ComplexAssume(std::unique_ptr<ComplexExpression> cond) : SyntacticSugarCommand(DesugarCheck(*cond, false)), condition(std::move(cond)) {
    assert(condition);
}

ComplexAssert::ComplexAssert(std::unique_ptr<ComplexExpression> cond) : SyntacticSugarCommand(DesugarCheck(*cond, true)), condition(std::move(cond)) {
    assert(condition);
}

ComplexAssert::ComplexAssert(std::unique_ptr<BinaryExpression> condition) : ComplexAssert(std::make_unique<AndExpression>(std::move(condition))) {}


//
// CAS
//

inline std::unique_ptr<BinaryExpression> MakeEq(const ValueExpression& lhs, const ValueExpression& rhs) {
    return std::make_unique<BinaryExpression>(BinaryOperator::EQ, plankton::Copy(lhs), plankton::Copy(rhs));
}

inline std::pair<std::unique_ptr<AndExpression>, std::unique_ptr<MemoryWrite>> MakeCasComponents(const std::vector<CheckedWrite>& cas) {
    std::vector<std::unique_ptr<BinaryExpression>> checks;
    auto write = std::make_unique<MemoryWrite>();
    for (const auto& elem : cas) {
        assert(elem.lhs);
        assert(elem.chk);
        assert(elem.rhs);
        checks.push_back(MakeEq(*elem.lhs, *elem.chk));
        write->lhs.push_back(plankton::Copy(*elem.lhs));
    }
    return std::make_pair(std::make_unique<AndExpression>(std::move(checks)), std::move(write));
}

inline std::unique_ptr<Statement> DesugarCas(const std::vector<CheckedWrite>& cas, bool spuriousFailure) {
    // CAS(x, y, z)  ==>  {spurious} if (*) { assume(x == y); x = z; }  ---  {!spurious} if (*) { assume(x == y); x = z; } else { assume(x != y); }
    auto [condition, write] = MakeCasComponents(cas);
    std::unique_ptr<Statement> operation(nullptr);
    if (spuriousFailure) {
        auto assume = std::make_unique<ComplexAssume>(std::move(condition));
        auto success = std::make_unique<Sequence>(std::move(assume), std::move(write));
        operation = std::make_unique<Choice>(std::make_unique<Scope>(std::move(success)), MkSkipScope());
    } else {
        operation = std::make_unique<IfThenElse>(std::move(condition), std::make_unique<Scope>(std::move(write)));
    }
    assert(operation);
    return std::make_unique<Atomic>(std::make_unique<Scope>(std::move(operation)));
}

CompareAndSwap::CompareAndSwap(std::vector<CheckedWrite> ops, bool allowSpuriousFailure)
        : SyntacticSugarCommand(DesugarCas(ops, allowSpuriousFailure)), cas(std::move(ops)) {}

inline std::vector<CheckedWrite> MakeCheckedWriteVector(CheckedWrite&& write) {
    std::vector<CheckedWrite> result;
    result.push_back(std::move(write));
    return result;
}

CompareAndSwap::CompareAndSwap(CheckedWrite cas, bool allowSpuriousFailure) : CompareAndSwap(MakeCheckedWriteVector(std::move(cas)), allowSpuriousFailure) {}
