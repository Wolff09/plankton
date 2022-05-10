#include "programs/util.hpp"

#include <deque>
#include <sstream>

using namespace plankton;

constexpr std::string_view LB = "\n";
constexpr std::string_view INDENT = "    ";
constexpr std::string_view LITERAL_TRUE = "true";
constexpr std::string_view LITERAL_FALSE = "false";
constexpr std::string_view LITERAL_MIN = "MIN";
constexpr std::string_view LITERAL_MAX = "MAX";
constexpr std::string_view LITERAL_NULL = "nullptr";
constexpr std::string_view LITERAL_VOID = "void";
constexpr std::string_view SYMBOL_TUPLE_BEGIN = "<";
constexpr std::string_view SYMBOL_TUPLE_END = ">";
constexpr std::string_view SYMBOL_DEREF = "->";
constexpr std::string_view SYMBOL_OP_SPACE = " ";
constexpr std::string_view SYMBOL_ASSIGN = " = ";
constexpr std::string_view SYMBOL_AND = "  &&  ";
constexpr std::string_view SYMBOL_OR = "  ||  ";
constexpr std::string_view CMD_SKIP = "skip";
constexpr std::string_view CMD_BREAK = "break";
constexpr std::string_view CMD_CONTINUE = "continue";
constexpr std::string_view CMD_RETURN = "return";
constexpr std::string_view CMD_ASSUME = "assume";
constexpr std::string_view CMD_ASSERT = "assert";
constexpr std::string_view CMD_LOCK = "__lock__";
constexpr std::string_view CMD_UNLOCK = "__unlock__";
constexpr std::string_view CMD_MALLOC = "malloc";
constexpr std::string_view CMD_SIZEOF = "__sizeof__";
constexpr std::string_view CMD_CAS = "CAS";
constexpr std::string_view STMT_ATOMIC = "atomic";
constexpr std::string_view STMT_LOOP = "while (*)";
constexpr std::string_view STMT_CHOICE = "choose";
constexpr std::string_view STMT_IF = "if";
constexpr std::string_view STMT_ELIF = "else if";
constexpr std::string_view STMT_ELSE = "else";
constexpr std::string_view STMT_WHILE = "while";
constexpr std::string_view STMT_DO = "do";

//
// Wrappers
//

template<typename T>
std::string MakeString(const T& object) {
    std::stringstream stream;
    stream << object;
    return stream.str();
}

std::string plankton::ToString(const Type& object) { return MakeString(object); }
std::string plankton::ToString(const Sort& object) { return MakeString(object); }
std::string plankton::ToString(const VariableDeclaration& object) { return MakeString(object); }
std::string plankton::ToString(const BinaryOperator& object) { return MakeString(object); }

std::string plankton::ToString(const AstNode& object, bool desugar) {
    std::stringstream stream;
    Print(object, stream, desugar);
    return stream.str();
}


//
// Printing AST
//

template<bool DESUGAR>
struct ExpressionPrinter : public BaseProgramVisitor {
    std::ostream& stream;
    explicit ExpressionPrinter(std::ostream& stream) : stream(stream) {}
    
    void Visit(const VariableExpression& object) override { stream << object.Decl().name; }
    void Visit(const TrueValue& /*object*/) override { stream << LITERAL_TRUE; }
    void Visit(const FalseValue& /*object*/) override { stream << LITERAL_FALSE; }
    void Visit(const MinValue& /*object*/) override { stream << LITERAL_MIN; }
    void Visit(const MaxValue& /*object*/) override { stream << LITERAL_MAX; }
    void Visit(const NullValue& /*object*/) override { stream << LITERAL_NULL; }
    void Visit(const Dereference& object) override {
        object.variable->Accept(*this);
        stream << SYMBOL_DEREF << object.fieldName;
    }
    void Visit(const BinaryExpression& object) override {
        object.lhs->Accept(*this);
        stream << SYMBOL_OP_SPACE << object.op << SYMBOL_OP_SPACE;
        object.rhs->Accept(*this);
    }

    template<typename T>
    inline void HandleCompound(const T& object, std::string_view op, std::string_view empty) {
        assert(!DESUGAR);
        if (object.expressions.empty()) stream << empty;
        stream << object.expressions.front();
        for (auto it = std::next(object.expressions.begin()); it != object.expressions.end(); ++it) {
            stream << op;
            (*it)->Accept(*this);
        }
    }
    void Visit(const AndExpression& object) override {
        HandleCompound(object, SYMBOL_AND, LITERAL_TRUE);
    }
    void Visit(const OrExpression& object) override {
        HandleCompound(object, SYMBOL_OR, LITERAL_FALSE);
    }
};

