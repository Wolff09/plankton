#include "engine/proof.hpp"

#include "programs/util.hpp"
#include "util/shortcuts.hpp"
#include "util/log.hpp"

using namespace plankton;


template<typename C>
static inline std::function<PostImage(std::unique_ptr<Annotation>)> MakePostTransformer(const C& cmd, const Solver& solver, Timer& timer) {
    return [&cmd, &solver, &timer](auto annotation){
        auto measurement = timer.Measure();
        return solver.Post(std::move(annotation), cmd);
    };
}


void ProofGenerator::Visit(const Skip& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    /* do nothing */
}

void ProofGenerator::Visit(const Break& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    DEBUG(" BREAKING with (" << current.size() << ") ")
    DEBUG_FOREACH(current, [](const auto& elem){ DEBUG(*elem) })
    DEBUG(std::endl)
    MoveInto(std::move(current), breaking);
    current.clear();
}

void ProofGenerator::Visit(const Fail& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    for (const auto& annotation : current) {
        if (solver.IsUnsatisfiable(*annotation)) continue;
        throw std::logic_error("Program potentially fails / violates an assertion.");
    }
    current.clear();
}

void ProofGenerator::Visit(const Assume& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const AssertFlow& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const AssumeFlow& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const AcquireLock &cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const ReleaseLock &cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const Malloc& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const VariableAssignment& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const MemoryWrite& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const UpdateStub& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    ApplyTransformer(MakePostTransformer(cmd, solver, timePost));
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const Suggestion& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    switch (cmd.hint) {
        case Suggestion::JOIN: JoinCurrent(); break;
    }
    MakeInterferenceStable(cmd);
}

void ProofGenerator::Visit(const Return& cmd) {
    INFO(infoPrefix << "Post for '" << cmd << "'." << INFO_SIZE << std::endl)
    DEBUG(" RETURNING " << cmd << " with (" << current.size() << ") ")
    DEBUG_FOREACH(current, [](const auto& elem){ DEBUG(*elem) })
    DEBUG(std::endl)
    // pointer accessibility is checked when returned values/variables are used
    for (auto& annotation : current) returning.emplace_back(std::move(annotation), &cmd);
    current.clear();
}
