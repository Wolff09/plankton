#pragma once
#ifndef PLANKTON_PROGRAMS_UTIL_HPP
#define PLANKTON_PROGRAMS_UTIL_HPP

#include <string>
#include "ast.hpp"
#include "util/shortcuts.hpp"

namespace plankton {
    
    template<typename T, EnableIfBaseOf<AstNode, T>>
    std::unique_ptr<T> Copy(const T& object);
    
    void Print(const AstNode& object, std::ostream& stream);
    
    std::string ToString(const Type& object);
    std::string ToString(const Sort& object);
    std::string ToString(const AstNode& object);
    std::string ToString(const VariableDeclaration& object);
    std::string ToString(const BinaryOperator& object);

    MoverType ComputeMoverType(const MovableAstNode& statement);

    bool SyntacticalEqual(const Expression& object, const Expression& other);
    
    inline bool IsVoid(const Function& function) { return function.returnType.empty(); }

} // namespace plankton

#endif //PLANKTON_PROGRAMS_UTIL_HPP