template<bool DESUGAR>
struct CommandPrinter : public ExpressionPrinter<DESUGAR> {
    using ExpressionPrinter<DESUGAR>::stream;
    using ExpressionPrinter<DESUGAR>::Visit;
    std::string_view lineEnd;

    explicit CommandPrinter(std::ostream& stream, bool breakLines) : ExpressionPrinter<DESUGAR>(stream), lineEnd(breakLines ? LB : " ") {}
    explicit CommandPrinter(std::ostream& stream) : CommandPrinter(stream, true) {}

    template<typename T>
    void PrintSequence(const T& sequence) {
        bool first = true;
        for (const auto& elem : sequence) {
            if (first) first = false;
            else stream << ", ";
            elem->Accept(*this);
        }
    }
    
    void Visit(const Skip& /*object*/) override { stream << CMD_SKIP << ";" << lineEnd; }
    void Visit(const Break& /*object*/) override { stream << CMD_BREAK << ";" << lineEnd; }
    void Visit(const Continue& /*object*/) override { stream << CMD_CONTINUE << ";" << lineEnd; }
    void Visit(const Return& object) override {
        stream << CMD_RETURN;
        if (!object.expressions.empty()) stream << " ";
        PrintSequence(object.expressions);
        stream << ";" << lineEnd;
    }
    void Visit(const Assume& object) override {
        stream << CMD_ASSUME << "(";
        object.condition->Accept(*this);
        stream << ");" << lineEnd;
    }
    void Visit(const Fail& /*object*/) override {
        stream << CMD_ASSERT << "(" << LITERAL_FALSE << ");" << lineEnd;
    }
    void Visit(const Malloc& object) override {
        object.lhs->Accept(*this);
        stream << SYMBOL_ASSIGN << CMD_MALLOC << "(" << CMD_SIZEOF << "(";
        stream << object.lhs->GetType().name << "))" << ";" << lineEnd;
    }
    void Visit(const Macro& object) override {
        if (!object.lhs.empty()) {
            PrintSequence(object.lhs);
            stream << SYMBOL_ASSIGN;
        }
        stream << object.Func().name << "(";
        PrintSequence(object.arguments);
        stream << ");" << lineEnd;
    }
    template<typename T>
    void HandleAssignment(const T& object) {
        assert(!object.lhs.empty());
        assert(!object.rhs.empty());
        PrintSequence(object.lhs);
        stream << SYMBOL_ASSIGN;
        PrintSequence(object.rhs);
        stream << ";" << lineEnd;
    }
    void Visit(const VariableAssignment& object) override { HandleAssignment(object); }
    void Visit(const MemoryWrite& object) override { HandleAssignment(object); }
    void Visit(const AcquireLock& object) override {
        stream << CMD_LOCK << "(";
        object.lock->Accept(*this);
        stream << ");" << lineEnd;
    }
    void Visit(const ReleaseLock& object) override {
        stream << CMD_UNLOCK << "(";
        object.lock->Accept(*this);
        stream << ");" << lineEnd;
    }

    void Visit(const ComplexAssume& object) override {
        if constexpr (DESUGAR) { object.desugared->Accept(*this); return; }
        stream << CMD_ASSUME << "(";
        object.condition->Accept(*this);
        stream << ");" << lineEnd;
    }
    void Visit(const ComplexAssert& object) override {
        if constexpr (DESUGAR) { object.desugared->Accept(*this); return; }
        stream << CMD_ASSERT << "(";
        object.condition->Accept(*this);
        stream << ");" << lineEnd;
    }
    void Visit(const CompareAndSwap& object) override {
        if constexpr (DESUGAR) { object.desugared->Accept(*this); return; }
        stream << CMD_CAS << "(";
        std::deque<const AstNode*> lhs, chk, rhs;
        for (const auto& elem : object.cas) {
            lhs.push_back(elem.lhs.get());
            chk.push_back(elem.chk.get());
            rhs.push_back(elem.rhs.get());
        }
        stream << "<"; PrintSequence(lhs); stream << ">, ";
        stream << "<"; PrintSequence(chk); stream << ">, ";
        stream << "<"; PrintSequence(rhs); stream << ">";
        stream << ");" << lineEnd;
    }
};

struct Indent {
    std::size_t depth = 0;
    inline Indent& operator++() { ++depth; return *this; }
    inline Indent& operator--() { --depth; return *this; }
};

