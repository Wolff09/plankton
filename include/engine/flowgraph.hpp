#pragma once
#ifndef PLANKTON_ENGINE_FLOWGRAPH_HPP
#define PLANKTON_ENGINE_FLOWGRAPH_HPP

#include "logics/ast.hpp"
#include "config.hpp"

namespace plankton {
    
    enum struct EMode { PRE, POST };
    
    struct Field {
        const std::string name;
        const Type& type;
        std::reference_wrapper<const SymbolDeclaration> preValue;
        std::reference_wrapper<const SymbolDeclaration> postValue;
    
        Field(Field&& other) = default;
        Field(const Field& other) = delete;
        Field(std::string name, const Type& type, const SymbolDeclaration& value);
    
        [[nodiscard]] bool HasUpdated() const;
        [[nodiscard]] const SymbolDeclaration& Value(EMode mode) const;
    };
    
    struct PointerField : public Field {
        std::reference_wrapper<const SymbolDeclaration> preAllOutflow;
        std::reference_wrapper<const SymbolDeclaration> preGraphOutflow;
        std::reference_wrapper<const SymbolDeclaration> postAllOutflow;
        std::reference_wrapper<const SymbolDeclaration> postGraphOutflow;
        
        PointerField(PointerField&& other) = default;
        PointerField(const PointerField& other) = delete;
        PointerField(std::string name, const Type& type, const SymbolDeclaration& value,
                     SymbolFactory& factory, const Type& flowType);
        
        [[nodiscard]] const SymbolDeclaration& AllOutflow(EMode mode) const;
        [[nodiscard]] const SymbolDeclaration& GraphOutflow(EMode mode) const;
    };

    struct FlowGraphNode {
        const struct FlowGraph& parent;
        const SymbolDeclaration& address;
        bool needed;
        bool preLocal;
        bool postLocal;
        std::reference_wrapper<const SymbolDeclaration> preAllInflow;
        std::reference_wrapper<const SymbolDeclaration> preGraphInflow;
        std::reference_wrapper<const SymbolDeclaration> preKeyset;
        std::reference_wrapper<const SymbolDeclaration> postAllInflow;
        std::reference_wrapper<const SymbolDeclaration> postGraphInflow;
        std::reference_wrapper<const SymbolDeclaration> postKeyset;
        std::reference_wrapper<const SymbolDeclaration> frameInflow;
        std::vector<Field> dataFields;
        std::vector<PointerField> pointerFields;

        FlowGraphNode(FlowGraphNode&& other) = default;
        FlowGraphNode(const FlowGraphNode& other) = delete;
        FlowGraphNode(const FlowGraph& parent, const SymbolDeclaration& address, bool local,
                      const SymbolDeclaration& preFlow, SymbolFactory& factory);
    
        [[nodiscard]] bool HasUpdated() const;
        [[nodiscard]] bool HasUpdatedFlow() const;
        [[nodiscard]] bool HasUpdatedPointers() const;
        [[nodiscard]] bool HasUpdatedData() const;
        [[nodiscard]] bool IsLocal(EMode mode) const;
        [[nodiscard]] const SymbolDeclaration& AllInflow(EMode mode) const;
        [[nodiscard]] const SymbolDeclaration& GraphInflow(EMode mode) const;
        [[nodiscard]] const SymbolDeclaration& Keyset(EMode mode) const;
        [[nodiscard]] const SymbolDeclaration& FrameInflow() const;
        [[nodiscard]] const Field& GetField(const std::string& name) const;
        [[nodiscard]] Field& GetField(const std::string& name);
        [[nodiscard]] std::unique_ptr<MemoryAxiom> ToLogic(EMode mode) const;
    };

    struct FlowGraph {
        const SolverConfig& config;
        std::unique_ptr<Annotation> pre;
        std::deque<FlowGraphNode> nodes;

        FlowGraph(FlowGraph&& other) = default;
        FlowGraph(const FlowGraph& other) = delete;
        
        [[nodiscard]] const FlowGraphNode& GetRoot() const;
        [[nodiscard]] FlowGraphNode* GetNodeOrNull(const SymbolDeclaration& address);
        [[nodiscard]] const FlowGraphNode* GetNodeOrNull(const SymbolDeclaration& address) const;
        [[nodiscard]] std::vector<const PointerField*> GetIncomingEdges(const FlowGraphNode& node, EMode mode) const;
        [[nodiscard]] std::vector<const PointerField*> GetIncomingEffectiveEdges(const FlowGraphNode& node, EMode mode) const;
        void Print(std::ostream& stream) const;

        private:
            explicit FlowGraph(std::unique_ptr<Annotation> pre, const SolverConfig& config);
            friend FlowGraph MakePureHeapGraph(std::unique_ptr<Annotation>, SymbolFactory&, const SolverConfig&);
            friend FlowGraph MakeFlowFootprint(std::unique_ptr<Annotation>, const MemoryWrite&, const SolverConfig&);
    };

    std::ostream& operator<<(std::ostream& stream, const FlowGraph& object);
    
    [[nodiscard]] FlowGraph MakePureHeapGraph(std::unique_ptr<Annotation> state, SymbolFactory& factory, const SolverConfig& config);
    [[nodiscard]] FlowGraph MakeFlowFootprint(std::unique_ptr<Annotation> pre, const MemoryWrite& update, const SolverConfig& config);
    
} // namespace plankton

#endif //PLANKTON_ENGINE_FLOWGRAPH_HPP
