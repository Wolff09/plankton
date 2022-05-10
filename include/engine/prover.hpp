#pragma once
#ifndef PLANKTON_ENGINE_PROVER_HPP
#define PLANKTON_ENGINE_PROVER_HPP

#include <deque>
#include <memory>
#include "programs/ast.hpp"
#include "logics/ast.hpp"
#include "engine/config.hpp"
#include "engine/solver.hpp"
#include "util/shortcuts.hpp"

namespace plankton {

    struct ProofListener;

    struct Prover {
        using ReturnValues = std::vector<std::unique_ptr<SymbolicExpression>>;

        std::unique_ptr<Solver> solver;
        std::deque<std::unique_ptr<FutureSuggestion>> futureSuggestions;

        std::deque<std::unique_ptr<HeapEffect>> discoveredInterference;
        std::deque<std::unique_ptr<Annotation>> current;
        std::deque<std::unique_ptr<Annotation>> continuing;
        std::deque<std::unique_ptr<Annotation>> breaking;
        std::deque<std::pair<std::unique_ptr<Annotation>, ReturnValues>> returning;

        explicit Prover(std::unique_ptr<Solver> solver);
        explicit Prover(std::unique_ptr<Solver> solver, std::deque<std::unique_ptr<FutureSuggestion>> futureSuggestions);
        virtual ~Prover() = default;

        inline void GenerateProof(const Program& program) { HandleProgram(program); }

        //
        // Proof generation
        //

        virtual void HandleProgram(const Program& object);
        virtual void HandleFunction(const Function& object);
        virtual void HandleApiFunction(const Function& object);
        virtual void HandleMaintenanceFunction(const Function& object);
        virtual void HandleMacroFunction(const Function& object);
        virtual void HandleSequence(const Sequence& object);
        virtual void HandleScope(const Scope& object);
        virtual void HandleAtomic(const Atomic& object);
        virtual void HandleChoice(const Choice& object);
        virtual void HandleLoop(const UnconditionalLoop& object);
        virtual void HandleSkip(const Skip& object);
        virtual void HandleBreak(const Break& object);
        virtual void HandleAssume(const Assume& object);
        virtual void HandleFail(const Fail& object);
        virtual void HandleReturn(const Return& object);
        virtual void HandleMalloc(const Malloc& object);
        virtual void HandleMacro(const Macro& object);
        virtual void HandleAcquireLock(const AcquireLock& object);
        virtual void HandleReleaseLock(const ReleaseLock& object);
        virtual void HandleVariableAssignment(const VariableAssignment& object);
        virtual void HandleMemoryWrite(const MemoryWrite& object);
        virtual void Handle(const AstNode& object);

        //
        // Helpers
        //

        virtual bool NeedsInterference(const Statement& object);
        virtual void ApplyInterference(const Statement& object);
        virtual void ApplyInterference();
        virtual void AddDiscoveredInterference(std::deque<std::unique_ptr<HeapEffect>> effects);
        virtual bool ConsolidateDiscoveredInterference();

        virtual void ImprovePast();
        virtual void ImproveFuture();
        virtual void ReducePast();
        virtual void ReduceFuture();
        virtual void Prune();
        virtual void Join();
        virtual void Widen();

        void Transform(const std::function<std::unique_ptr<Annotation>(std::unique_ptr<Annotation>)>& transformer);
        void Transform(const std::function<PostImage(std::unique_ptr<Annotation>)>& transformer);

        //
        // Callback shenanigans
        //

        void AddListener(std::shared_ptr<ProofListener> listener) {
            if (!listener) return;
            listeners.push_back(std::move(listener));
        }

        template<typename... Args>
        void AddListener(std::shared_ptr<ProofListener> listener, Args... args) {
            AddListener(std::move(listener));
            AddListener(args...);
        }

        private:
            std::vector<std::shared_ptr<ProofListener>> listeners;
            template<typename Func, typename... Args>
            void Callback(Func memberPtr, Args... args) {
                for (auto& listener : listeners)
                    std::bind(std::mem_fn(memberPtr), listener.get(), std::forward<Args>(args)...)();
            }
            template<typename Func, typename... Args>
            void CallbackReverse(Func memberPtr, Args... args) {
                for (auto it = listeners.rbegin(); it != listeners.rend(); ++it)
                    std::bind(std::mem_fn(memberPtr), it->get(), std::forward<Args>(args)...)();
            }
    };


    struct ProofListener {
        virtual ~ProofListener() = default;

