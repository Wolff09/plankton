#include "programs/util.hpp"

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
constexpr std::string_view CMD_SKIP = "skip";
constexpr std::string_view CMD_BREAK = "break";
constexpr std::string_view CMD_RETURN = "return";
constexpr std::string_view CMD_ASSUME = "assume";
constexpr std::string_view CMD_ASSERT = "assert";
constexpr std::string_view CMD_MALLOC = "malloc";
constexpr std::string_view CMD_SIZEOF = "malloc";
constexpr std::string_view STMT_ATOMIC = "atomic";
constexpr std::string_view STMT_LOOP = "while (true)";
constexpr std::string_view STMT_CHOICE = "choose";

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
std::string plankton::ToString(const AstNode& object) { return MakeString(object); }
std::string plankton::ToString(const VariableDeclaration& object) { return MakeString(object); }
std::string plankton::ToString(const BinaryOperator& object) { return MakeString(object); }


//
// Printing AST
//

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
};

struct CommandPrinter : public BaseProgramVisitor {
    std::ostream& stream;
    ExpressionPrinter expressionPrinter;
    explicit CommandPrinter(std::ostream& stream) : stream(stream), expressionPrinter(stream) {}

    template<typename T>
    void PrintSequence(const T& sequence) {
        bool first = true;
        for (const auto& elem : sequence) {
            if (first) first = false;
            else stream << ", ";
            elem->Accept(*this);
        }
    }
    
    void Visit(const Skip& /*object*/) override { stream << CMD_SKIP << ";" << LB; }
    void Visit(const Break& /*object*/) override { stream << CMD_BREAK << ";" << LB; }
    void Visit(const Return& object) override {
        stream << CMD_RETURN;
        if (!object.expressions.empty()) stream << " ";
        PrintSequence(object.expressions);
        stream << ";" << LB;
    }
    void Visit(const Assume& object) override {
        stream << CMD_ASSUME << "(";
        object.condition->Accept(expressionPrinter);
        stream << ");" << LB;
    }
    void Visit(const Assert& object) override {
        stream << CMD_ASSERT << "(";
        object.condition->Accept(expressionPrinter);
        stream << ");" << LB;
    }
    void Visit(const Malloc& object) override {
        object.lhs->Accept(*this);
        stream << SYMBOL_ASSIGN << CMD_MALLOC << "(" << CMD_SIZEOF << "(";
        stream << object.lhs->Type().name << "))" << ";" << LB;
    }
    void Visit(const Macro& object) override {
        PrintSequence(object.lhs);
        stream << SYMBOL_ASSIGN << object.Func().name << "(";
        PrintSequence(object.arguments);
        stream << ");" << LB;
    }
    template<typename T>
    void HandleAssignment(const T& object) {
        assert(!object.lhs.empty());
        assert(!object.rhs.empty());
        PrintSequence(object.lhs);
        stream << SYMBOL_ASSIGN;
        PrintSequence(object.rhs);
        stream << LB;
    }
    void Visit(const VariableAssignment& object) override { HandleAssignment(object); }
    void Visit(const MemoryRead& object) override { HandleAssignment(object); }
    void Visit(const MemoryWrite& object) override { HandleAssignment(object); }
};

struct Indent {
    std::size_t depth = 0;
    inline Indent& operator++() { depth++; return *this; }
    inline Indent& operator--() { depth--; return *this; }
};

inline std::ostream& operator<<(std::ostream& stream, const Indent& indent) {
    for (std::size_t index = 0; index < indent.depth; ++index) stream << INDENT;
    return stream;
}

inline std::string GetPrintableTypeName(const Type& type) {
    std::string name = type.name;
    if (type.sort == Sort::PTR) name.pop_back(); // remove trailing * for pointer types
    return name;
}

struct ProgramPrinter : public BaseProgramVisitor {
    std::ostream& stream;
    Indent indent;
    CommandPrinter commandPrinter;
    explicit ProgramPrinter(std::ostream& stream) : stream(stream), commandPrinter(stream) {}
    
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
    
    void PrintScope(const Scope& scope) {
        stream << "{" << LB << ++indent;
        for (const auto& decl : scope.variables) stream << indent << *decl << LB;
        if (!scope.variables.empty()) stream << LB;
        stream << indent;
        scope.body->Accept(commandPrinter);
        stream << --indent << "}" << LB;
    }
    void Visit(const Scope& object) override {
        stream << indent;
        PrintScope(object);
    }
    void Visit(const Atomic& object) override {
        stream << indent << STMT_ATOMIC << " ";
        PrintScope(*object.body);
    }
    void Visit(const Sequence& object) override {
        object.first->Accept(*this);
        object.second->Accept(*this);
    }
    void Visit(const UnconditionalLoop& object) override {
        stream << indent << STMT_LOOP << " ";
        PrintScope(*object.body);
    }
    void Visit(const Choice& object) override {
        stream << STMT_CHOICE << " ";
        if (object.branches.empty()) {
            stream << ";" << LB;
            return;
        }
        for (const auto& branch : object.branches) {
            PrintScope(*branch);
        }
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
        stream << indent << "struct " << GetPrintableTypeName(type) << " {" << LB;
        ++indent;
        for (const auto& [fieldName, fieldType] : type) {
            stream << indent << GetPrintableTypeName(fieldType) << " " << fieldName << ";" << LB;
        }
        stream << --indent << "}" << LB;
    }
    void Visit(const Program& object) override {
        stream << "//" << LB << "// BEGIN " << object.name << LB << "//" << LB << LB;
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
        stream << "//" << LB << "// END " << LB << "//" << LB;
    }
};

void plankton::Print(const AstNode& object, std::ostream& stream) {
    struct Visitor : public ProgramVisitor {
        std::ostream& stream;
        explicit Visitor(std::ostream& stream) : stream(stream) {}
        void PrintExpression(const AstNode& object) { ExpressionPrinter printer(stream); object.Accept(printer); }
        void PrintCommand(const Command& expr) { CommandPrinter printer(stream); expr.Accept(printer); }
        void PrintProgram(const AstNode& expr) { ProgramPrinter printer(stream); expr.Accept(printer); }
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
        void Visit(const Return& object) override { PrintCommand(object); }
        void Visit(const Assume& object) override { PrintCommand(object); }
        void Visit(const Assert& object) override { PrintCommand(object); }
        void Visit(const Malloc& object) override { PrintCommand(object); }
        void Visit(const Macro& object) override { PrintCommand(object); }
        void Visit(const VariableAssignment& object) override { PrintCommand(object); }
        void Visit(const MemoryRead& object) override { PrintCommand(object); }
        void Visit(const MemoryWrite& object) override { PrintCommand(object); }
        void Visit(const Scope& object) override { PrintProgram(object); }
        void Visit(const Atomic& object) override { PrintProgram(object); }
        void Visit(const Sequence& object) override { PrintProgram(object); }
        void Visit(const UnconditionalLoop& object) override { PrintProgram(object); }
        void Visit(const Choice& object) override { PrintProgram(object); }
        void Visit(const Function& object) override { PrintProgram(object); }
        void Visit(const Program& object) override { PrintProgram(object); }
    } visitor(stream);
    object.Accept(visitor);
}