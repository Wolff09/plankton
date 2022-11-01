#pragma once
#ifndef PLANKTON_ENGINE_CONFIG_HPP
#define PLANKTON_ENGINE_CONFIG_HPP

#include <memory>
#include "programs/ast.hpp"
#include "logics/ast.hpp"

namespace plankton {

    struct SolverConfig {
        explicit SolverConfig() = default;
        virtual ~SolverConfig() = default;

        enum Acyclicity { PHYSICAL, EFFECTIVE, NONE };

        /**
         * Retrieves the type of members of second-order flow variables.
         * @return The type of flow values.
         */
        [[nodiscard]] virtual const Type& GetFlowValueType() const = 0;
        
        /**
         * Suggests a maximum depth beyond which an engine may not explore a (potential) flow footprint.
         * @param type The type of node the update of which leads to a flow footprint being constructed.
         * @param updatedFieldName The field name the update of which leads to a flow footprint being constructed.
         * @return A maximal depth suggestion.
         */
        [[nodiscard]] virtual std::size_t
        GetMaxFootprintDepth(const Type& type, const std::string& updatedField) const = 0;

        /**
         * Gives the acyclicity condition of the overall heap graph.
         * @return The heap graph's acyclicity
         */
        [[nodiscard]] virtual Acyclicity GetGraphAcyclicity() const = 0;
        
        /**
         * An invariant 'I(node)' that is implicitly universally quantified over all local nodes in the heap.
         * @param memory The node that the invariant should be instantiated for.
         * @return Instantiated invariant.
         */
        [[nodiscard]] virtual std::unique_ptr<ImplicationSet>
        GetLocalNodeInvariant(const LocalMemoryResource& memory) const = 0;

        /**
         * An invariant 'I(node)' that is implicitly universally quantified over all shared nodes in the heap.
         * @param memory The node that the invariant should be instantiated for.
         * @return Instantiated invariant.
         */
        [[nodiscard]] virtual std::unique_ptr<ImplicationSet>
        GetSharedNodeInvariant(const SharedMemoryCore& memory) const = 0;

        /**
         * An invariant 'I(node, node)' that is implicitly universally quantified over all pairs of shared nodes in the heap.
         * @param memory The first node that the invariant should be instantiated for.
         * @param other The second node that the invariant should be instantiated for.
         * @return Instantiated invariant.
         */
        [[nodiscard]] virtual std::unique_ptr<ImplicationSet>
        GetSharedNodePairInvariant(const SharedMemoryCore& memory, const SharedMemoryCore& other) const = 0;
        
        /**
         * An invariant 'I(variable)' for the given shared variable.
         * @param variable The variable that the invariant should be instantiated for.
         * @return Instantiated invariant.
         */
        [[nodiscard]] virtual std::unique_ptr<ImplicationSet>
        GetSharedVariableInvariant(const EqualsToAxiom& variable) const = 0;
        
        /**
         * A predicate 'P(node, val)' computing whether or not 'val' is in the outflow of 'node'
		 * via the field named 'fieldName'. This predicate encodes the edge function.
         *
         * The type of 'value' is expected to be the one provided by 'GetFlowValueType()'.
         * The order of 'value' is expected to be first order.
         * The type of 'fieldName' is expected to be of pointer sort.
		 *
		 * NOTE: the outflow is constrained by the inflow of node, i.e., the outflow is precisely
		 *       the set '{ v | v ∈ Inflow(node) && P(node, v) }'.
		 *
         * @param memory The node the outflow of which shall be computed.
         * @param fieldName The field of memory the outflow of which shall be computed.
         * @param value The first-order value that is tested for being in the outflow of fieldName.
         * @return Instantiated predicate.
         */
        [[nodiscard]] virtual std::unique_ptr<ImplicationSet>
        GetOutflowContains(const MemoryAxiom& memory, const std::string& fieldName,
                           const SymbolDeclaration& value) const = 0;
        
        /**
         * A predicate 'P(node, key)' computing whether or not 'node' logically contains 'key'.
         * @param memory The node for which containment shall be computed.
         * @param value The first-order value that is tested for being contained.
         * @return Instantiated predicate.
         */
        [[nodiscard]] virtual std::unique_ptr<ImplicationSet>
        GetLogicallyContains(const MemoryAxiom& memory, const SymbolDeclaration& value) const = 0;

    };
    
} // namespace plankton

#endif //PLANKTON_ENGINE_CONFIG_HPP