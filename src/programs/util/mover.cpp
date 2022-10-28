#include "programs/util.hpp"

using namespace plankton;


struct RightMoveVisitor : public BaseProgramVisitor {
    bool isRightMover = true;
    void Visit(const TrueValue& /*node*/) override { /* do nothing */ }
    void Visit(const FalseValue& /*node*/) override { /* do nothing */ }
    void Visit(const NullValue& /*node*/) override { /* do nothing */ }
    void Visit(const MaxValue& /*node*/) override { /* do nothing */ }
    void Visit(const MinValue& /*node*/) override { /* do nothing */ }
    void Visit(const VariableExpression& node) override { isRightMover &= !node.Decl().isShared; }
    void Visit(const BinaryExpression& node) override { node.lhs->Accept(*this); node.rhs->Accept(*this); }
    void Visit(const Dereference& /*node*/) override { isRightMover = false; }
    void Visit(const Sequence& node) override { node.first->Accept(*this); node.second->Accept(*this); }
    void Visit(const Scope& node) override { node.body->Accept(*this); }
    void Visit(const Atomic& node) override { node.body->Accept(*this); }
    void Visit(const Choice& node) override { for (const auto& branch : node.branches) branch->Accept(*this); }
    void Visit(const UnconditionalLoop& node) override { node.body->Accept(*this); }
    void Visit(const Skip& /*node*/) override { /* do nothing */ }
    void Visit(const Fail& /*node*/) override { /* do nothing */ }
    void Visit(const Break& /*node*/) override { /* do nothing */ }
    void Visit(const Assume& node) override { node.condition->Accept(*this); }
    void Visit(const AssertFlow& /*node*/) override  { /* do nothing */ }
    void Visit(const AssumeFlow& /*node*/) override  { /* do nothing */ }
    void Visit(const UpdateStub& /*node*/) override  { /* do nothing */ }
    void Visit(const Suggestion& /*node*/) override  { /* do nothing */ }
    void Visit(const Return& node) override { for (const auto& expr : node.expressions) expr->Accept(*this); }
    void Visit(const Malloc& node) override { node.lhs->Accept(*this); }
    void Visit(const Macro& /*node*/) override { isRightMover = false; }
    template<typename T>
    void HandleAssignment(const T& node) {
        for (const auto& elem : node.lhs) elem->Accept(*this);
        for (const auto& elem : node.rhs) elem->Accept(*this);
    }
    void Visit(const VariableAssignment& node) override { HandleAssignment(node); }
    void Visit(const MemoryWrite& node) override { HandleAssignment(node); }
    void Visit(const AcquireLock& /*node*/) override { isRightMover = true; }
    void Visit(const ReleaseLock& /*node*/) override { isRightMover = false; }
    void Visit(const Function& /*node*/) override { isRightMover = false; }
};

bool plankton::IsRightMover(const Statement& stmt) {
    RightMoveVisitor visitor;
    stmt.Accept(visitor);
    return visitor.isRightMover;
}
