#include "programs/ast.hpp"

#include <cassert>
#include "programs/util.hpp"

using namespace plankton;


//
// AstNodes
//

template<typename T>
[[nodiscard]] const T* GetParent(const NavigableAstNode& node) {
    for (auto current = node.parent; current != nullptr; current = current->parent) {
        if (auto target = dynamic_cast<const T*>(current)) return target;
    }
    return nullptr;
}

struct ParentSetter : public MutableBaseProgramVisitor {
    template<typename P, typename C>
    inline void Handle(P& parent, C& child) {
        child->parent = &parent;
        child->parentProgram = GetParent<Program>(*child);
        child->parentFunction = GetParent<Function>(*child);
        child->parentScope = GetParent<Scope>(*child);
        child->parentAtomic = GetParent<Atomic>(*child);
        child->Accept(*this);
    }
    template<typename P, typename C>
    inline void HandleAll(P& parent, C& children) {
        for (auto& child : children) Handle(parent, child);
    }
    void Visit(Scope& object) override { Handle(object, object.body); }
    void Visit(Atomic& object) override { Handle(object, object.body); }
    void Visit(Sequence& object) override { HandleAll(object, object.statements); }
    void Visit(UnconditionalLoop& object) override { Handle(object, object.body); }
    void Visit(Choice& object) override { HandleAll(object, object.branches); }
    void Visit(Function& object) override { Handle(object, object.body); }
    void Visit(Program& object) override {
        Handle(object, object.initializer);
        HandleAll(object, object.macroFunctions);
        HandleAll(object, object.apiFunctions);
        HandleAll(object, object.maintenanceFunctions);
    }
};

void NavigableAstNode::MakeNavigable() {
    ParentSetter setter;
    Accept(setter);
}

//
// Types, Variables
//

Type::Type(std::string name, Sort sort) : name(std::move(name)), sort(sort) {}

decltype(Type::fields)::const_iterator Type::begin() const { return fields.cbegin(); }

decltype(Type::fields)::const_iterator Type::end() const { return fields.cend(); }

bool Type::operator==(const Type& other) const { return this == &other; }

bool Type::operator!=(const Type& other) const { return this != &other; }

const Type& Type::operator[](const std::string& fieldName) const { return fields.at(fieldName); }

bool Type::AssignableFrom(const Type& other) const { return other.AssignableTo(*this); }

bool Type::Comparable(const Type& other) const { return AssignableTo(other) || AssignableFrom(other); }

const Type& Type::Bool() {
    static const Type type = Type("bool", Sort::BOOL);
    return type;
}

const Type& Type::Data() {
    static const Type type = Type("data_t", Sort::DATA);
    return type;
}

const Type& Type::Null() {
    static const Type type = Type("nullptr_t", Sort::PTR);
    return type;
}

const Type& Type::Thread() {
    static const Type type = Type("thread_t", Sort::TID);
    return type;
}

std::optional<std::reference_wrapper<const Type>> Type::GetField(const std::string& fieldName) const {
    auto find = fields.find(fieldName);
    if (find == fields.end()) return std::nullopt;
    else return find->second;
}

bool Type::AssignableTo(const Type& other) const {
    if (sort != Sort::PTR) return sort == other.sort;
    else return *this == other || *this == Null();
}

VariableDeclaration::VariableDeclaration(std::string name, const Type& type, bool shared)
: name(std::move(name)), type(type), isShared(shared) {}

bool VariableDeclaration::operator==(const VariableDeclaration& other) const { return this == &other; }

bool VariableDeclaration::operator!=(const VariableDeclaration& other) const { return this != &other; }


//
// Programs, Functions
//

Program::Program(std::string name, std::unique_ptr<Function> init)
: name(std::move(name)), initializer(std::move(init)) {
    assert(initializer);
}

Function::Function(std::string name, Kind kind, std::unique_ptr<Scope> bdy)
        : name(std::move(name)), kind(kind), body(std::move(bdy)) {
    assert(body);
}


//
// Expressions
//

VariableExpression::VariableExpression(const VariableDeclaration& decl) : decl(decl) {}

const Type& VariableExpression::GetType() const { return Decl().type; }

const plankton::Type& TrueValue::GetType() const { return plankton::Type::Bool(); }

const plankton::Type& FalseValue::GetType() const { return plankton::Type::Bool(); }

const Type& MinValue::GetType() const { return plankton::Type::Data(); }

const Type& MaxValue::GetType() const { return plankton::Type::Data(); }

const Type& NullValue::GetType() const { return plankton::Type::Null(); }

Dereference::Dereference(std::unique_ptr<VariableExpression> var, std::string fieldName_)
        : variable(std::move(var)), fieldName(std::move(fieldName_)) {
    assert(variable->GetSort() == Sort::PTR); // TODO: throw
    assert(variable->GetType().GetField(fieldName).has_value()); // TODO: throw
}

const Type& Dereference::GetType() const {
    return variable->GetType()[fieldName];
}

BinaryOperator plankton::Symmetric(BinaryOperator op) {
    switch (op) {
        case BinaryOperator::EQ: return BinaryOperator::EQ;
        case BinaryOperator::NEQ: return BinaryOperator::NEQ;
        case BinaryOperator::LEQ: return BinaryOperator::GEQ;
        case BinaryOperator::LT: return BinaryOperator::GT;
        case BinaryOperator::GEQ: return BinaryOperator::LEQ;
        case BinaryOperator::GT: return BinaryOperator::LT;
    }
    throw;
}

