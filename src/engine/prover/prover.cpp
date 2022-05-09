#include "engine/prover.hpp"

#include "util/shortcuts.hpp"

using namespace plankton;


Prover::Prover(std::unique_ptr<Solver> solver_, std::deque<std::unique_ptr<FutureSuggestion>> futureSuggestions_)
        : solver(std::move(solver_)), futureSuggestions(std::move(futureSuggestions_)) {
    assert(NonNull(solver));
    assert(AllNonNull(futureSuggestions));
}

Prover::Prover(std::unique_ptr<Solver> solver) : Prover(std::move(solver), {}) {}


void Prover::Transform(const std::function<std::unique_ptr<Annotation>(std::unique_ptr<Annotation>)>& transformer) {
    if (current.empty()) return;
    for (auto& annotation : current) {
        annotation = transformer(std::move(annotation));
    }
}

void Prover::Transform(const std::function<PostImage(std::unique_ptr<Annotation>)>& transformer) {
    if (current.empty()) return;
    decltype(current) newCurrent;
    for (auto& annotation : current) {
        auto postImage = transformer(std::move(annotation));
        MoveInto(std::move(postImage.annotations), newCurrent);
        AddDiscoveredInterference(std::move(postImage.effects));
    }
    current = std::move(newCurrent);
}


bool Prover::NeedsInterference(const Statement& object) {
    // return !insideAtomic && !isRightMover && !allSuccessorsAreLeftMovers;
    throw std::logic_error("not yet implemented");
}

void Prover::ApplyInterference(const Statement& object) {
    if (current.empty()) return;
    if (!NeedsInterference(object)) return;
}

void Prover::ApplyInterference() {
    if (current.empty()) return;
    Callback(&ProofListener::BeforeApplyInterference);
    Transform([this](auto annotation){
        return solver->MakeInterferenceStable(std::move(annotation));
    });
    CallbackReverse(&ProofListener::AfterApplyInterference);
}

void Prover::AddDiscoveredInterference(std::deque<std::unique_ptr<HeapEffect>> effects) {
    Callback(&ProofListener::BeforeAddDiscoveredInterference);
    MoveInto(effects, discoveredInterference);
    CallbackReverse(&ProofListener::AfterAddDiscoveredInterference);
}

bool Prover::ConsolidateDiscoveredInterference() {
    Callback(&ProofListener::BeforeConsolidateDiscoveredInterference);
    auto result = solver->AddInterference(std::move(discoveredInterference));
    discoveredInterference.clear();
    CallbackReverse(&ProofListener::AfterConsolidateDiscoveredInterference, result);
    return result;
}

void Prover::ImprovePast() {
    Callback(&ProofListener::BeforeImprovePast);
    Transform([this](auto annotation) {
        return solver->ImprovePast(std::move(annotation));
    });
    CallbackReverse(&ProofListener::AfterImprovePast);
}

void Prover::ImproveFuture() {
    Callback(&ProofListener::BeforeImproveFuture);
    for (const auto& future : futureSuggestions) {
        Transform([this, &future](auto annotation) {
            return solver->ImproveFuture(std::move(annotation), *future);
        });
    }
    CallbackReverse(&ProofListener::AfterImproveFuture);
}

void Prover::Prune() {
    Callback(&ProofListener::BeforePrune);
    // remove unsatisfiable
    for (auto& annotation : current) {
        // TODO: solve in one shot
        if (!solver->IsUnsatisfiable(*annotation)) continue;
        annotation.reset(nullptr);
    }
    // remove subsumed
    for (const auto& annotation : current) {
        if (!annotation) continue;
        for (auto& otherAnnotation : current) {
            if (!otherAnnotation) continue;
            if (annotation.get() == otherAnnotation.get()) continue;
            if (!solver->Implies(*otherAnnotation, *annotation)) continue;
            otherAnnotation.reset(nullptr);
        }
    }
    plankton::RemoveIf(current, [](const auto& elem) { return !elem; });
    CallbackReverse(&ProofListener::AfterPrune);
}

void Prover::ReducePast() {
    Callback(&ProofListener::BeforeReducePast);
    Transform([this](auto annotation){
        return solver->ReducePast(std::move(annotation));
    });
    CallbackReverse(&ProofListener::AfterReducePast);
}

void Prover::ReduceFuture() {
    Callback(&ProofListener::BeforeReduceFuture);
    Transform([this](auto annotation){
        return solver->ReduceFuture(std::move(annotation));
    });
    CallbackReverse(&ProofListener::AfterReduceFuture);
}

void Prover::Join() {
    if (current.empty()) return;
    Callback(&ProofListener::BeforeJoin);
    auto join = solver->Join(std::move(current));
    current.clear();
    current.push_back(std::move(join));
    CallbackReverse(&ProofListener::AfterJoin);
}

void Prover::Widen() {
    if (current.empty()) return;
    Callback(&ProofListener::BeforeWiden);
    Transform([this](auto annotation){
        return solver->Widen(std::move(annotation));
    });
    CallbackReverse(&ProofListener::AfterWiden);
}
