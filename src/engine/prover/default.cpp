#include "engine/prover.hpp"

#include "util/log.hpp"

using namespace plankton;


struct TimeReasoningListener final : public ProofListener {
    Prover& prover;
    bool goFast;

    TimeReasoningListener(Prover& prover, bool goFast) : prover(prover), goFast(goFast) {}

    void BeforePrune() override {
        // past/future reasoning?
    }

    void BeforeApplyInterference() override {
        prover.Transform([this](auto annotation) {
            return prover.solver->ImprovePast(std::move(annotation));
        });
    }

    void AfterApplyInterference() override {
        prover.Transform([this](auto annotation) {
            return prover.solver->ReducePast(std::move(annotation));
        });
    }

    void BeforeJoin() override {
        prover.Prune();
        prover.ImprovePast();
        prover.ReducePast();
        prover.ImproveFuture();
        prover.ReduceFuture();
        // prover.Prune();
    }

    void AfterJoin() override {
        prover.ReducePast();
        prover.ReduceFuture();
    }

    void AfterWiden() override {
        prover.ReducePast();
        prover.ReduceFuture();
    }

};


std::unique_ptr<Prover> plankton::MakeDefaultProver(std::unique_ptr<Solver> solver, const Program& program, bool precise) {
    auto futureSuggestions = plankton::SuggestFutures(program);
    auto result = std::make_unique<Prover>(std::move(solver), std::move(futureSuggestions));
    result->AddListener(std::make_shared<TimeReasoningListener>(*result, !precise));
    return result;
}

std::unique_ptr<Prover> plankton::MakeDefaultProver(const SolverConfig& config, const Program& program, bool precise) {
    auto solver = std::make_unique<Solver>(program, config);
    return MakeDefaultProver(std::move(solver), program, precise);
}
