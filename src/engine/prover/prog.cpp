#include "engine/prover.hpp"

#include "programs/util.hpp"

using namespace plankton;


void Prover::HandleProgram(const Program& object) {
    throw std::logic_error("not yet implemented");
}

inline void HandleFunctionBody(const Function& func, Prover& prover) {
    prover.HandleScope(*func.body);

    // check for missing return
    if (!prover.current.empty() && !plankton::IsVoid(func)) {
        throw std::logic_error("Detected non-returning path through non-void function '" + func.name + "'."); // TODO: better error handling
    }

    // patch void return
    static Return returnVoid;
    prover.HandleReturn(returnVoid);
    assert(prover.current.empty());
    prover.current.clear();
}

void Prover::HandleFunction(const Function& object) {
    switch (object.kind) {
        case Function::API: HandleApiFunction(object); break;
        case Function::HELP: HandleMaintenanceFunction(object); break;
        case Function::MACRO: HandleMacroFunction(object); break;
        case Function::INIT: break; // TODO: handle init
    }
}

void Prover::HandleMacroFunction(const Function& object) {
    HandleFunctionBody(object, *this);
}

void Prover::HandleMaintenanceFunction(const Function& object) {
    throw std::logic_error("not yet implemented");
}

void Prover::HandleApiFunction(const Function& object) {
    throw std::logic_error("not yet implemented");
}