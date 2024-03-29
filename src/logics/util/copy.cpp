#include "logics/util.hpp"

#include "programs/util.hpp"

using namespace plankton;


//
// Handling superclasses
//

template<typename T>
struct CopyVisitor : public LogicVisitor {
    std::unique_ptr<T> result;
    
    static std::unique_ptr<T> Copy(const T& object) {
        CopyVisitor<T> visitor;
        object.Accept(visitor);
        assert(visitor.result);
        if (visitor.result) return std::move(visitor.result);
        throw std::logic_error("Internal error: 'Copy' failed."); // TODO: better error handling
    }
    
    template<typename U>
    void Handle(const U& object) {
        if constexpr (std::is_base_of_v<T, U>) result = plankton::Copy(object);
        else throw std::logic_error("Internal error: 'Copy' failed."); // TODO: better error handling
    }
    
    void Visit(const SymbolicVariable& object) override { Handle(object); }
    void Visit(const SymbolicBool& object) override { Handle(object); }
    void Visit(const SymbolicNull& object) override { Handle(object); }
    void Visit(const SymbolicMin& object) override { Handle(object); }
    void Visit(const SymbolicMax& object) override { Handle(object); }
    void Visit(const SymbolicSelfTid& object) override { Handle(object); }
    void Visit(const SymbolicSomeTid& object) override { Handle(object); }
    void Visit(const SymbolicUnlocked& object) override { Handle(object); }
    void Visit(const Guard& object) override { Handle(object); }
    void Visit(const Update& object) override { Handle(object); }
    void Visit(const SeparatingConjunction& object) override { Handle(object); }
    void Visit(const LocalMemoryResource& object) override { Handle(object); }
    void Visit(const SharedMemoryCore& object) override { Handle(object); }
    void Visit(const EqualsToAxiom& object) override { Handle(object); }
    void Visit(const StackAxiom& object) override { Handle(object); }
    void Visit(const InflowEmptinessAxiom& object) override { Handle(object); }
    void Visit(const InflowContainsValueAxiom& object) override { Handle(object); }
    void Visit(const InflowContainsRangeAxiom& object) override { Handle(object); }
    void Visit(const ObligationAxiom& object) override { Handle(object); }
    void Visit(const FulfillmentAxiom& object) override { Handle(object); }
    void Visit(const NonSeparatingImplication& object) override { Handle(object); }
    void Visit(const ImplicationSet& object) override { Handle(object); }
    void Visit(const PastPredicate& object) override { Handle(object); }
    void Visit(const FuturePredicate& object) override { Handle(object); }
    void Visit(const Annotation& object) override { Handle(object); }
};

template<>
std::unique_ptr<LogicObject> plankton::Copy<LogicObject>(const LogicObject& object) {
    return CopyVisitor<LogicObject>::Copy(object);
}

template<>
std::unique_ptr<Formula> plankton::Copy<Formula>(const Formula& object) {
    return CopyVisitor<Formula>::Copy(object);
}

template<>
std::unique_ptr<Axiom> plankton::Copy<Axiom>(const Axiom& object) {
    return CopyVisitor<Axiom>::Copy(object);
}

template<>
std::unique_ptr<SymbolicExpression> plankton::Copy<SymbolicExpression>(const SymbolicExpression& object) {
    return CopyVisitor<SymbolicExpression>::Copy(object);
}

template<>
std::unique_ptr<MemoryAxiom> plankton::Copy<MemoryAxiom>(const MemoryAxiom& object) {
    return CopyVisitor<MemoryAxiom>::Copy(object);
}


//
// Copying objects
//

template<>
std::unique_ptr<SymbolicVariable> plankton::Copy<SymbolicVariable>(const SymbolicVariable& object) {
    return std::make_unique<SymbolicVariable>(object.Decl());
}

template<>
std::unique_ptr<SymbolicBool> plankton::Copy<SymbolicBool>(const SymbolicBool& object) {
    return std::make_unique<SymbolicBool>(object.value);
}

template<>
std::unique_ptr<SymbolicNull> plankton::Copy<SymbolicNull>(const SymbolicNull& /*object*/) {
    return std::make_unique<SymbolicNull>();
}

template<>
std::unique_ptr<SymbolicMin> plankton::Copy<SymbolicMin>(const SymbolicMin& /*object*/) {
    return std::make_unique<SymbolicMin>();
}

template<>
std::unique_ptr<SymbolicMax> plankton::Copy<SymbolicMax>(const SymbolicMax& /*object*/) {
    return std::make_unique<SymbolicMax>();
}

template<>
std::unique_ptr<SymbolicSelfTid> plankton::Copy<SymbolicSelfTid>(const SymbolicSelfTid& /*object*/) {
    return std::make_unique<SymbolicSelfTid>();
}

template<>
std::unique_ptr<SymbolicSomeTid> plankton::Copy<SymbolicSomeTid>(const SymbolicSomeTid& /*object*/) {
    return std::make_unique<SymbolicSomeTid>();
}

template<>
std::unique_ptr<SymbolicUnlocked> plankton::Copy<SymbolicUnlocked>(const SymbolicUnlocked& /*object*/) {
    return std::make_unique<SymbolicUnlocked>();
}

