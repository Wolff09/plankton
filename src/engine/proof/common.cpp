#include "engine/proof.hpp"

#include "programs/util.hpp"
#include "util/shortcuts.hpp"
#include "util/log.hpp"

using namespace plankton;


ProofGenerator::ProofGenerator(const Program& program, const SolverConfig& config, EngineSetup setup)
        : program(program), solver(program, config), setup(setup), insideAtomic(false),
          timePost("TIME Post"), timeJoin("TIME Join"), timeInterference("TIME Interference"),
          timePastImprove("TIME Past improve"), timePastReduce("TIME Past reduce"),
          timeFutureImprove("TIME Future improve"), timeFutureReduce("TIME Future reduce") {
    futureSuggestions = plankton::SuggestFutures(program);
}

void ProofGenerator::LeaveAllNestedScopes(const AstNode& node) {
    struct : public ProgramListener {
        std::deque<const Scope*> result;
        void Enter(const Scope& object) override { result.push_back(&object); }
    } collector;
    node.Accept(collector);

    for (auto it = collector.result.rbegin(); it != collector.result.rend(); ++it) {
        for (auto& annotation : current) {
            annotation = solver.PostLeave(std::move(annotation), **it);
        }
    }
}

void ProofGenerator::PruneCurrent() {
    // TODO: keep or remove?

    for (auto& annotation : current) {
        if (!solver.IsUnsatisfiable(*annotation)) continue;
        annotation.reset(nullptr);
    }
    for (const auto& annotation : current) {
        if (!annotation) continue;
        for (auto& otherAnnotation : current) {
            if (!otherAnnotation) continue;
            if (annotation.get() == otherAnnotation.get()) continue;
            if (!solver.Implies(*otherAnnotation, *annotation)) continue;
            otherAnnotation.reset(nullptr);
        }
    }
    plankton::RemoveIf(current, [](const auto& elem) { return !elem; });
}

inline bool SameReturns(const Return* command, const Return* other) {
    if (command == other) return true;
    if (!command || !other) return false;
    if (command->expressions.size() != other->expressions.size()) return false;
    for (std::size_t index = 0; index < command->expressions.size(); ++index) {
        if (!plankton::SyntacticalEqual(*command->expressions.at(index), *other->expressions.at(index))) return false;
    }
    return true;
}

void ProofGenerator::PruneReturning() {
    // TODO: keep or remove?

    // TODO: avoid code duplication
    for (auto& pair : returning) {
        if (!solver.IsUnsatisfiable(*pair.first)) continue;
        pair.first.reset(nullptr);
    }
    for (const auto& [annotation, command] : returning) {
        if (!annotation) continue;
        for (auto& [otherAnnotation, otherCommand] : returning) {
            if (!otherAnnotation) continue;
            if (annotation.get() == otherAnnotation.get()) continue;
            if (!SameReturns(command, otherCommand)) continue;
            if (!solver.Implies(*otherAnnotation, *annotation)) continue;
            otherAnnotation.reset(nullptr);
        }
    }
    plankton::RemoveIf(returning, [](const auto& elem) { return !elem.first; });
}

void ProofGenerator::ApplyTransformer(const std::function<std::unique_ptr<Annotation>(std::unique_ptr<Annotation>)>& transformer) {
    if (current.empty()) return;
    for (auto& annotation : current) {
        annotation = transformer(std::move(annotation));
    }
}

void ProofGenerator::ApplyTransformer(const std::function<PostImage(std::unique_ptr<Annotation>)>& transformer) {
    if (current.empty()) return;
    decltype(current) newCurrent;
    for (auto& annotation : current) {
        auto postImage = transformer(std::move(annotation));
        MoveInto(std::move(postImage.annotations), newCurrent);
        AddNewInterference(std::move(postImage.effects));
    }
    current = std::move(newCurrent);
}

void ProofGenerator::AddNewInterference(std::deque<std::unique_ptr<HeapEffect>> effects) {
    MoveInto(effects, newInterference);
}

bool ProofGenerator::ConsolidateNewInterference() {
    INFO(infoPrefix << "Checking for new effects. (" << newInterference.size() << ") " << std::endl)
    auto result = solver.AddInterference(std::move(newInterference));
    newInterference.clear();
    return result;
}

void ProofGenerator::MakeInterferenceStable(const Statement& after) {
    INFO(infoPrefix << "Applying interference." << INFO_SIZE << std::endl)
    if (insideAtomic) return;
    if (current.empty()) return;
    if (plankton::IsRightMover(after)) return;
    ApplyTransformer([this](auto annotation){
        // TODO: improve future?
        {
            auto measure = timePastImprove.Measure();
            annotation = solver.ImprovePast(std::move(annotation), setup.improvePastIncreasedPrecisionForStability);
        }
        {
            auto measure = timeInterference.Measure();
            annotation = solver.MakeInterferenceStable(std::move(annotation));
        }
        {
            auto measure = timePastReduce.Measure();
            annotation = solver.ReducePast(std::move(annotation));
        }
        return annotation;
    });
}

void ProofGenerator::JoinCurrent() {
    if (current.empty()) return;
    INFO(infoPrefix << "Preparing Join." << INFO_SIZE << std::endl)

    PruneCurrent();
    ImproveCurrentTime();
    ReduceCurrentTime();

    INFO(infoPrefix << "Joining." << INFO_SIZE << std::endl)
    {
        auto measure = timeJoin.Measure();
        auto join = solver.Join(std::move(current));
        current.clear();
        current.push_back(std::move(join));
    }

    ReduceCurrentTime();
}
void ProofGenerator::ImproveCurrentTime() {
    INFO(infoPrefix << "Improving time predicates." << INFO_SIZE << std::endl)
    ApplyTransformer([this](auto annotation) {
        auto measure = timePastImprove.Measure();
        return solver.ImprovePast(std::move(annotation), setup.improvePastIncreasedPrecisionForAnnotations);
    });
    for (const auto& future : futureSuggestions) {
        ApplyTransformer([this, &future](auto annotation) {
            auto measure = timeFutureImprove.Measure();
            return solver.ImproveFuture(std::move(annotation), *future);
        });
    }
    for (const auto& future : futureSuggestions) { // repeat because Z3 hates us...
        ApplyTransformer([this, &future](auto annotation) {
            auto measure = timeFutureImprove.Measure();
            return solver.ImproveFuture(std::move(annotation), *future);
        });
    }
    //for (const auto& future : futureSuggestions) { // repeat because Z3 really hates us... why?
    //    ApplyTransformer([this, &future](auto annotation) {
    //        auto measure = timeFutureImprove.Measure();
    //        return solver.ImproveFuture(std::move(annotation), *future);
    //    });
    //}
}

void ProofGenerator::ReduceCurrentTime() {
    INFO(infoPrefix << "Minimizing time predicates." << INFO_SIZE << std::endl)
    ApplyTransformer([this](auto annotation){
        {
            auto measure = timePastReduce.Measure();
            annotation = solver.ReducePast(std::move(annotation));
        }
        {
            auto measure = timeFutureReduce.Measure();
            annotation = solver.ReduceFuture(std::move(annotation));
        }
        return annotation;
    });
}
