#include "engine/solver.hpp"

#include "engine/util.hpp"

using namespace plankton;


struct PointerCollector : public ProgramListener {
    std::set<const VariableDeclaration*> result;
    void Enter(const VariableDeclaration& object) override { result.insert(&object); }
};

inline void CheckVariableAccess(const Command& command, const Annotation& annotation) {
    PointerCollector collector;
    command.Accept(collector);
    for (const auto& decl : collector.result) {
        auto resource = plankton::TryGetResource(*decl, *annotation.now);
        if (resource) continue;
        throw std::logic_error("No resource for access of variable '" + decl->name + "'."); // TODO: better error handling
    }
}

struct DereferenceCollector : public ProgramListener {
    std::set<const VariableDeclaration*> result;
    void Enter(const Dereference& object) override { result.insert(&object.variable->Decl()); }
    void Enter(const AssertFlow& object) override { result.insert(&object.object->Decl()); }
    void Enter(const AssumeFlow& object) override { result.insert(&object.object->Decl()); }
};

inline void CheckMemoryAccess(const Command& command, const Annotation& annotation) {
    DereferenceCollector collector;
    command.Accept(collector);
    for (const auto& decl : collector.result) {
        auto& variable = plankton::GetResource(*decl, *annotation.now);
        auto memory = plankton::TryGetResource(variable.Value(), *annotation.now);
        if (memory) continue;
        throw std::logic_error("No resource for dereference of variable '" + decl->name + "'."); // TODO: better error handling
    }
}

inline void MakeAccessible(Annotation& annotation, const Command& command, const SolverConfig& config,
                           const DataFlowAnalysis& dataFlow) {
    // get dereferenced variables
    DereferenceCollector collector;
    command.Accept(collector);

    // prune non-shared variables, evaluate
    std::set<const SymbolDeclaration*> symbols;
    for (const auto* variable : collector.result) {
        if (variable->type.sort != Sort::PTR) continue;
        // TODO: more involved shared reachability check?
        if (!dataFlow.AlwaysPointsToShared(*variable)) continue;
        
        auto& resource = plankton::GetResource(*variable, *annotation.now);
        symbols.insert(&resource.Value());
    }
    
    // make accessible
    plankton::MakeMemoryAccessible(annotation, symbols, config);
}

void Solver::PrepareAccess(Annotation& annotation, const Command& command) const {
    CheckVariableAccess(command, annotation);
    MakeAccessible(annotation, command, config, dataFlow);
    CheckMemoryAccess(command, annotation);
}
