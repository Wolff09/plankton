#include "engine/prover.hpp"

#include "programs/util.hpp"
#include "engine/util.hpp"
#include "util/shortcuts.hpp"

using namespace plankton;


inline const Scope& GetExitingScope(const Break& cmd) {
    // return { cmd.parentScope.get_or(nullptr); }
    throw std::logic_error("not yet implemented");
}

void Prover::HandleBreak(const Break& cmd) {
    Callback(&ProofListener::BeforeHandleBreak, std::cref(cmd));
    auto& scope = GetExitingScope(cmd);
    Transform([this, &scope](auto annotation){
        return solver->PostLeave(std::move(annotation), scope);
    });
    MoveInto(std::move(current), breaking);
    current.clear();
    CallbackReverse(&ProofListener::AfterHandleBreak, std::cref(cmd));
}

Prover::ReturnValues GetReturnValues(const Annotation& annotation, const Return& cmd) {
    auto result = MakeVector<Prover::ReturnValues::value_type>(cmd.expressions.size());
    for (const auto& expr : cmd.expressions) {
        auto value = plankton::TryMakeSymbolic(*expr, *annotation.now);
        if (!value) throw std::logic_error("Cannot determine return value for '" + plankton::ToString(*expr) + "' in '" + plankton::ToString(cmd) + "'."); // TODO: better error handling
        result.push_back(std::move(value));
    }
    return result;
}

inline std::deque<const Scope*> GetExitingScopes(const Return& cmd) {
    // std::deque<const Scope*> result;
    // for (const Scope* scope = cmd.parentScope.get_or(nullptr); scope != nullptr; scope = scope.parentScope.get_or(nullptr)) {
    //     result.push_back(scope);
    // }
    // return result;
    throw std::logic_error("not yet implemented");
}

void Prover::HandleReturn(const Return& cmd) {
    if (current.empty()) return;
    Callback(&ProofListener::BeforeHandleReturn, std::cref(cmd));
    auto scopes = GetExitingScopes(cmd);
    for (auto& annotation : current) {
        auto result = GetReturnValues(*annotation, cmd);
        for (const auto* scope : scopes) {
            annotation = solver->PostLeave(std::move(annotation), *scope);
        }
        returning.emplace_back(std::move(annotation), std::move(result));
    }
    current.clear();
    CallbackReverse(&ProofListener::AfterHandleReturn, std::cref(cmd));
}


void Prover::HandleFail(const Fail& cmd) {
    Callback(&ProofListener::BeforeHandleFail, std::cref(cmd));
    for (const auto& annotation : current) {
        if (solver->IsUnsatisfiable(*annotation)) continue;
        throw std::logic_error("Program potentially fails / violates an assertion.");
    }
    current.clear();
    CallbackReverse(&ProofListener::AfterHandleFail, std::cref(cmd));
}


void Prover::HandleSkip(const Skip& cmd) {
    Callback(&ProofListener::BeforeHandleSkip, std::cref(cmd));
    CallbackReverse(&ProofListener::AfterHandleSkip, std::cref(cmd));
}

#define POST_CMD(before, after) { \
    Callback(&ProofListener::before, std::cref(cmd)); \
    Transform([this, &cmd](auto annotation){ \
        return solver->Post(std::move(annotation), cmd); \
    }); \
    CallbackReverse(&ProofListener::after, std::cref(cmd)); \
    ApplyInterference(cmd); \
}

void Prover::HandleAssume(const Assume& cmd) {
    POST_CMD(BeforeHandleAssume, AfterHandleAssume)
}

void Prover::HandleAcquireLock(const AcquireLock &cmd) {
    POST_CMD(BeforeHandleAcquireLock, AfterHandleAcquireLock)
}

void Prover::HandleReleaseLock(const ReleaseLock &cmd) {
    POST_CMD(BeforeHandleReleaseLock, AfterHandleReleaseLock)
}

void Prover::HandleMalloc(const Malloc& cmd) {
    POST_CMD(BeforeHandleMalloc, AfterHandleMalloc)
}

void Prover::HandleVariableAssignment(const VariableAssignment& cmd) {
    POST_CMD(BeforeHandleVariableAssignment, AfterHandleVariableAssignment)
}

void Prover::HandleMemoryWrite(const MemoryWrite& cmd) {
    POST_CMD(BeforeHandleMemoryWrite, AfterHandleMemoryWrite)
}
