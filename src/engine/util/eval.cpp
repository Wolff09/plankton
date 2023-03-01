#include "engine/util.hpp"

#include <utility>
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
