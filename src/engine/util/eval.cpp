#include "engine/util.hpp"

#include "logics/util.hpp"

using namespace plankton;


template<typename T, typename U>
const T* GetResourceOrNull(const Formula& state, const U& filter) {
    auto collect = plankton::Collect<T>(state, filter);
    if (collect.empty()) return nullptr;
    assert(collect.size() == 1); // TODO: this is wrong ~> there may be multiple SharedMemoryCores for one address
    return *collect.begin();
}

template<typename T, typename U>
const T& GetResourceOrFail(const Formula& state, const U& filter) {
    auto result = GetResourceOrNull<T>(state, filter);
    if (!result) throw std::logic_error("Internal error: cannot find resource."); // better error handling
    return *result;
}

const EqualsToAxiom* plankton::TryGetResource(const VariableDeclaration& variable, const Formula& state) {
    return GetResourceOrNull<EqualsToAxiom>(state, [&variable](auto& obj){ return obj.Variable() == variable; });
}

const EqualsToAxiom& plankton::GetResource(const VariableDeclaration& variable, const Formula& state) {
    return GetResourceOrFail<EqualsToAxiom>(state, [&variable](auto& obj){ return obj.Variable() == variable; });
}

EqualsToAxiom& plankton::GetResource(const VariableDeclaration& variable, Formula& state) {
    return const_cast<EqualsToAxiom&>(plankton::GetResource(variable, std::as_const(state)));
}

const MemoryAxiom* plankton::TryGetResource(const SymbolDeclaration& address, const Formula& state) {
    return GetResourceOrNull<MemoryAxiom>(state, [&address](auto& obj){ return obj.node->Decl() == address; });
}

const MemoryAxiom& plankton::GetResource(const SymbolDeclaration& address, const Formula& state) {
    return GetResourceOrFail<MemoryAxiom>(state, [&address](auto& obj){ return obj.node->Decl() == address; });
}

MemoryAxiom& plankton::GetResource(const SymbolDeclaration& address, Formula& state) {
    return const_cast<MemoryAxiom&>(plankton::GetResource(address, std::as_const(state)));
}

const SymbolDeclaration* plankton::TryEvaluate(const VariableExpression& variable, const Formula& state) {
    auto* resource = plankton::TryGetResource(variable.Decl(), state);
    if (!resource) return nullptr;
    return &resource->Value();
}

const SymbolDeclaration* plankton::TryEvaluate(const Dereference& dereference, const Formula& state) {
    auto* address = plankton::TryEvaluate(*dereference.variable, state);
    if (!address) return nullptr;
    auto* memory = plankton::TryGetResource(*address, state);
    if (!memory) return nullptr;
    return &memory->fieldToValue.at(dereference.fieldName)->Decl();
}

template<typename T>
const SymbolDeclaration& EvaluateOrFail(const T& expr, const Formula& state) {
    auto result = TryEvaluate(expr, state);
    if (!result) throw std::logic_error("Internal error: cannot evaluate."); // better error handling
    return *result;
}

const SymbolDeclaration& plankton::Evaluate(const VariableExpression& variable, const Formula& state) {
    return EvaluateOrFail(variable, state);
}

const SymbolDeclaration& plankton::Evaluate(const Dereference& dereference, const Formula& state) {
    return EvaluateOrFail(dereference, state);
}

// struct EvalHeapVisitor : public EffectVisitor {
//     const MemoryLookup& adrToMem;
//     std::unique_ptr<MemoryAxiom> result;
//     explicit EvalHeapVisitor(const MemoryLookup& adrToMem) : adrToMem(adrToMem) {}
//     void Visit(const plankton::SymbolicHeapFlow &obj) override { result = adrToMem(obj.symbol->Decl()); }
//     void Visit(const plankton::SymbolicHeapEquality &obj) override { result = adrToMem(obj.lhsSymbol->Decl()); }
// };
//
// std::unique_ptr<MemoryAxiom> plankton::GetResource(const SymbolicHeapExpression& expr, const MemoryLookup& adrToMem) {
//     EvalHeapVisitor visitor(adrToMem);
//     expr.Accept(visitor);
//     return std::move(visitor.result);
// }

struct EvalHeapEvaluator : public EffectVisitor {
    const MemoryLookup& adrToMem;
    std::unique_ptr<Formula> result;
    explicit EvalHeapEvaluator(const MemoryLookup& adrToMem) : adrToMem(adrToMem) {}
    void Visit(const plankton::SymbolicHeapFlow &obj) override {
        auto mem = adrToMem(obj.symbol->Decl());
        if (!mem) return;
        result = std::make_unique<InflowEmptinessAxiom>(mem->flow->Decl(), obj.isEmpty);
    }
    void Visit(const plankton::SymbolicHeapEquality &obj) override {
        auto mem = adrToMem(obj.lhsSymbol->Decl());
        if (!mem) return;
        auto& symbol = mem->fieldToValue.at(obj.lhsFieldName)->Decl();
        result = std::make_unique<StackAxiom>(obj.op, std::make_unique<SymbolicVariable>(symbol), plankton::Copy(*obj.rhs));
    }
};

std::unique_ptr<Formula> plankton::TryMakeSymbolic(const SymbolicHeapExpression& expression, const MemoryLookup& adrToMem) {
    EvalHeapEvaluator eval(adrToMem);
    expression.Accept(eval);
    return std::move(eval.result);
}

std::unique_ptr<Formula> plankton::TryMakeSymbolic(const SymbolicHeapExpression& expression, const Formula& context) {
    return plankton::TryMakeSymbolic(expression, [&context](const SymbolDeclaration& adr) -> std::unique_ptr<MemoryAxiom> {
        auto mem = plankton::TryGetResource(adr, context);
        if (!mem) return nullptr;
        return plankton::Copy(*mem);
    });
}

std::unique_ptr<Formula> plankton::TryMakeSymbolic(const std::vector<std::unique_ptr<SymbolicHeapExpression>>& expressions, const MemoryLookup& adrToMem, bool ignoreFailed) {
    auto result = std::make_unique<SeparatingConjunction>();
    for (const auto& expr : expressions) {
        auto symbolic = plankton::TryMakeSymbolic(*expr, adrToMem);
        if (!symbolic && ignoreFailed) continue;
        if (!symbolic) return nullptr;
        result->Conjoin(std::move(symbolic));
    }
    return result;
}

std::unique_ptr<Formula> plankton::TryMakeSymbolic(const std::vector<std::unique_ptr<SymbolicHeapExpression>>& expressions, const Formula& context, bool ignoreFailed) {
    return plankton::TryMakeSymbolic(expressions, [&context](const SymbolDeclaration& adr) -> std::unique_ptr<MemoryAxiom> {
        auto mem = plankton::TryGetResource(adr, context);
        if (!mem) return nullptr;
        return plankton::Copy(*mem);
    }, ignoreFailed);
}
