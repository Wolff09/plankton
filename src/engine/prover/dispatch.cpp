#include "engine/prover.hpp"

using namespace plankton;


struct Dispatcher : public ProgramVisitor {
    Prover& prover;
    explicit Dispatcher(Prover& prover) : prover(prover) {}
    void Visit(const VariableExpression&) override { assert(false); }
    void Visit(const TrueValue&) override { assert(false); }
    void Visit(const FalseValue&) override { assert(false); }
    void Visit(const MinValue&) override { assert(false); }
    void Visit(const MaxValue&) override { assert(false); }
    void Visit(const NullValue&) override { assert(false); }
    void Visit(const Dereference&) override { assert(false); }
    void Visit(const BinaryExpression&) override { assert(false); }
    void Visit(const Skip& object) override { prover.HandleSkip(object); }
    void Visit(const Fail& object) override { prover.HandleFail(object); }
    void Visit(const Break& object) override { prover.HandleBreak(object); }
    void Visit(const Return& object) override { prover.HandleReturn(object); }
    void Visit(const Assume& object) override { prover.HandleAssume(object); }
    void Visit(const Malloc& object) override { prover.HandleMalloc(object); }
    void Visit(const Macro& object) override { prover.HandleMacro(object); }
    void Visit(const VariableAssignment& object) override { prover.HandleVariableAssignment(object); }
    void Visit(const MemoryWrite& object) override { prover.HandleMemoryWrite(object); }
    void Visit(const AcquireLock& object) override { prover.HandleAcquireLock(object); }
    void Visit(const ReleaseLock& object) override { prover.HandleReleaseLock(object); }
    void Visit(const Scope& object) override { prover.HandleScope(object); }
    void Visit(const Atomic& object) override { prover.HandleAtomic(object); }
    void Visit(const Sequence& object) override { prover.HandleSequence(object); }
    void Visit(const UnconditionalLoop& object) override { prover.HandleLoop(object); }
    void Visit(const Choice& object) override { prover.HandleChoice(object); }
    void Visit(const Function& object) override { prover.HandleFunction(object); }
    void Visit(const Program& object) override { prover.HandleProgram(object); }
};

void Prover::Handle(const AstNode& object) {
    Dispatcher dispatcher(*this);
    object.Accept(dispatcher);
}