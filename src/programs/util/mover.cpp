#include "programs/util.hpp"

using namespace plankton;


template<bool ACQ_MOVE, bool REL_MOVE>
struct MoveVisitor : public BaseProgramVisitor {
    bool isMover = true;
    void Visit(const TrueValue& /*node*/) override { /* do nothing */ }
    void Visit(const FalseValue& /*node*/) override { /* do nothing */ }
    void Visit(const NullValue& /*node*/) override { /* do nothing */ }
    void Visit(const MaxValue& /*node*/) override { /* do nothing */ }
    void Visit(const MinValue& /*node*/) override { /* do nothing */ }
    void Visit(const VariableExpression& node) override { isMover &= !node.Decl().isShared; }
    void Visit(const BinaryExpression& node) override { node.lhs->Accept(*this); node.rhs->Accept(*this); }
    void Visit(const Dereference& /*node*/) override { isMover = false; }
    void Visit(const Sequence& node) override { for (const auto& elem : node.statements) elem->Accept(*this); }
    void Visit(const Scope& node) override { node.body->Accept(*this); }
    void Visit(const Atomic& node) override { node.body->Accept(*this); }
    void Visit(const Choice& node) override { for (const auto& branch : node.branches) branch->Accept(*this); }
    void Visit(const NondeterministicLoop& node) override { node.body->Accept(*this); }
    void Visit(const Skip& /*node*/) override { /* do nothing */ }
    void Visit(const Fail& /*node*/) override { /* do nothing */ }
    void Visit(const Break& /*node*/) override { /* do nothing */ }
    void Visit(const Continue& /*node*/) override { /* do nothing */ }
    void Visit(const Assume& node) override { node.condition->Accept(*this); }
    void Visit(const Return& node) override { for (const auto& expr : node.expressions) expr->Accept(*this); }
    void Visit(const Malloc& node) override { node.lhs->Accept(*this); }
    void Visit(const Macro& /*node*/) override { isMover = false; }
    template<typename T>
    void HandleAssignment(const T& node) {
        for (const auto& elem : node.lhs) elem->Accept(*this);
        for (const auto& elem : node.rhs) elem->Accept(*this);
    }
    void Visit(const VariableAssignment& node) override { HandleAssignment(node); }
    void Visit(const MemoryWrite& node) override { HandleAssignment(node); }
    void Visit(const AcquireLock& /*node*/) override { isMover &= ACQ_MOVE; }
    void Visit(const ReleaseLock& /*node*/) override { isMover &= REL_MOVE; }
    void Visit(const Function& /*node*/) override { isMover = false; }

    void Visit(const AndExpression& object) override { for (const auto& elem : object.expressions) elem->Accept(*this); }
    void Visit(const OrExpression& object) override { for (const auto& elem : object.expressions) elem->Accept(*this); }
    void Visit(const IfThenElse& object) override { object.desugared->Accept(*this); }
    void Visit(const IfElifElse& object) override { object.desugared->Accept(*this); }
    void Visit(const WhileLoop& object) override { object.desugared->Accept(*this); }
    void Visit(const DoWhileLoop& object) override { object.desugared->Accept(*this); }
    void Visit(const ComplexAssume& object) override { object.desugared->Accept(*this); }
    void Visit(const ComplexAssert& object) override { object.desugared->Accept(*this); }
    void Visit(const CompareAndSwap& object) override { object.desugared->Accept(*this); }
};

template<bool ACQ_MOVE, bool REL_MOVE>
inline bool IsMover(const MovableAstNode& stmt) {
    MoveVisitor<ACQ_MOVE, REL_MOVE> visitor;
    stmt.Accept(visitor);
    return visitor.isMover;
}

inline bool IsRightMover(const MovableAstNode& stmt) {
    return IsMover<true, false>(stmt);
}

inline bool IsLeftMover(const MovableAstNode& stmt) {
    return IsMover<false, true>(stmt);
}

MoverType plankton::ComputeMoverType(const MovableAstNode& statement) {
    auto left = IsLeftMover(statement);
    auto right = IsRightMover(statement);
    if (left && right) return MoverType::BOTH;
    if (right) return MoverType::RIGHT;
    if (left) return MoverType::LEFT;
    return MoverType::NONE;
}
