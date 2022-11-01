#pragma once
#ifndef PLANKTON_TOOL_TOSTRING_HPP
#define PLANKTON_TOOL_TOSTRING_HPP

#include "programs/ast.hpp"
#include "logics/util.hpp"
#include "engine/config.hpp"

namespace plankton {
    
    inline std::string ConfigToString(const SolverConfig& config, const Program& program) {
        constexpr std::string_view LB = "\n";
        constexpr std::string_view INDENT = "    ";
        
        std::stringstream stream;
        SymbolFactory factory;
        auto& flowType = config.GetFlowValueType();
        
        stream << "//" << LB << "// BEGIN solver config" << LB << "//" << LB << LB;
        stream << "flow value type:  Set<" << flowType.name << ">" << LB << LB;
        
        for (const auto& type : program.types) {
            for (const auto& pair : type->fields) {
                stream << "footprint depth for " << type->name << "::" << pair.first;
                stream << ":  " << config.GetMaxFootprintDepth(*type, pair.first) << LB;
            }
        }
        stream << LB;
        
        for (const auto& type : program.types) {
            auto dummyMem = plankton::MakeSharedMemory(factory.GetFreshFO(*type), flowType, factory);
            auto& dummyVal = factory.GetFreshFO(flowType);
            auto instance = config.GetLogicallyContains(*dummyMem, dummyVal);
            stream << "logically contains " << dummyVal << " for " << type->name << "::" << *dummyMem << ":  ";
            stream << LB << INDENT << *instance << LB;
        }
        stream << LB;
        
        for (const auto& type : program.types) {
            auto dummyMem = plankton::MakeSharedMemory(factory.GetFreshFO(*type), flowType, factory);
            auto& dummyVal = factory.GetFreshFO(flowType);
            for (const auto& [fieldName, fieldType] : type->fields) {
                if (fieldType.get().sort != Sort::PTR) continue;
                auto instance = config.GetOutflowContains(*dummyMem, fieldName, dummyVal);
                stream << "outflow of " << type->name << "::" << fieldName << " contains " << dummyVal << " for " << *dummyMem << ":  ";
                stream << LB << INDENT  << *instance << LB;
            }
        }
        stream << LB;
        
        for (const auto& var : program.variables) {
            auto dummyRes = std::make_unique<EqualsToAxiom>(*var, factory.GetFreshFO(var->type));
            auto instance = config.GetSharedVariableInvariant(*dummyRes);
            stream << "invariant for " << *dummyRes << ":  ";
            stream << LB << INDENT  << *instance << LB;
        }
        stream << LB;
        
        for (const auto& type : program.types) {
            auto dummyMem = plankton::MakeLocalMemory(factory.GetFreshFO(*type), flowType, factory);
            auto instance = config.GetLocalNodeInvariant(*dummyMem);
            stream << "local invariant for " << type->name << "::" << *dummyMem << ":  ";
            stream << LB << INDENT  << *instance << LB;
        }
        stream << LB;
        
        for (const auto& type : program.types) {
            auto dummyMem = plankton::MakeSharedMemory(factory.GetFreshFO(*type), flowType, factory);
            auto instance = config.GetSharedNodeInvariant(*dummyMem);
            stream << "shared invariant for " << type->name << "::" << *dummyMem << ":";
            stream << LB << INDENT  << *instance << LB;
        }
        stream << LB;

        for (const auto& type : program.types) {
            auto typeMem = plankton::MakeSharedMemory(factory.GetFreshFO(*type), flowType, factory);
            for (const auto& other : program.types) {
                auto otherMem = plankton::MakeSharedMemory(factory.GetFreshFO(*other), flowType, factory);
                auto instance = config.GetSharedNodePairInvariant(*typeMem, *otherMem);
                stream << "shared pairwise invariant for " << type->name << "::" << *typeMem << " and " << other->name << "::" << *otherMem << ":";
                stream << LB << INDENT  << *instance << LB;
            }
        }
        
        stream << LB << "//" << LB << "// END solver config" << LB << "//" << LB;
        return stream.str();
    }
    
}

#endif //PLANKTON_TOOL_TOSTRING_HPP
