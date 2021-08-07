#include "programs/visitors.hpp"

#include "programs/ast.hpp"
#include "util/shortcuts.hpp"

using namespace plankton;


struct ProgramVisitorNotImplementedException : public ExceptionWithMessage {
    explicit ProgramVisitorNotImplementedException(std::string base, std::string arg) : ExceptionWithMessage(
            "Call to unimplemented visitor member function with argument type '" + std::move(arg) +
            "' of base class '" + std::move(base) + "'.") {}
};

#define COMPLAIN(B,T) throw ProgramVisitorNotImplementedException(#B,#T)

void BaseProgramVisitor::Visit(const VariableExpression& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const TrueValue& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const FalseValue& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const MinValue& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const MaxValue& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const NullValue& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Dereference& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const BinaryExpression& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Skip& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Break& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Return& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Assume& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Assert& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Malloc& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Macro& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const VariableAssignment& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const MemoryRead& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const MemoryWrite& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Scope& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Atomic& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Sequence& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const UnconditionalLoop& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Choice& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Function& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }
void BaseProgramVisitor::Visit(const Program& /*object*/) { COMPLAIN(BaseProgramVisitor, const ProgramVisitorNotImplementedException&); }

void MutableBaseProgramVisitor::Visit(VariableExpression& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, VariableExpression&); }
void MutableBaseProgramVisitor::Visit(TrueValue& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, TrueValue&); }
void MutableBaseProgramVisitor::Visit(FalseValue& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, FalseValue&); }
void MutableBaseProgramVisitor::Visit(MinValue& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, MinValue&); }
void MutableBaseProgramVisitor::Visit(MaxValue& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, MaxValue&); }
void MutableBaseProgramVisitor::Visit(NullValue& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, NullValue&); }
void MutableBaseProgramVisitor::Visit(Dereference& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Dereference&); }
void MutableBaseProgramVisitor::Visit(BinaryExpression& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, BinaryExpression&); }
void MutableBaseProgramVisitor::Visit(Skip& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Skip&); }
void MutableBaseProgramVisitor::Visit(Break& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Break&); }
void MutableBaseProgramVisitor::Visit(Return& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Return&); }
void MutableBaseProgramVisitor::Visit(Assume& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Assume&); }
void MutableBaseProgramVisitor::Visit(Assert& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Assert&); }
void MutableBaseProgramVisitor::Visit(Malloc& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Assert&); }
void MutableBaseProgramVisitor::Visit(Macro& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Assert&); }
void MutableBaseProgramVisitor::Visit(VariableAssignment& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, VariableAssignment&); }
void MutableBaseProgramVisitor::Visit(MemoryRead& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, MemoryRead&); }
void MutableBaseProgramVisitor::Visit(MemoryWrite& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, MemoryWrite&); }
void MutableBaseProgramVisitor::Visit(Scope& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Scope&); }
void MutableBaseProgramVisitor::Visit(Atomic& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Atomic&); }
void MutableBaseProgramVisitor::Visit(Sequence& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Sequence&); }
void MutableBaseProgramVisitor::Visit(UnconditionalLoop& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, UnconditionalLoop&); }
void MutableBaseProgramVisitor::Visit(Choice& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Choice&); }
void MutableBaseProgramVisitor::Visit(Function& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Function&); }
void MutableBaseProgramVisitor::Visit(Program& /*object*/) { COMPLAIN(MutableBaseProgramVisitor, Program&); }


void DefaultProgramVisitor::Visit(const VariableExpression& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const TrueValue& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const FalseValue& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const MinValue& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const MaxValue& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const NullValue& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Dereference& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const BinaryExpression& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Skip& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Break& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Return& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Assume& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Assert& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Malloc& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Macro& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const VariableAssignment& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const MemoryRead& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const MemoryWrite& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Scope& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Atomic& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Sequence& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const UnconditionalLoop& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Choice& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Function& /*object*/) { /* do nothing */ }
void DefaultProgramVisitor::Visit(const Program& /*object*/) { /* do nothing */ }