BinaryOperator plankton::Negate(BinaryOperator op) {
    switch (op) {
        case BinaryOperator::EQ: return BinaryOperator::NEQ;
        case BinaryOperator::NEQ: return BinaryOperator::EQ;
        case BinaryOperator::LEQ: return BinaryOperator::GT;
        case BinaryOperator::LT: return BinaryOperator::GEQ;
        case BinaryOperator::GEQ: return BinaryOperator::LT;
        case BinaryOperator::GT: return BinaryOperator::LEQ;
    }
    throw;
}

BinaryExpression::BinaryExpression(BinaryOperator op, std::unique_ptr<ValueExpression> lhs_,
                                   std::unique_ptr<ValueExpression> rhs_)
        : op(op), lhs(std::move(lhs_)), rhs(std::move(rhs_)) {
    assert(lhs);
    assert(rhs);
    assert(lhs->GetType().Comparable(rhs->GetType()));
    assert(op == BinaryOperator::EQ || op == BinaryOperator::NEQ || lhs->GetType() == Type::Data());
}

const Type& BinaryExpression::GetType() const { return plankton::Type::Bool(); }


//
// Statements
//

Scope::Scope(std::unique_ptr<Statement> bdy) : body(std::move(bdy)) {
    assert(body);
}

Atomic::Atomic(std::unique_ptr<Scope>  bdy) : body(std::move(bdy)) {
    assert(body);
}

Sequence::Sequence(std::unique_ptr<Statement> stmt) {
    assert(stmt);
    statements.push_back(std::move(stmt));
}

Sequence::Sequence(std::unique_ptr<Statement> first, std::unique_ptr<Statement> second) {
    assert(first);
    assert(second);
    statements.push_back(std::move(first));
    statements.push_back(std::move(second));
}

Sequence::Sequence(std::vector<std::unique_ptr<Statement>> stmts) {
    assert(AllNonNull(stmts));
    MoveInto(std::move(stmts), statements);
}

NondeterministicLoop::NondeterministicLoop(std::unique_ptr<Scope> bdy) : body(std::move(bdy)) {
    assert(body);
}

Choice::Choice() = default;

Choice::Choice(std::unique_ptr<Scope> branch1, std::unique_ptr<Scope> branch2) {
    assert(branch1);
    assert(branch2);
    branches.push_back(std::move(branch1));
    branches.push_back(std::move(branch2));
}


//
// Commands
//

Skip::Skip() = default;

Fail::Fail() = default;

Continue::Continue() = default;

Break::Break() = default;

Return::Return() = default;

Return::Return(std::unique_ptr<SimpleExpression> expression) {
    assert(expression);
    expressions.push_back(std::move(expression));
}

Assume::Assume(std::unique_ptr<BinaryExpression> cond) : condition(std::move(cond)) {
    assert(condition);
    assert(condition->GetType() == Type::Bool());
}

Malloc::Malloc(std::unique_ptr<VariableExpression> left) : lhs(std::move(left)) {
    assert(lhs);
}

Macro::Macro(const Function& function) : function(function) {
}

AcquireLock::AcquireLock(std::unique_ptr<Dereference> lock_) : lock(std::move(lock_)) {
    assert(lock);
    assert(lock->GetSort() == Sort::TID);
}

ReleaseLock::ReleaseLock(std::unique_ptr<Dereference> lock_) : lock(std::move(lock_)) {
    assert(lock);
    assert(lock->GetSort() == Sort::TID);
}

const Function& Macro::Func() const { return function; }

template<typename L, typename R>
Assignment<L,R>::Assignment() = default;

template<typename L, typename R>
Assignment<L,R>::Assignment(std::unique_ptr<L> left, std::unique_ptr<R> right) {
    assert(left);
    assert(right);
    lhs.push_back(std::move(left));
    rhs.push_back(std::move(right));
}

template struct plankton::Assignment<VariableExpression, ValueExpression>;
template struct plankton::Assignment<Dereference, SimpleExpression>;


//
// Output
//

std::ostream& plankton::operator<<(std::ostream& stream, const Sort& object) {
    switch (object) {
        case Sort::VOID: stream << "VoidSort"; break;
        case Sort::BOOL: stream << "BoolSort"; break;
        case Sort::DATA: stream << "DataSort"; break;
        case Sort::TID: stream << "ThreadSort"; break;
        case Sort::PTR: stream << "PointerSort"; break;
    }
    return stream;
}

std::ostream& plankton::operator<<(std::ostream& stream, const BinaryOperator& object) {
    switch (object) {
        case BinaryOperator::EQ: stream << "=="; break;
        case BinaryOperator::NEQ: stream << "!="; break;
        case BinaryOperator::LEQ: stream << "<="; break;
        case BinaryOperator::LT: stream << "<"; break;
        case BinaryOperator::GEQ: stream << ">="; break;
        case BinaryOperator::GT: stream << ">"; break;
    }
    return stream;
}

std::ostream& plankton::operator<<(std::ostream& stream, const Type& object) {
    stream << "type " << object.name << " { ";
    bool first = true;
    for (const auto& [field, type] : object) {
        if (first) first = false;
        else stream << ", ";
        stream << field << ":" << type.get().name;
    }
    stream << " }";
    return stream;
}

std::ostream& plankton::operator<<(std::ostream& stream, const AstNode& object) {
    plankton::Print(object, stream);
    return stream;
}

std::ostream& plankton::operator<<(std::ostream& stream, const VariableDeclaration& object) {
    stream << object.type.name;
    if (object.type.sort == Sort::PTR) stream << "*";
    stream << " " << object.name << ";";
    return stream;
}