template<>
std::unique_ptr<Guard> plankton::Copy<Guard>(const Guard& object) {
    return std::make_unique<Guard>(plankton::CopyAll(object.conjuncts));
}

template<>
std::unique_ptr<Update> plankton::Copy<Update>(const Update& object) {
    return std::make_unique<Update>(plankton::CopyAll(object.fields), plankton::CopyAll(object.values));
}

template<>
std::unique_ptr<SeparatingConjunction> plankton::Copy<SeparatingConjunction>(const SeparatingConjunction& object) {
    auto result = std::make_unique<SeparatingConjunction>();
    for (const auto& elem : object.conjuncts) result->conjuncts.push_back(plankton::Copy(*elem));
    return result;
}

template<typename T, typename = EnableIfBaseOf<MemoryAxiom, T>>
std::unique_ptr<T> CopyMemoryAxiom(const T& object) {
    auto adr = plankton::Copy(*object.node);
    auto flow = plankton::Copy(*object.flow);
    std::map<std::string, std::unique_ptr<SymbolicVariable>> fields;
    for (const auto& [field, value] : object.fieldToValue) fields[field] = plankton::Copy(*value);
    return std::make_unique<T>(std::move(adr), std::move(flow), std::move(fields));
}

template<>
std::unique_ptr<LocalMemoryResource> plankton::Copy<LocalMemoryResource>(const LocalMemoryResource& object) {
    return CopyMemoryAxiom(object);
}

template<>
std::unique_ptr<SharedMemoryCore> plankton::Copy<SharedMemoryCore>(const SharedMemoryCore& object) {
    return CopyMemoryAxiom(object);
}

template<>
std::unique_ptr<EqualsToAxiom> plankton::Copy<EqualsToAxiom>(const EqualsToAxiom& object) {
    return std::make_unique<EqualsToAxiom>(object.Variable(), plankton::Copy(*object.value));
}

template<>
std::unique_ptr<StackAxiom> plankton::Copy<StackAxiom>(const StackAxiom& object) {
    return std::make_unique<StackAxiom>(object.op, plankton::Copy(*object.lhs), plankton::Copy(*object.rhs));
}

template<>
std::unique_ptr<InflowEmptinessAxiom> plankton::Copy<InflowEmptinessAxiom>(const InflowEmptinessAxiom& object) {
    return std::make_unique<InflowEmptinessAxiom>(plankton::Copy(*object.flow), object.isEmpty);
}

template<>
std::unique_ptr<InflowContainsValueAxiom> plankton::Copy<InflowContainsValueAxiom>(const InflowContainsValueAxiom& object) {
    return std::make_unique<InflowContainsValueAxiom>(plankton::Copy(*object.flow), plankton::Copy(*object.value));
}

template<>
std::unique_ptr<InflowContainsRangeAxiom> plankton::Copy<InflowContainsRangeAxiom>(const InflowContainsRangeAxiom& object) {
    return std::make_unique<InflowContainsRangeAxiom>(plankton::Copy(*object.flow),
                                                      plankton::Copy(*object.valueLow),
                                                      plankton::Copy(*object.valueHigh));
}

template<>
std::unique_ptr<ObligationAxiom> plankton::Copy<ObligationAxiom>(const ObligationAxiom& object) {
    return std::make_unique<ObligationAxiom>(object.spec, plankton::Copy(*object.key));
}

template<>
std::unique_ptr<FulfillmentAxiom> plankton::Copy<FulfillmentAxiom>(const FulfillmentAxiom& object) {
    return std::make_unique<FulfillmentAxiom>(object.returnValue);
}

template<>
std::unique_ptr<NonSeparatingImplication> plankton::Copy<NonSeparatingImplication>(const NonSeparatingImplication& object) {
    auto result = std::make_unique<NonSeparatingImplication>();
    result->premise = plankton::Copy(*object.premise);
    result->conclusion = plankton::Copy(*object.conclusion);
    return result;
}

template<>
std::unique_ptr<ImplicationSet> plankton::Copy<ImplicationSet>(const ImplicationSet& object) {
    auto result = std::make_unique<ImplicationSet>();
    for (const auto& elem : object.conjuncts) result->conjuncts.push_back(plankton::Copy(*elem));
    return result;
}

template<>
std::unique_ptr<PastPredicate> plankton::Copy<PastPredicate>(const PastPredicate& object) {
    return std::make_unique<PastPredicate>(plankton::Copy(*object.formula));
}

template<>
std::unique_ptr<FuturePredicate> plankton::Copy<FuturePredicate>(const FuturePredicate& object) {
    return std::make_unique<FuturePredicate>(plankton::Copy(*object.update), plankton::Copy(*object.guard));
}

template<>
std::unique_ptr<Annotation> plankton::Copy<Annotation>(const Annotation& object) {
    auto result = std::make_unique<Annotation>(plankton::Copy(*object.now));
    for (const auto& elem : object.past) result->past.push_back(plankton::Copy(*elem));
    for (const auto& elem : object.future) result->future.push_back(plankton::Copy(*elem));
    return result;
}
