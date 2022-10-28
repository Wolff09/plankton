#include "engine/util.hpp"

#include "logics/util.hpp"
#include "util/shortcuts.hpp"

using namespace plankton;


//
// Reachability
//

bool ReachSet::IsReachable(const SymbolDeclaration& source, const SymbolDeclaration& target) const {
    return plankton::Membership(container.at(&source), &target);
}

const std::set<const SymbolDeclaration*>& ReachSet::GetReachable(const SymbolDeclaration& source) const {
    static std::set<const SymbolDeclaration*> empty;
    auto find = container.find(&source);
    if (find != container.end()) return find->second;
    return empty;
}

inline ReachSet ComputeReach(ReachSet initial) {
    ReachSet reachability = std::move(initial);
    bool changed;
    do {
        changed = false;
        for (auto& [node, nodeReach] : reachability.container) {
            assert(node);
            auto size = nodeReach.size();
            for (const auto* value : nodeReach) {
                if (value->type.sort != Sort::PTR) continue;
                const auto& valueReach = reachability.container[value];
                nodeReach.insert(valueReach.begin(), valueReach.end());
            }
            changed |= size != nodeReach.size();
        }
    } while (changed);
    return reachability;
}

ReachSet ComputeReach(const Formula& formula, const std::set<std::pair<const Type*, std::string>>& ignoreEdges) {
    auto ignore = [&ignoreEdges](const auto& pair) {
        auto find = ignoreEdges.find({ &pair.second->GetType(), pair.first });
        return find != ignoreEdges.end();
    };
    ReachSet initial;
    for (const auto *memory: plankton::Collect<MemoryAxiom>(formula)) {
        for (const auto& pair: memory->fieldToValue) {
            if (pair.second->GetSort() != Sort::PTR) continue;
            if (ignore(pair)) continue;
            initial.container[&memory->node->Decl()].insert(&pair.second->Decl());
        }
    }
    return ComputeReach(std::move(initial));
}

ReachSet ComputeReach(const FlowGraph& graph, EMode mode, const std::set<std::pair<const Type*, std::string>>& ignoreEdges) {
    auto ignore = [&ignoreEdges](const auto& type, const auto& name) {
        auto find = ignoreEdges.find({ &type, name });
        return find != ignoreEdges.end();
    };
    ReachSet initial;
    for (const auto& node : graph.nodes) {
        for (const auto& field : node.pointerFields) {
            assert(field.Value(mode).type.sort == Sort::PTR);
            if (ignore(node.address.type, field.name)) continue;
            initial.container[&node.address].insert(&field.Value(mode));
        }
    }
    return ComputeReach(std::move(initial));
}

ReachSet plankton::ComputeReachability(const Formula& formula) {
    return ComputeReach(formula, {});
}

ReachSet plankton::ComputeReachability(const FlowGraph& graph, EMode mode) {
    return ComputeReach(graph, mode, {});
}

inline std::pair<std::unique_ptr<SharedMemoryCore>, const SymbolDeclaration*> MakeDummy(const Type& type, const Type& flowType) {
    SymbolFactory factory;
    auto& node = factory.GetFreshFO(type);
    auto& flow = factory.GetFreshSO(flowType);
    std::map<std::string, std::reference_wrapper<const SymbolDeclaration>> fieldToValue;
    for (const auto& [fieldName, fieldType] : type.fields) {
        fieldToValue.emplace(fieldName, factory.GetFreshFO(fieldType));
    }
    auto memory = std::make_unique<SharedMemoryCore>(node, flow, std::move(fieldToValue));
    auto& value = factory.GetFreshFO(flowType);
    return std::make_pair(std::move(memory), &value);
}

bool plankton::IsOutflowFalse(const Type& type, const std::string& field, const SolverConfig& config) {
    assert(type.GetField(field).has_value());
    assert(type.GetField(field)->get().sort == Sort::PTR);
    Encoding encoding;
    auto [axiom, value] = MakeDummy(type, config.GetFlowValueType());
    auto pred = config.GetOutflowContains(*axiom, field, *value);
    return encoding.Implies(encoding.Encode(*pred) == encoding.Bool(false));
}

std::set<std::pair<const Type*, std::string>> MakeBlacklist(const std::set<const Type*>& types, const SolverConfig& config) {
    std::set<std::pair<const Type*, std::string>> blacklist;
    for (const auto* type : types) {
        for (const auto& [fieldName, fieldType] : type->fields) {
            if (fieldType.get().sort != Sort::PTR) continue;
            if (!IsOutflowFalse(*type, fieldName, config)) continue;
            blacklist.emplace(type, fieldName);
        }
    }
    return blacklist;
}

ReachSet plankton::ComputeEffectiveReachability(const Formula& formula, const SolverConfig& config) {
    std::set<const Type*> types;
    for (const auto* decl : plankton::Collect<SymbolDeclaration>(formula)) types.insert(&decl->type);
    return ComputeReach(formula, MakeBlacklist(types, config));
}

ReachSet plankton::ComputeEffectiveReachability(const FlowGraph& graph, EMode mode) {
    std::set<const Type*> types;
    for (const auto& node : graph.nodes) types.insert(&node.address.type);
    return ComputeReach(graph, mode, MakeBlacklist(types, graph.config));
}
