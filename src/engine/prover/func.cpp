#include "engine/prover.hpp"

#include "programs/util.hpp"
#include "logics/util.hpp"
#include "engine/util.hpp"

using namespace plankton;


//
// Helpers
//

inline std::unique_ptr<SymbolicVariable> GetSearchKeyValue(const Annotation& annotation, const Function& function) {
    if (function.parameters.size() != 1) {
        throw std::logic_error("Unexpected number of parameters to function '" + function.name + "': expected 1, got " + std::to_string(function.parameters.size()) + "."); // TODO: better error handling
    }
    auto& resource = plankton::GetResource(*function.parameters.front(), *annotation.now);
    return std::make_unique<SymbolicVariable>(resource.Value());
}

inline Specification GetSpecification(const Function& function) { // TODO: move to utils?
    if (function.name == "contains") {
        return Specification::CONTAINS;
    } else if (function.name == "insert" || function.name == "add") {
        return Specification::INSERT;
    } else if (function.name == "delete" || function.name == "remove") {
        return Specification::DELETE;
    } else {
        throw std::logic_error("Specification for function '" + function.name + "' unknown, expected one of: 'contains', 'insert', 'add', 'delete', 'remove'."); // TODO: better error handling
    }
}

inline std::unique_ptr<Formula> MakeKeyBounds(const SymbolDeclaration& key) {
    static auto mkLt = [](auto lhs, auto rhs){
        return std::make_unique<StackAxiom>(BinaryOperator::LT, std::move(lhs), std::move(rhs));
    };
    auto result = std::make_unique<SeparatingConjunction>();
    result->Conjoin(mkLt(std::make_unique<SymbolicMin>(), std::make_unique<SymbolicVariable>(key)));
    result->Conjoin(mkLt(std::make_unique<SymbolicVariable>(key), std::make_unique<SymbolicMax>()));
    return result;
}

inline decltype(Prover::current) MakeInitialAnnotation(const Function& function, const Solver& solver) {
    auto result = std::make_unique<Annotation>();
    result = solver.PostEnter(std::move(result), program);
    result = solver.PostEnter(std::move(result), function);
    auto key = GetSearchKeyValue(*result, function);
    result->Conjoin(MakeKeyBounds(key->Decl()));
    if (function.kind == Function::API) {
        auto spec = GetSpecification(function);
        auto obligation = std::make_unique<ObligationAxiom>(spec, std::move(key));
        result->Conjoin(std::move(obligation));
    } else assert(function.kind == Function::MAINTENANCE);
    plankton::Simplify(*result);

    decltype(Prover::current) container;
    container.push_back(std::move(result));
    return container;
}

inline bool IsFulfilled(const Annotation& annotation, const Prover::ReturnValues& values) {
    if (values.size() != 1) {
        throw std::logic_error("Unexpected number of return values: expected 1, got " + std::to_string(values.size()) + "."); // TODO: better error handling
    }
    auto returnExpression = values.front().get();
    bool returnValue;
    // TODO: infer return value semantically
    if (dynamic_cast<const TrueValue*>(returnExpression)) returnValue = true;
    else if (dynamic_cast<const FalseValue*>(returnExpression)) returnValue = false;
    else throw std::logic_error("Cannot detect return value"); // TODO: better error handling
    auto fulfillments = plankton::Collect<FulfillmentAxiom>(*annotation.now);
    return std::any_of(fulfillments.begin(), fulfillments.end(),
                       [returnValue](auto* elem) { return elem->returnValue == returnValue; });
}

//
// Function handling
//

inline void HandleFunctionBody(const Function& func, Prover& prover) {
    // reset
    prover.returning.clear();
    prover.breaking.clear();

    // descent
    prover.HandleScope(*func.body);

    // check for missing return
    if (!prover.current.empty() && !plankton::IsVoid(func)) {
        throw std::logic_error("Detected non-returning path through non-void function '" + func.name + "'."); // TODO: better error handling
    }

    // patch void return
    static Return returnVoid;
    prover.HandleReturn(returnVoid);
    assert(prover.current.empty());
    prover.current.clear();
}

void Prover::HandleFunction(const Function& object) {
    switch (object.kind) {
        case Function::API: HandleApiFunction(object); break;
        case Function::MAINTENANCE: HandleMaintenanceFunction(object); break;
        case Function::MACRO: HandleMacroFunction(object); break;
        case Function::INIT: break; // TODO: handle init
    }
}

void Prover::HandleMacroFunction(const Function& object) {
    Callback(&ProofListener::BeforeHandleMacroFunction, std::cref(object));
    HandleFunctionBody(object, *this);
    CallbackReverse(&ProofListener::AfterHandleMacroFunction, std::cref(object));
}

void Prover::HandleMaintenanceFunction(const Function& object) {
    Callback(&ProofListener::BeforeHandleMaintenanceFunction, std::cref(object));
    current = MakeInitialAnnotation(object, *solver);
    HandleFunctionBody(object, *this);
    CallbackReverse(&ProofListener::AfterHandleMaintenanceFunction, std::cref(object));
}

void Prover::HandleApiFunction(const Function& object) {
    Callback(&ProofListener::BeforeHandleApiFunction, std::cref(object));

    // generate proof
    current = MakeInitialAnnotation(object, *solver);
    HandleFunctionBody(object, *this);
    Prune();
    ImprovePast();

    // check linearizability
    Callback(&ProofListener::BeforeLinearizabilityCheck, std::cref(object));
    for (auto&[annotation, values]: returning) {
        if (IsFulfilled(*annotation, values)) continue;
        annotation = solver->TryAddFulfillment(std::move(annotation));
        if (IsFulfilled(*annotation, values)) continue;
        throw std::logic_error("Could not establish linearizability for function '" + object.name + "'."); // TODO: better error handling
    }

    CallbackReverse(&ProofListener::AfterLinearizabilityCheck, std::cref(object));
    CallbackReverse(&ProofListener::AfterHandleApiFunction, std::cref(object));
}