inline std::ostream& operator<<(std::ostream& stream, const Indent& indent) {
    for (std::size_t index = 0; index < indent.depth; ++index) stream << INDENT;
    return stream;
}

template<bool DESUGAR>
struct ProgramPrinter : public CommandPrinter<DESUGAR> {
    using CommandPrinter<DESUGAR>::CommandPrinter;
    using CommandPrinter<DESUGAR>::Visit;
    using CommandPrinter<DESUGAR>::stream;
    Indent indent;
    
    template<typename T, typename F>
    void PrintSequence(const T& sequence, const F& printElem, const std::string& emptyString="") {
        if (sequence.empty()) {
            stream << emptyString;
            return;
        }
        bool first = true;
        for (const auto& elem : sequence) {
            if (first) first = false;
            else stream << ", ";
            printElem(elem);
        }
    }
    
    void PrintScope(const Scope& scope, bool breakLine = true) {
        stream << "{" << LB;
        ++indent;
        for (const auto& decl : scope.variables) stream << indent << *decl << LB;
        if (!scope.variables.empty()) stream << LB;
        stream << indent;
        scope.body->Accept(*this);
        --indent;
        stream << indent << "}";
        if (breakLine) stream << LB;
    }
    void Visit(const Scope& object) override {
        PrintScope(object);
    }
    void Visit(const Atomic& object) override {
        stream << STMT_ATOMIC << " ";
        PrintScope(*object.body);
    }
    void Visit(const Sequence& object) override {
        if (object.statements.empty()) return;
        object.statements.front()->Accept(*this);
        for (auto it = std::next(object.statements.begin()); it != object.statements.end(); ++it) {
            stream << indent;
            (*it)->Accept(*this);
        }
    }
    void Visit(const NondeterministicLoop& object) override {
        stream << STMT_LOOP << " ";
        PrintScope(*object.body);
    }
    void Visit(const Choice& object) override {
        stream << STMT_CHOICE << " ";
        if (object.branches.empty()) {
            stream << ";" << LB;
            return;
        }
        for (const auto& branch : object.branches) {
            PrintScope(*branch, false);
        }
        stream << LB;
    }
    void Visit(const Function& object) override {
        stream << indent;
        if (object.kind == Function::MACRO) stream << "inline ";
        if (object.returnType.size() > 1) stream << SYMBOL_TUPLE_BEGIN;
        PrintSequence(object.returnType, [this](auto& elem){ stream << elem.get().name; }, std::string(LITERAL_VOID));
        if (object.returnType.size() > 1) stream << SYMBOL_TUPLE_END;
        stream << " " << object.name << "(";
        PrintSequence(object.parameters, [this](auto& elem){ stream << elem->name; });
        stream << ") ";
        PrintScope(*object.body);
    }
    void PrintType(const Type& type) {
        stream << indent << "struct " << type.name << " {" << LB;
        ++indent;
        for (const auto& [fieldName, fieldType] : type) {
            VariableDeclaration dummy(fieldName, fieldType, false);
            stream << indent << dummy << LB;
            // stream << indent << fieldType.get().name << "* " << fieldName << ";" << LB;
        }
        stream << --indent << "}" << LB;
    }
    void Visit(const Program& object) override {
        stream << "//" << LB << "// BEGIN program " << object.name << LB << "//" << LB << LB;
        for (const auto& type : object.types) PrintType(*type);
        stream << LB << LB;
        for (const auto& decl : object.variables) stream << indent << *decl << LB;
        stream << LB << LB;
        object.initializer->Accept(*this);
        if (!object.macroFunctions.empty()) stream << LB << LB;
        for (const auto& macro : object.macroFunctions) {
            macro->Accept(*this);
            stream << LB;
        }
        stream << LB << LB;
        for (const auto& func : object.apiFunctions) {
            func->Accept(*this);
            stream << LB;
        }
        stream << "//" << LB << "// END program " << LB << "//" << LB;
    }

