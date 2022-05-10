#include "engine/prover.hpp"

using namespace plankton;

constexpr std::size_t PROOF_ABORT_AFTER = 7;


inline void PerformFixpointIteration(const Program& object, Prover& prover) {
    for (const auto& function : object.apiFunctions) {
        prover.HandleFunction(*function);
    }
    for (const auto& function : object.maintenanceFunctions) {
        prover.HandleFunction(*function);
    }
}

void Prover::HandleProgram(const Program& object) {
    Callback(&ProofListener::BeforeHandleProgram, std::cref(object));
    for (std::size_t counter = 0; counter < PROOF_ABORT_AFTER; ++counter) {
        Callback(&ProofListener::BeforeHandleProgramFixpointIteration, std::cref(object), counter);
        PerformFixpointIteration(object, *this);
        if (!ConsolidateDiscoveredInterference()) {
            CallbackReverse(&ProofListener::BeforeHandleProgram, std::cref(object));
            return;
        }
        CallbackReverse(&ProofListener::AfterHandleProgramFixpointIteration, std::cref(object), counter);
    }
    throw std::logic_error("Aborting: proof does not seem to stabilize."); // TODO: remove / better error handling
}
