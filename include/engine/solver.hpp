#pragma once
#ifndef PLANKTON_ENGINE_SOLVER_HPP
#define PLANKTON_ENGINE_SOLVER_HPP

#include <deque>
#include <memory>
#include "programs/ast.hpp"
#include "logics/ast.hpp"
#include "engine/config.hpp"
#include "engine/static.hpp"
#include "engine/setup.hpp"
#include "engine/effect.hpp"

namespace plankton {

    struct PostImage final {
        std::deque<std::unique_ptr<Annotation>> annotations;
        std::deque<std::unique_ptr<HeapEffect>> effects;

        explicit PostImage();
        explicit PostImage(std::unique_ptr<Annotation> post);
        explicit PostImage(std::deque<std::unique_ptr<Annotation>> posts);
        explicit PostImage(std::unique_ptr<Annotation> post, std::deque<std::unique_ptr<HeapEffect>> effects);
        explicit PostImage(std::deque<std::unique_ptr<Annotation>> posts, std::deque<std::unique_ptr<HeapEffect>> effects);
    };

    struct Solver final {
        explicit Solver(const Program& program, const SolverConfig& config);

        [[nodiscard]] std::unique_ptr<Annotation> PostEnter(std::unique_ptr<Annotation> pre, const Program& scope) const;
        [[nodiscard]] std::unique_ptr<Annotation> PostEnter(std::unique_ptr<Annotation> pre, const Function& scope) const;
        [[nodiscard]] std::unique_ptr<Annotation> PostEnter(std::unique_ptr<Annotation> pre, const Scope& scope) const;
        [[nodiscard]] std::unique_ptr<Annotation> PostLeave(std::unique_ptr<Annotation> pre, const Function& scope) const;
        [[nodiscard]] std::unique_ptr<Annotation> PostLeave(std::unique_ptr<Annotation> pre, const Scope& scope) const;

        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const Assume& cmd) const;
        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const AssertFlow& cmd) const;
        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const AssumeFlow& cmd) const;
        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const UpdateStub& cmd) const;
        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const Malloc& cmd) const;
        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const AcquireLock& cmd) const;
        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const ReleaseLock& cmd) const;
        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const VariableAssignment& cmd) const;
        [[nodiscard]] PostImage Post(std::unique_ptr<Annotation> pre, const MemoryWrite& cmd, bool useFuture = true) const;

        [[nodiscard]] std::unique_ptr<Annotation> Join(std::deque<std::unique_ptr<Annotation>> annotations) const;
        [[nodiscard]] std::unique_ptr<Annotation> Widen(std::unique_ptr<Annotation> annotation) const;
        [[nodiscard]] std::unique_ptr<Annotation> TryAddFulfillment(std::unique_ptr<Annotation> annotation) const;

        bool AddInterference(std::deque<std::unique_ptr<HeapEffect>> interference);
        [[nodiscard]] std::unique_ptr<Annotation> MakeInterferenceStable(std::unique_ptr<Annotation> annotation) const;

        [[nodiscard]] bool IsUnsatisfiable(const Annotation& annotation) const;
        [[nodiscard]] bool Implies(const Annotation& premise, const Annotation& conclusion) const;

        [[nodiscard]] std::unique_ptr<Annotation> ImprovePast(std::unique_ptr<Annotation> annotation) const; // TODO: past suggestions
        [[nodiscard]] PostImage ImproveFuture(std::unique_ptr<Annotation> annotation, const FutureSuggestion& target) const;
        [[nodiscard]] std::unique_ptr<Annotation> ReducePast(std::unique_ptr<Annotation> annotation) const;
        [[nodiscard]] std::unique_ptr<Annotation> ReduceFuture(std::unique_ptr<Annotation> annotation) const;

        private:
            const Program& program;
            const SolverConfig& config;
            DataFlowAnalysis dataFlow;
            std::deque<std::unique_ptr<HeapEffect>> interference;
            
            void PrepareAccess(Annotation& annotation, const Command& command) const;
            void ReducePast(Annotation& annotation) const;
            void ReduceFuture(Annotation& annotation) const;
    };

    std::ostream& operator<<(std::ostream& out, const HeapEffect& object);

} // namespace plankton

#endif //PLANKTON_ENGINE_SOLVER_HPP