    void Visit(const IfThenElse& object) override {
        if constexpr (DESUGAR) { object.desugared->Accept(*this); return; }
        stream << STMT_IF << " (";
        object.condition->Accept(*this);
        stream << ") ";
        PrintScope(*object.ifBranch, false);
        stream << " " << STMT_ELSE << " ";
        PrintScope(*object.elseBranch);
    }
    void Visit(const IfElifElse& object) override {
        if constexpr (DESUGAR) { object.desugared->Accept(*this); return; }
        auto printBranch = [this](const auto& cb, std::string_view text) {
            stream << text << " (";
            cb.first->Accept(*this);
            stream << ") ";
            PrintScope(*cb.second, false);
            stream << " ";
        };
        assert(!object.conditionalBranches.empty());
        printBranch(object.conditionalBranches.front(), STMT_IF);
        for (auto it = std::next(object.conditionalBranches.begin()); it != object.conditionalBranches.end(); ++it) {
            printBranch(*it, STMT_ELIF);
        }
        stream << STMT_ELSE << " ";
        PrintScope(*object.elseBranch);
    }
    void Visit(const WhileLoop& object) override {
        if constexpr (DESUGAR) { object.desugared->Accept(*this); return; }
        stream << STMT_WHILE << " (";
        object.condition->Accept(*this);
        stream << ") ";
        PrintScope(*object.body);
    }
    void Visit(const DoWhileLoop& object) override {
        if constexpr (DESUGAR) { object.desugared->Accept(*this); return; }
        stream << STMT_DO << " ";
        PrintScope(*object.body, false);
        stream << " " << STMT_WHILE << " (";
        object.condition->Accept(*this);
        stream << ");" << LB;
    }
};

template<bool DESUGAR>
struct Dispatcher : public ProgramVisitor {
    std::ostream& stream;
    explicit Dispatcher(std::ostream& stream) : stream(stream) {}

    void PrintExpression(const AstNode& object) { ExpressionPrinter<DESUGAR> printer(stream); object.Accept(printer); }
    void PrintCommand(const Command& expr) { CommandPrinter<DESUGAR> printer(stream, false); expr.Accept(printer); }
    void PrintProgram(const AstNode& expr) { ProgramPrinter<DESUGAR> printer(stream); expr.Accept(printer); }

    void Visit(const VariableExpression& object) override { PrintExpression(object); }
    void Visit(const TrueValue& object) override { PrintExpression(object); }
    void Visit(const FalseValue& object) override { PrintExpression(object); }
    void Visit(const MinValue& object) override { PrintExpression(object); }
    void Visit(const MaxValue& object) override { PrintExpression(object); }
    void Visit(const NullValue& object) override { PrintExpression(object); }
    void Visit(const Dereference& object) override { PrintExpression(object); }
    void Visit(const BinaryExpression& object) override { PrintExpression(object); }
    void Visit(const Skip& object) override { PrintCommand(object); }
    void Visit(const Break& object) override { PrintCommand(object); }
    void Visit(const Continue& object) override { PrintCommand(object); }
    void Visit(const Return& object) override { PrintCommand(object); }
    void Visit(const Assume& object) override { PrintCommand(object); }
    void Visit(const Fail& object) override { PrintCommand(object); }
    void Visit(const Malloc& object) override { PrintCommand(object); }
    void Visit(const Macro& object) override { PrintCommand(object); }
    void Visit(const VariableAssignment& object) override { PrintCommand(object); }
    void Visit(const MemoryWrite& object) override { PrintCommand(object); }
    void Visit(const AcquireLock& object) override { PrintCommand(object); }
    void Visit(const ReleaseLock& object) override { PrintCommand(object); }
    void Visit(const Scope& object) override { PrintProgram(object); }
    void Visit(const Atomic& object) override { PrintProgram(object); }
    void Visit(const Sequence& object) override { PrintProgram(object); }
    void Visit(const NondeterministicLoop& object) override { PrintProgram(object); }
    void Visit(const Choice& object) override { PrintProgram(object); }
    void Visit(const Function& object) override { PrintProgram(object); }
    void Visit(const Program& object) override { PrintProgram(object); }

    void Visit(const AndExpression& object) override { PrintExpression(object); }
    void Visit(const OrExpression& object) override { PrintExpression(object); }
    void Visit(const ComplexAssume& object) override { PrintCommand(object); }
    void Visit(const ComplexAssert& object) override { PrintCommand(object); }
    void Visit(const CompareAndSwap& object) override { PrintCommand(object); }
    void Visit(const IfThenElse& object) override { PrintProgram(object); }
    void Visit(const IfElifElse& object) override { PrintProgram(object); }
    void Visit(const WhileLoop& object) override { PrintProgram(object); }
    void Visit(const DoWhileLoop& object) override { PrintProgram(object); }
};

void plankton::Print(const AstNode& object, std::ostream& stream, bool desugar) {
    if (desugar) {
        Dispatcher<true> dispatcher(stream);
        object.Accept(dispatcher);
    } else {
        Dispatcher<false> dispatcher(stream);
        object.Accept(dispatcher);
    }
}
