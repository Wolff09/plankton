#pragma once
#ifndef PLANKTON_ENGINE_UTIL_HPP
#define PLANKTON_ENGINE_UTIL_HPP

#include "programs/ast.hpp"
#include "logics/ast.hpp"
#include "engine/solver.hpp"
#include "engine/encoding.hpp"

namespace plankton {

    std::set<const SymbolDeclaration*> CollectUsefulSymbols(const LogicObject& object);
    
    const EqualsToAxiom* TryGetResource(const VariableDeclaration& variable, const Formula& state);
    const EqualsToAxiom& GetResource(const VariableDeclaration& variable, const Formula& state);
    EqualsToAxiom& GetResource(const VariableDeclaration& variable, Formula& state);
    
    const MemoryAxiom* TryGetResource(const SymbolDeclaration& address, const Formula& state);
    const MemoryAxiom& GetResource(const SymbolDeclaration& address, const Formula& state);
    MemoryAxiom& GetResource(const SymbolDeclaration& address, Formula& state);

    const SymbolDeclaration& Evaluate(const VariableExpression& variable, const Formula& state);
    const SymbolDeclaration& Evaluate(const Dereference& dereference, const Formula& state);
    const SymbolDeclaration* TryEvaluate(const VariableExpression& variable, const Formula& state);
    const SymbolDeclaration* TryEvaluate(const Dereference& dereference, const Formula& state);

    std::unique_ptr<SymbolicExpression> MakeSymbolic(const ValueExpression& expression, const Formula& context);
    std::unique_ptr<SymbolicExpression> TryMakeSymbolic(const ValueExpression& expression, const Formula& context);

    bool UpdatesFlow(const HeapEffect& effect);
    bool UpdatesField(const HeapEffect& effect, const std::string& field);
    
    void AvoidEffectSymbols(SymbolFactory& factory, const HeapEffect& effect);
    void AvoidEffectSymbols(SymbolFactory& factory, const std::deque<std::unique_ptr<HeapEffect>>& effects);
    
    void MakeMemoryAccessible(SeparatingConjunction& formula, std::set<const SymbolDeclaration*> addresses,
                              const Type& flowType, SymbolFactory& factory, Encoding& encoding);
    void MakeMemoryAccessible(Annotation& annotation, std::set<const SymbolDeclaration*> addresses,
                              const SolverConfig& config);
    
    enum struct ExtensionPolicy { POINTERS, FAST, FULL };
    std::deque<std::unique_ptr<Axiom>> MakeStackCandidates(const LogicObject& object, ExtensionPolicy policy);
    std::deque<std::unique_ptr<Axiom>> MakeStackCandidates(const LogicObject& object, const LogicObject& other, ExtensionPolicy policy);
    void ExtendStack(Annotation& annotation, Encoding& encoding, ExtensionPolicy policy);
    void ExtendStack(Annotation& annotation, const SolverConfig& config, ExtensionPolicy policy);

    struct ReachSet {
        std::map<const SymbolDeclaration*, std::set<const SymbolDeclaration*>> container;
        [[nodiscard]] bool IsReachable(const SymbolDeclaration& source, const SymbolDeclaration& target) const;
        [[nodiscard]] const std::set<const SymbolDeclaration*>& GetReachable(const SymbolDeclaration& source) const;
    };
    ReachSet ComputeReachability(const Formula& formula);
    ReachSet ComputeReachability(const FlowGraph& graph, EMode mode);
    ReachSet ComputeEffectiveReachability(const Formula& formula, const SolverConfig& config);
    ReachSet ComputeEffectiveReachability(const FlowGraph& graph, EMode mode);
    bool IsOutflowFalse(const Type& type, const std::string& field, const SolverConfig& config);

    void AddPureCheck(const FlowGraph& graph, Encoding& encoding, const std::function<void(bool)>& acceptPurity);
    void AddPureSpecificationCheck(const FlowGraph& graph, Encoding& encoding, const ObligationAxiom& obligation,
                                   const std::function<void(std::optional<bool>)>& acceptFulfillment);
    void AddImpureSpecificationCheck(const FlowGraph& graph, Encoding& encoding, const ObligationAxiom& obligation,
                                     const std::function<void(std::optional<bool>)>& acceptFulfillment);
}

#endif //PLANKTON_ENGINE_UTIL_HPP
