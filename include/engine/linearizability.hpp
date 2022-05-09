#pragma once
#ifndef PLANKTON_ENGINE_LINEARIZABILITY_HPP
#define PLANKTON_ENGINE_LINEARIZABILITY_HPP

#include "programs/ast.hpp"
#include "engine/config.hpp"
#include "engine/prover.hpp"

namespace plankton {

    bool IsLinearizable(const Program& program, Prover& prover) {
        prover.GenerateProof(program);
        return true;
    }

    bool IsLinearizable(const Program& program, const SolverConfig& config) {
        auto solver = std::make_unique<Solver>(program, config);
        auto prover = plankton::MakeDefaultProver(std::move(solver), program);
        return IsLinearizable(program, *prover);
    }

} // namespace engine

#endif //PLANKTON_ENGINE_LINEARIZABILITY_HPP