void MutableDefaultProgramVisitor::Visit(VariableExpression& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(TrueValue& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(FalseValue& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(MinValue& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(MaxValue& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(NullValue& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Dereference& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(BinaryExpression& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Skip& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Break& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Return& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Assume& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Assert& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Malloc& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Macro& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(VariableAssignment& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(MemoryRead& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(MemoryWrite& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Scope& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Atomic& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Sequence& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(UnconditionalLoop& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Choice& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Function& /*object*/) { /* do nothing */ }
void MutableDefaultProgramVisitor::Visit(Program& /*object*/) { /* do nothing */ }


void ProgramListener::Enter(const VariableDeclaration& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const VariableExpression& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const TrueValue& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const FalseValue& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const MinValue& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const MaxValue& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const NullValue& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Dereference& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const BinaryExpression& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Skip& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Break& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Return& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Assume& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Assert& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Malloc& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Macro& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const VariableAssignment& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const MemoryRead& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const MemoryWrite& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Scope& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Atomic& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Sequence& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const UnconditionalLoop& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Choice& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Function& /*object*/) { /* do nothing */ }
void ProgramListener::Enter(const Program& /*object*/) { /* do nothing */ }

void MutableProgramListener::Enter(const VariableDeclaration& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(VariableDeclaration& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(VariableExpression& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(TrueValue& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(FalseValue& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(MinValue& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(MaxValue& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(NullValue& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Dereference& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(BinaryExpression& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Skip& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Break& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Return& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Assume& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Assert& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Malloc& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Macro& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(VariableAssignment& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(MemoryRead& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(MemoryWrite& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Scope& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Atomic& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Sequence& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(UnconditionalLoop& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Choice& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Function& /*object*/) { /* do nothing */ }
void MutableProgramListener::Enter(Program& /*object*/) { /* do nothing */ }

void ProgramListener::Visit(const TrueValue& object) { Enter(object); }
void ProgramListener::Visit(const FalseValue& object) { Enter(object); }
void ProgramListener::Visit(const MinValue& object) { Enter(object); }
void ProgramListener::Visit(const MaxValue& object) { Enter(object); }
void ProgramListener::Visit(const NullValue& object) { Enter(object); }
void ProgramListener::Visit(const Skip& object) { Enter(object); }
void ProgramListener::Visit(const Break& object) { Enter(object); }
void ProgramListener::Visit(const VariableExpression& object) {
    Enter(object);
    Enter(object.Decl());
}
void ProgramListener::Visit(const Dereference& object) {
    Enter(object);
    object.variable->Accept(*this);
}
void ProgramListener::Visit(const BinaryExpression& object) {
    Enter(object);
    object.lhs->Accept(*this);
    object.rhs->Accept(*this);
}
void ProgramListener::Visit(const Return& object) {
    Enter(object);
    for (const auto& elem : object.expressions) elem->Accept(*this);
}
void ProgramListener::Visit(const Assume& object) {
    Enter(object);
    object.condition->Accept(*this);
}
void ProgramListener::Visit(const Assert& object) {
    Enter(object);
    object.condition->Accept(*this);
}
void ProgramListener::Visit(const Malloc& object) {
    Enter(object);
    object.lhs->Accept(*this);
}
void ProgramListener::Visit(const Macro& object) {
    Enter(object);
    for (const auto& elem : object.lhs) elem->Accept(*this);
    for (const auto& elem : object.arguments) elem->Accept(*this);
}
void ProgramListener::Visit(const VariableAssignment& object) {
    Enter(object);
    for (const auto& elem : object.lhs) elem->Accept(*this);
    for (const auto& elem : object.rhs) elem->Accept(*this);
}
void ProgramListener::Visit(const MemoryRead& object) {
    Enter(object);
    for (const auto& elem : object.lhs) elem->Accept(*this);
    for (const auto& elem : object.rhs) elem->Accept(*this);
}
void ProgramListener::Visit(const MemoryWrite& object) {
    Enter(object);
    for (const auto& elem : object.lhs) elem->Accept(*this);
    for (const auto& elem : object.rhs) elem->Accept(*this);
}
void ProgramListener::Visit(const Scope& object) {
    Enter(object);
    for (const auto& elem : object.variables) Enter(*elem);
    object.body->Accept(*this);
}
void ProgramListener::Visit(const Atomic& object) {
    Enter(object);
    object.body->Accept(*this);
}
void ProgramListener::Visit(const Sequence& object) {
    Enter(object);
    object.first->Accept(*this);
    object.second->Accept(*this);
}
void ProgramListener::Visit(const UnconditionalLoop& object) {
    Enter(object);
    object.body->Accept(*this);
}
void ProgramListener::Visit(const Choice& object) {
    Enter(object);
    for (const auto& elem : object.branches) elem->Accept(*this);
}
void ProgramListener::Visit(const Function& object) {
    Enter(object);
    for (const auto& elem : object.parameters) Enter(*elem);
    object.body->Accept(*this);
}
void ProgramListener::Visit(const Program& object) {
    Enter(object);
    for (const auto& elem : object.variables) Enter(*elem);
    object.initializer->Accept(*this);
    for (const auto& elem : object.macroFunctions) elem->Accept(*this);
    for (const auto& elem : object.apiFunctions) elem->Accept(*this);
}

void MutableProgramListener::Visit(TrueValue& object) { Enter(object); }
void MutableProgramListener::Visit(FalseValue& object) { Enter(object); }
void MutableProgramListener::Visit(MinValue& object) { Enter(object); }
void MutableProgramListener::Visit(MaxValue& object) { Enter(object); }
void MutableProgramListener::Visit(NullValue& object) { Enter(object); }
void MutableProgramListener::Visit(Skip& object) { Enter(object); }
void MutableProgramListener::Visit(Break& object) { Enter(object); }
void MutableProgramListener::Visit(VariableExpression& object) {
    Enter(object);
    Enter(object.Decl());
}
void MutableProgramListener::Visit(Dereference& object) {
    Enter(object);
    object.variable->Accept(*this);
}
void MutableProgramListener::Visit(BinaryExpression& object) {
    Enter(object);
    object.lhs->Accept(*this);
    object.rhs->Accept(*this);
}
void MutableProgramListener::Visit(Return& object) {
    Enter(object);
    for (const auto& elem : object.expressions) elem->Accept(*this);
}
void MutableProgramListener::Visit(Assume& object) {
    Enter(object);
    object.condition->Accept(*this);
}
void MutableProgramListener::Visit(Assert& object) {
    Enter(object);
    object.condition->Accept(*this);
}
void MutableProgramListener::Visit(Malloc& object) {
    Enter(object);
    object.lhs->Accept(*this);
}
void MutableProgramListener::Visit(Macro& object) {
    Enter(object);
    for (auto& elem : object.lhs) elem->Accept(*this);
    for (auto& elem : object.arguments) elem->Accept(*this);
}
void MutableProgramListener::Visit(VariableAssignment& object) {
    Enter(object);
    for (const auto& elem : object.lhs) elem->Accept(*this);
    for (const auto& elem : object.rhs) elem->Accept(*this);
}
void MutableProgramListener::Visit(MemoryRead& object) {
    Enter(object);
    for (const auto& elem : object.lhs) elem->Accept(*this);
    for (const auto& elem : object.rhs) elem->Accept(*this);
}
void MutableProgramListener::Visit(MemoryWrite& object) {
    Enter(object);
    for (const auto& elem : object.lhs) elem->Accept(*this);
    for (const auto& elem : object.rhs) elem->Accept(*this);
}
void MutableProgramListener::Visit(Scope& object) {
    Enter(object);
    for (const auto& elem : object.variables) Enter(*elem);
    object.body->Accept(*this);
}
void MutableProgramListener::Visit(Atomic& object) {
    Enter(object);
    object.body->Accept(*this);
}
void MutableProgramListener::Visit(Sequence& object) {
    Enter(object);
    object.first->Accept(*this);
    object.second->Accept(*this);
}
void MutableProgramListener::Visit(UnconditionalLoop& object) {
    Enter(object);
    object.body->Accept(*this);
}
void MutableProgramListener::Visit(Choice& object) {
    Enter(object);
    for (const auto& elem : object.branches) elem->Accept(*this);
}
void MutableProgramListener::Visit(Function& object) {
    Enter(object);
    for (const auto& elem : object.parameters) Enter(*elem);
    object.body->Accept(*this);
}
void MutableProgramListener::Visit(Program& object) {
    Enter(object);
    for (const auto& elem : object.variables) Enter(*elem);
    object.initializer->Accept(*this);
    for (const auto& elem : object.macroFunctions) elem->Accept(*this);
    for (const auto& elem : object.apiFunctions) elem->Accept(*this);
}
