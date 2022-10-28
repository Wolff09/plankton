#pragma once
#ifndef PLANKTON_ENGINE_PROOF_HPP
#define PLANKTON_ENGINE_PROOF_HPP

#include <deque>
#include <memory>
#include "programs/ast.hpp"
#include "logics/ast.hpp"
#include "engine/config.hpp"
#include "engine/solver.hpp"
#include "engine/setup.hpp"
#include "util/log.hpp"
#include "util/timer.hpp"

namespace plankton {

    struct ProofGenerator final : public BaseProgramVisitor {
        explicit ProofGenerator(const Program& program, const SolverConfig& config, EngineSetup setup);
        void GenerateProof();

        // TODO: can we make those inaccessible for callers generating a proof?
        void Visit(const Sequence& object) override;
        void Visit(const Scope& object) override;
        void Visit(const Atomic& object) override;
        void Visit(const Choice& object) override;
        void Visit(const UnconditionalLoop& object) override;
        void Visit(const Skip& object) override;
        void Visit(const Break& object) override;
        void Visit(const Assume& object) override;
        void Visit(const AssertFlow& object) override;
        void Visit(const AssumeFlow& object) override;
        void Visit(const Fail& object) override;
        void Visit(const Return& object) override;
        void Visit(const Malloc& object) override;
        void Visit(const Macro& object) override;
        void Visit(const AcquireLock& object) override;
        void Visit(const ReleaseLock& object) override;
        void Visit(const VariableAssignment& object) override;
        void Visit(const MemoryWrite& object) override;
        void Visit(const UpdateStub& object) override;
        void Visit(const Suggestion& object) override;
        void Visit(const Function& object) override;
        void Visit(const Program& object) override;

    private:
        using AnnotationList = std::deque<std::unique_ptr<Annotation>>;
        using PrePostPair = std::pair<std::unique_ptr<Annotation>, AnnotationList>;

        const Program& program;
        Solver solver;
        EngineSetup setup;
        std::deque<std::unique_ptr<HeapEffect>> newInterference;
        std::deque<std::unique_ptr<Annotation>> current;
        std::deque<std::unique_ptr<Annotation>> breaking;
        std::deque<std::pair<std::unique_ptr<Annotation>, const Return*>> returning;
        std::map<const Function*, std::deque<PrePostPair>> macroPostTable;
        bool insideAtomic;
        bool firstRound;
        std::deque<std::unique_ptr<FutureSuggestion>> futureSuggestions;

        #define INFO_SIZE (" (" + std::to_string(current.size()) + ") ")
        StatusStack infoPrefix;
        Timer timePost, timeJoin, timeInterference, timePastImprove, timePastReduce, timeFutureImprove, timeFutureReduce;
    
        void HandleInterfaceFunction(const Function& function);
        void HandleMacroLazy(const Macro& macro);
        void HandleMacroEager(const Macro& macro);
        void HandleMacroProlog(const Macro& macro);
        void HandleMacroEpilog(const Macro& macro);
        std::optional<AnnotationList> LookupMacroPost(const Macro& node, const Annotation& pre);
        void AddMacroPost(const Macro& node, const Annotation& pre, const AnnotationList& post);

        void MakeInterferenceStable(const Statement& after);
        void AddNewInterference(std::deque<std::unique_ptr<HeapEffect>> effects);
        bool ConsolidateNewInterference();

        void JoinCurrent();
        void PruneCurrent();
        void PruneReturning();
        void ImproveCurrentTime();
        void ReduceCurrentTime();
        void LeaveAllNestedScopes(const AstNode& node);
        void ApplyTransformer(const std::function<std::unique_ptr<Annotation>(std::unique_ptr<Annotation>)>& transformer);
        void ApplyTransformer(const std::function<PostImage(std::unique_ptr<Annotation>)>& transformer);
    };

} // namespace plankton

#endif //PLANKTON_ENGINE_PROOF_HPP
