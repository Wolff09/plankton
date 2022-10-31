#include "engine/effect.hpp"

#include <utility>
#include "engine/solver.hpp"
#include "programs/util.hpp"

using namespace plankton;


SymbolicHeapEquality::SymbolicHeapEquality(BinaryOperator op, const SymbolDeclaration& lhsSymbol_, std::string lhsField_, std::unique_ptr<SymbolicExpression> rhs_)
        : op(op), lhsSymbol(std::make_unique<SymbolicVariable>(lhsSymbol_)), lhsFieldName(std::move(lhsField_)), rhs(std::move(rhs_)) {
    assert(lhsSymbol_.order == Order::FIRST);
    assert(lhsSymbol_.type.sort == Sort::PTR);
    assert(lhsSymbol_.type.GetField(lhsFieldName).has_value());
    assert(rhs);
    assert(rhs->GetType().Comparable(lhsSymbol_.type.GetField(lhsFieldName).value()));
    assert(rhs->GetOrder() == Order::FIRST);
}

SymbolicHeapFlow::SymbolicHeapFlow(const SymbolDeclaration& sym, bool empty) : symbol(std::make_unique<SymbolicVariable>(sym)), isEmpty(empty) {
    assert(sym.order == Order::FIRST);
    assert(sym.type.sort == Sort::PTR);
}

HeapEffect::HeapEffect(std::unique_ptr<SharedMemoryCore> before, std::unique_ptr<SharedMemoryCore> after, std::unique_ptr<Formula> ctx)
        : pre(std::move(before)), post(std::move(after)), context(std::move(ctx)) {
    assert(pre);
    assert(post);
    assert(pre->node->Decl() == post->node->Decl());
    assert(context);
}

HeapEffect::HeapEffect(std::unique_ptr<SharedMemoryCore> before, std::unique_ptr<SharedMemoryCore> after, std::unique_ptr<Formula> ctx,
                       std::vector<std::unique_ptr<SymbolicHeapExpression>> preHaloCtx, std::vector<std::unique_ptr<SymbolicHeapExpression>> postHaloCtx)
        : HeapEffect(std::move(before), std::move(after), std::move(ctx)) {
    preHalo = std::move(preHaloCtx);
    preHalo = std::move(postHaloCtx);
    assert(plankton::AllNonNull(preHalo));
    assert(plankton::AllNonNull(postHalo));
}

HeapEffect::HeapEffect(std::unique_ptr<SharedMemoryCore> before, std::unique_ptr<SharedMemoryCore> after, std::unique_ptr<Formula> ctx,
                       std::deque<std::unique_ptr<SymbolicHeapExpression>> preHaloCtx, std::deque<std::unique_ptr<SymbolicHeapExpression>> postHaloCtx)
        : HeapEffect(std::move(before), std::move(after), std::move(ctx)) {
    plankton::MoveInto(std::move(preHaloCtx), preHalo);
    plankton::MoveInto(std::move(postHaloCtx), postHalo);
    assert(plankton::AllNonNull(preHalo));
    assert(plankton::AllNonNull(postHalo));
}

std::unique_ptr<SymbolicHeapExpression> plankton::Copy(const SymbolicHeapExpression& obj) {
    struct : public EffectVisitor {
        std::unique_ptr<SymbolicHeapExpression> result;
        void Visit(const SymbolicHeapEquality& obj) override { result = Copy(obj); }
        void Visit(const SymbolicHeapFlow& obj) override { result = Copy(obj); }
    } visitor;
    obj.Accept(visitor);
    assert(visitor.result);
    return std::move(visitor.result);
}

std::unique_ptr<SymbolicHeapEquality> plankton::Copy(const SymbolicHeapEquality& obj) {
    return std::make_unique<SymbolicHeapEquality>(obj.op, obj.lhsSymbol->Decl(), obj.lhsFieldName, plankton::Copy(*obj.rhs));
}

std::unique_ptr<SymbolicHeapFlow> plankton::Copy(const SymbolicHeapFlow& obj) {
    return std::make_unique<SymbolicHeapFlow>(obj.symbol->Decl(), obj.isEmpty);
}

struct SymbolicDereferencePrinter : public EffectVisitor {
    std::ostream& out;
    explicit SymbolicDereferencePrinter(std::ostream& out) : out(out) {}
    void Visit(const SymbolicHeapEquality& obj) override { out << obj; }
    void Visit(const SymbolicHeapFlow& obj) override { out << obj; }
};

std::ostream& plankton::operator<<(std::ostream& out, const SymbolicHeapExpression& object) {
    SymbolicDereferencePrinter printer(out);
    object.Accept(printer);
    return out;
}

std::ostream& plankton::operator<<(std::ostream& out, const SymbolicHeapEquality& object) {
    out << *object.lhsSymbol << "->" << object.lhsFieldName << " " << object.op << " " << *object.rhs;
    return out;
}

std::ostream& plankton::operator<<(std::ostream& out, const SymbolicHeapFlow& object) {
    out << *object.symbol << "->_flow " << (object.isEmpty ? "==" : "!=") << " ∅";
    return out;
}

std::ostream& plankton::operator<<(std::ostream& out, const HeapEffect& object) {
    out << "[ " << *object.pre << " ~~> " << *object.post << " | " << *object.context << " | ";
    bool first = true;
    if (object.preHalo.empty()) out << "true";
    for (const auto& elem : object.preHalo) {
        if (!first) out << "  &&  ";
        out << *elem;
        first = false;
    }
    out << " | ";
    first = true;
    if (object.postHalo.empty()) out << "true";
    for (const auto& elem : object.postHalo) {
        if (!first) out << "  &&  ";
        out << *elem;
        first = false;
    }
    out << " ]";
    return out;
}
