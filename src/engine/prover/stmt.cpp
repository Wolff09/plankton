#include "engine/prover.hpp"

#include "logics/util.hpp"
#include "util/shortcuts.hpp"

using namespace plankton;


constexpr std::size_t LOOP_ABORT_AFTER = 23;

#define BEFORE(X) Callback(&ProofListener::X, std::cref(stmt));
#define AFTER(X) CallbackReverse(&ProofListener::X, std::cref(stmt));


void Prover::HandleSequence(const Sequence& stmt) {
    BEFORE(BeforeHandleSequence)
    Handle(*stmt.first);
    Handle(*stmt.second);
    AFTER(AfterHandleSequence)
}

void Prover::HandleScope(const Scope& stmt) {
    BEFORE(BeforeHandleScope)
    Transform([this,&stmt](auto annotation){ return solver->PostEnter(std::move(annotation), stmt); });
    Handle(*stmt.body);
    Transform([this,&stmt](auto annotation){ return solver->PostLeave(std::move(annotation), stmt); });
    AFTER(AfterHandleScope)
}

void Prover::HandleAtomic(const Atomic& stmt) {
    BEFORE(BeforeHandleAtomic)
    Handle(*stmt.body);
    ApplyInterference(stmt);
    AFTER(AfterHandleAtomic)
}

void Prover::HandleChoice(const Choice& stmt) {
    if (stmt.branches.empty()) return;
    BEFORE(BeforeHandleChoice)
    
    auto pre = std::move(current);
    decltype(pre) post;
    current.clear();
    
    for (const auto& branch : stmt.branches) {
        current = plankton::CopyAll(pre);
        Handle(*branch);
        MoveInto(std::move(current), post);
    }
    
    current = std::move(post);
    AFTER(AfterHandleChoice)
}

void Prover::HandleLoop(const UnconditionalLoop& stmt) {
    if (current.empty()) return;
    BEFORE(BeforeHandleLoop)
    BEFORE(BeforeHandleLoopBody)

    auto implies = [this](const auto& premises, const auto& conclusions) {
        if (conclusions.empty()) return true;
        return plankton::All(premises, [this,&conclusions](const auto& premise){
            return plankton::Any(conclusions, [this,&premise](const auto& conclusion) {
                return solver->Implies(*premise, *conclusion);
            });
        });
    };

    // prepare
    std::size_t counter = 0;
    auto outerBreaking = std::move(breaking);
    breaking.clear();

    // peel first loop iteration
    Callback(&ProofListener::BeforeHandleLoopIteration, std::cref(stmt), counter);
    decltype(current) postLoop;
    Handle(*stmt.body);
    CallbackReverse(&ProofListener::AfterHandleLoopIteration, std::cref(stmt), counter);
    MoveInto(std::move(breaking), postLoop);
    breaking.clear();

    // prepare more
    auto outerInterference = std::move(discoveredInterference);
    auto outerReturning = std::move(returning);
    discoveredInterference.clear();
    returning.clear();

    // looping until fixed point
    Join();
    while (true) {
        if (counter++ > LOOP_ABORT_AFTER) throw std::logic_error("Aborting: loop does not seem to stabilize."); // TODO: remove / better error handling
        discoveredInterference.clear();
        returning.clear();
        breaking.clear();

        Callback(&ProofListener::BeforeHandleLoopIteration, std::cref(stmt), counter);
        auto join = CopyAll(current);
        Handle(*stmt.body);
        MoveInto(CopyAll(join), current);
        CallbackReverse(&ProofListener::AfterHandleLoopIteration, std::cref(stmt), counter);
        Join();

        if (implies(current, join)) {
            MoveInto(std::move(join), postLoop);
            break;
        }
    }

    // post loop
    AFTER(AfterHandleLoopBody)
    current = std::move(postLoop);
    MoveInto(std::move(breaking), current);

    breaking = std::move(outerBreaking);
    MoveInto(std::move(outerReturning), returning);
    MoveInto(std::move(outerInterference), discoveredInterference);

    AFTER(AfterHandleLoop)
}
