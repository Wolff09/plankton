#include "engine/solver.hpp"

#include "logics/util.hpp"
#include "engine/encoding.hpp"
#include "engine/util.hpp"

using namespace plankton;



struct AxiomAnalyser : public DefaultLogicVisitor {
    bool result = false;
    
    bool IsStack(const LogicObject& object) {
        result = false;
        object.Accept(*this);
        return result;
    }
    
    void Visit(const StackAxiom& /*object*/) override { result = true; }
    void Visit(const InflowEmptinessAxiom& /*object*/) override { result = true; }
    void Visit(const InflowContainsValueAxiom& /*object*/) override { result = true; }
    void Visit(const InflowContainsRangeAxiom& /*object*/) override { result = true; }
};

inline bool IsStack(const std::unique_ptr<Formula>& object) {
    static AxiomAnalyser analyser;
    return analyser.IsStack(*object);
}

inline std::unique_ptr<Annotation> Strip(const Annotation& annotation) {
    auto result = plankton::Copy(annotation);
    plankton::RemoveIf(result->now->conjuncts, IsStack);
    return result;
}

inline bool ResourcesMatch(const Annotation& premise, const Annotation& conclusion) {
    return plankton::SyntacticalEqual(*Strip(premise), *Strip(conclusion));
}

inline bool StackImplies(const Formula& premise, const SeparatingConjunction& conclusion, const SolverConfig& config) {
    Encoding encoding(premise);
    encoding.AddPremise(encoding.EncodeInvariants(premise, config));
     return encoding.Implies(conclusion);
}

inline void TryAvoidResourceMismatch(Annotation& annotation, Annotation& other, const SolverConfig& config) {
    // TODO: extend stack with pointer equalities?
    std::set<const SymbolDeclaration*> memories;
    for (auto* elem : plankton::Collect<SharedMemoryCore>(*annotation.now)) memories.insert(&elem->node->Decl());
    for (auto* elem : plankton::Collect<SharedMemoryCore>(*other.now)) memories.insert(&elem->node->Decl());
    plankton::MakeMemoryAccessible(annotation, memories, config);
    plankton::MakeMemoryAccessible(other, memories, config);
}

bool Solver::Implies(const Annotation& premise, const Annotation& conclusion) const {
    auto normalizedPremise = plankton::Normalize(plankton::Copy(premise));
    auto normalizedConclusion = plankton::Normalize(plankton::Copy(conclusion));
    plankton::Simplify(*normalizedPremise);
    plankton::Simplify(*normalizedConclusion);
    if (plankton::SyntacticalEqual(*normalizedPremise, *normalizedConclusion)) return true;
    
    TryAvoidResourceMismatch(*normalizedPremise, *normalizedConclusion, config);
    plankton::InlineAndSimplify(*normalizedPremise->now);
    plankton::InlineAndSimplify(*normalizedConclusion->now);
    normalizedPremise = plankton::Normalize(std::move(normalizedPremise));
    normalizedConclusion = plankton::Normalize(std::move(normalizedConclusion));

    if (plankton::SyntacticalEqual(*normalizedPremise, *normalizedConclusion)) return true;
    return ResourcesMatch(*normalizedPremise, *normalizedConclusion) &&
           StackImplies(*normalizedPremise->now, *normalizedConclusion->now, config);
}