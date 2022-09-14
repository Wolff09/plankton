#pragma once
#ifndef PLANKTON_ENGINE_SETUP_HPP
#define PLANKTON_ENGINE_SETUP_HPP


namespace plankton {

    struct EngineSetup {
        // TODO: configurable join
        // TODO: configurable extension policies in various places

        // loops
        bool loopJoinUntilFixpoint = true;
        bool loopJoinPost = true;
        std::size_t loopMaxIterations = 23;

        // macros
        bool macrosTabulateInvocations = true;

        // proof
        std::size_t proofMaxIterations = 7;

        explicit EngineSetup() = default;
    };

}

#endif //PLANKTON_ENGINE_SETUP_HPP
