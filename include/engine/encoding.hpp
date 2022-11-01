#pragma once
#ifndef PLANKTON_ENGINE_ENCODING_HPP
#define PLANKTON_ENGINE_ENCODING_HPP

#include <map>
#include <variant>
#include "logics/ast.hpp"
#include "solver.hpp"
#include "flowgraph.hpp"


namespace plankton {
    
    struct InternalExpr {
        virtual ~InternalExpr() = default;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Negate() const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> And(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Or(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Eq(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Neq(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Lt(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Leq(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Gt(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Geq(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Implies(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Contains(const InternalExpr& other) const = 0;
        [[nodiscard]] virtual std::unique_ptr<InternalExpr> Copy() const = 0;
    };
    
    struct InternalStorage {
        virtual ~InternalStorage() = default;
    };
    
    struct EExpr {
        EExpr operator!() const;
        EExpr operator&&(const EExpr& other) const;
        EExpr operator||(const EExpr& other) const;
        EExpr operator==(const EExpr& other) const;
        EExpr operator!=(const EExpr& other) const;
        EExpr operator<(const EExpr& other) const;
        EExpr operator<=(const EExpr& other) const;
        EExpr operator>(const EExpr& other) const;
        EExpr operator>=(const EExpr& other) const;
        EExpr operator>>(const EExpr& other) const;
        EExpr operator()(const EExpr& other) const;
    
        [[nodiscard]] const InternalExpr& Repr() const;
        explicit EExpr(std::unique_ptr<InternalExpr> repr);
        EExpr(const EExpr& other);
        EExpr& operator=(const EExpr& other);
    
        private:
            std::unique_ptr<InternalExpr> repr;
    };
    
    struct Encoding { // TODO: rename to 'StackEncoding' ?
        explicit Encoding();
        explicit Encoding(const Formula& premise);
        explicit Encoding(const Formula& premise, const SolverConfig& config);
        explicit Encoding(const FlowGraph& graph);
        
        void AddPremise(const EExpr& expr);
        void AddPremise(const Formula& premise);
        void AddPremise(const NonSeparatingImplication& premise);
        void AddPremise(const ImplicationSet& premise);
        void AddPremise(const FlowGraph& graph);
        void Push();
        void Pop();
        
        void AddCheck(const EExpr& expr, std::function<void(bool)> callback);
        void Check();
        
        bool ImpliesFalse();
        bool Implies(const EExpr& expr);
        bool Implies(const Formula& formula);
        bool Implies(const NonSeparatingImplication& formula);
        bool Implies(const ImplicationSet& formula);
        std::set<const SymbolDeclaration*> ComputeNonNull(std::set<const SymbolDeclaration*> symbols);

        EExpr Encode(const VariableDeclaration& decl);
        EExpr Encode(const SymbolDeclaration& decl);
        EExpr Encode(const LogicObject& object);

        EExpr EncodeExists(Sort sort, const std::function<EExpr(EExpr)>& makeInner);
        EExpr EncodeForAll(Sort sort, const std::function<EExpr(EExpr)>& makeInner);
        EExpr EncodeFlowContains(const SymbolDeclaration& flow, const EExpr& expr);
        EExpr EncodeIsNonNull(const EExpr& expr);
        EExpr EncodeIsNonNull(const SymbolDeclaration& decl);
        EExpr EncodeIsNonNull(const VariableDeclaration& decl);
        EExpr EncodeIsNull(const EExpr& expr);
        EExpr EncodeIsNull(const SymbolDeclaration& decl);
        EExpr EncodeIsNull(const VariableDeclaration& decl);
        EExpr EncodeMemoryEquality(const MemoryAxiom& memory, const MemoryAxiom& other);
        EExpr EncodeInvariants(const Formula& formula, const SolverConfig& config);
        EExpr EncodePairInvariants(const Formula& formula, const SolverConfig& config);
        EExpr EncodePairInvariant(const SharedMemoryCore& memory, const SharedMemoryCore& other, const SolverConfig& config);
        EExpr EncodeSimpleFlowRules(const Formula& formula, const SolverConfig& config);
        EExpr EncodeAcyclicity(const Formula& formula, const SolverConfig& config);
        EExpr EncodeOutflowContains(const MemoryAxiom& memory, const std::string& field, const EExpr& value, const SolverConfig& config);
        EExpr EncodeOwnership(const Formula& formula);
        EExpr EncodeFormulaWithKnowledge(const Formula& formula, const SolverConfig& config);
        
        EExpr Encode(const FlowGraph& graph);
        EExpr EncodeNodeInvariant(const FlowGraphNode& node, EMode mode);
        EExpr EncodeKeysetDisjointness(const FlowGraph& graph, EMode mode);
        EExpr EncodeInflowUniqueness(const FlowGraph& graph, EMode mode);
        EExpr EncodeLogicallyContains(const FlowGraphNode& node, const EExpr& value, EMode mode);
        EExpr EncodeOutflowContains(const FlowGraphNode& node, const std::string& field, const EExpr& value, EMode mode);
        EExpr EncodeIsPure(const FlowGraph& graph);
        EExpr EncodeContainsKey(const FlowGraph& graph, const SymbolDeclaration& key);
        EExpr EncodeNotContainsKey(const FlowGraph& graph, const SymbolDeclaration& key);
        EExpr EncodeIsInsertion(const FlowGraph& graph, const SymbolDeclaration& key);
        EExpr EncodeIsDeletion(const FlowGraph& graph, const SymbolDeclaration& key);
        
        EExpr Min();
        EExpr Max();
        EExpr Null();
        EExpr Bool(bool val);
        EExpr TidSelf();
        EExpr TidSome();
        EExpr TidUnlocked();
        
        EExpr MakeDistinct(const std::vector<EExpr>& expressions);
        EExpr MakeAnd(const std::vector<EExpr>& expressions);
        EExpr MakeOr(const std::vector<EExpr>& expressions);
        EExpr MakeAtMost(const std::vector<EExpr>& expressions, unsigned int count);
        
        private:
            std::unique_ptr<InternalStorage> internal;
            std::deque<EExpr> checks_premise;
            std::deque<std::function<void(bool)>> checks_callback;
            std::map<const VariableDeclaration*, EExpr> variableEncoding;
            std::map<const SymbolDeclaration*, EExpr> symbolEncoding;
            
            EExpr MakeQuantifiedVariable(Sort sort);
            EExpr EncodeFlowRules(const FlowGraphNode& node);
            EExpr EncodeOutflow(const FlowGraphNode& node, const PointerField& field, EMode mode);
            EExpr Replace(const EExpr& expression, const EExpr& replace, const EExpr& with);
    };
    
} // plankton

#endif //PLANKTON_ENGINE_ENCODING_HPP