        virtual void BeforeHandleSequence(const Sequence& /*object*/) {}
        virtual void BeforeHandleScope(const Scope& /*object*/) {}
        virtual void BeforeHandleAtomic(const Atomic& /*object*/) {}
        virtual void BeforeHandleChoice(const Choice& /*object*/) {}
        virtual void BeforeHandleLoop(const UnconditionalLoop& /*object*/) {}
        virtual void BeforeHandleLoopBody(const UnconditionalLoop& /*object*/) {}
        virtual void BeforeHandleLoopIteration(const UnconditionalLoop& /*object*/, std::size_t /*iteration*/) {}
        virtual void BeforeHandleSkip(const Skip& /*object*/) {}
        virtual void BeforeHandleBreak(const Break& /*object*/) {}
        virtual void BeforeHandleAssume(const Assume& /*object*/) {}
        virtual void BeforeHandleFail(const Fail& /*object*/) {}
        virtual void BeforeHandleReturn(const Return& /*object*/) {}
        virtual void BeforeHandleMalloc(const Malloc& /*object*/) {}
        virtual void BeforeHandleMacro(const Macro& /*object*/) {}
        virtual void BeforeHandleAcquireLock(const AcquireLock& /*object*/) {}
        virtual void BeforeHandleReleaseLock(const ReleaseLock& /*object*/) {}
        virtual void BeforeHandleVariableAssignment(const VariableAssignment& /*object*/) {}
        virtual void BeforeHandleMemoryWrite(const MemoryWrite& /*object*/) {}
        virtual void BeforeHandleApiFunction(const Function& /*object*/) {}
        virtual void BeforeHandleMaintenanceFunction(const Function& /*object*/) {}
        virtual void BeforeHandleMacroFunction(const Function& /*object*/) {}
        virtual void BeforeHandleProgram(const Program& /*object*/) {}

        virtual void AfterHandleSequence(const Sequence& /*object*/) {}
        virtual void AfterHandleScope(const Scope& /*object*/) {}
        virtual void AfterHandleAtomic(const Atomic& /*object*/) {}
        virtual void AfterHandleChoice(const Choice& /*object*/) {}
        virtual void AfterHandleLoop(const UnconditionalLoop& /*object*/) {}
        virtual void AfterHandleLoopBody(const UnconditionalLoop& /*object*/) {}
        virtual void AfterHandleLoopIteration(const UnconditionalLoop& /*object*/, std::size_t /*iteration*/) {}
        virtual void AfterHandleSkip(const Skip& /*object*/) {}
        virtual void AfterHandleBreak(const Break& /*object*/) {}
        virtual void AfterHandleAssume(const Assume& /*object*/) {}
        virtual void AfterHandleFail(const Fail& /*object*/) {}
        virtual void AfterHandleReturn(const Return& /*object*/) {}
        virtual void AfterHandleMalloc(const Malloc& /*object*/) {}
        virtual void AfterHandleMacro(const Macro& /*object*/) {}
        virtual void AfterHandleAcquireLock(const AcquireLock& /*object*/) {}
        virtual void AfterHandleReleaseLock(const ReleaseLock& /*object*/) {}
        virtual void AfterHandleVariableAssignment(const VariableAssignment& /*object*/) {}
        virtual void AfterHandleMemoryWrite(const MemoryWrite& /*object*/) {}
        virtual void AfterHandleApiFunction(const Function& /*object*/) {}
        virtual void AfterHandleMaintenanceFunction(const Function& /*object*/) {}
        virtual void AfterHandleMacroFunction(const Function& /*object*/) {}
        virtual void AfterHandleProgram(const Program& /*object*/) {}

        virtual void BeforeApplyInterference() {}
        virtual void BeforeAddDiscoveredInterference() {}
        virtual void BeforeConsolidateDiscoveredInterference() {}
        virtual void BeforeImprovePast() {}
        virtual void BeforeImproveFuture() {}
        virtual void BeforeReducePast() {}
        virtual void BeforeReduceFuture() {}
        virtual void BeforePrune() {}
        virtual void BeforeJoin() {}
        virtual void BeforeWiden() {}
        virtual void BeforeLinearizabilityCheck(const Function& /*object*/) {}

        virtual void AfterApplyInterference() {}
        virtual void AfterAddDiscoveredInterference() {}
        virtual void AfterConsolidateDiscoveredInterference(bool /*result*/) {}
        virtual void AfterImprovePast() {}
        virtual void AfterImproveFuture() {}
        virtual void AfterReducePast() {}
        virtual void AfterReduceFuture() {}
        virtual void AfterPrune() {}
        virtual void AfterJoin() {}
        virtual void AfterWiden() {}
        virtual void AfterLinearizabilityCheck(const Function& /*object*/) {}
    };

    std::unique_ptr<Prover> MakeDefaultProver(const SolverConfig& config, const Program& program, bool precise = false);
    std::unique_ptr<Prover> MakeDefaultProver(std::unique_ptr<Solver> solver, const Program& program, bool precise = false);

}

#endif //PLANKTON_ENGINE_PROVER_HPP
