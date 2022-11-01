#include "engine/encoding.hpp"

#include "internal.hpp"
#include "logics/util.hpp"
#include "engine/util.hpp"

using namespace plankton;


#define CTX AsContext(internal)
#define SOL AsSolver(internal)

static constexpr int NULL_VALUE = 0;
static constexpr int MIN_VALUE = -65536;
static constexpr int MAX_VALUE = 65536;
static constexpr int UNLOCKED_VALUE = 0;

inline z3::sort EncodeSort(Sort sort, z3::context& context) {
    switch (sort) {
        case Sort::BOOL: return context.bool_sort();
        default: return context.int_sort();
    }
}


EExpr Encoding::Min() { return AsEExpr(CTX.int_val(MIN_VALUE)); }
EExpr Encoding::Max() { return AsEExpr(CTX.int_val(MAX_VALUE)); }
EExpr Encoding::Null() { return AsEExpr(CTX.int_val(NULL_VALUE)); }
EExpr Encoding::Bool(bool val) { return AsEExpr(CTX.bool_val(val)); }

EExpr Encoding::TidSelf() { return AsEExpr(CTX.constant("__SELF", EncodeSort(Sort::TID, CTX))); }
EExpr Encoding::TidSome() { return AsEExpr(CTX.constant("__SOME", EncodeSort(Sort::TID, CTX))); }
EExpr Encoding::TidUnlocked() { return AsEExpr(CTX.int_val(UNLOCKED_VALUE)); }

EExpr Encoding::Replace(const EExpr& expression, const EExpr& replace, const EExpr& with) {
    z3::expr_vector replaceVec(CTX), withVec(CTX);
    replaceVec.push_back(AsExpr(replace));
    withVec.push_back(AsExpr(with));
    return AsEExpr(AsExpr(expression).substitute(replaceVec, withVec));
}

inline z3::expr_vector AsVector(const std::vector<EExpr>& vector, z3::context& context) {
    z3::expr_vector result(context);
    for (const auto& elem : vector) result.push_back(AsExpr(elem));
    return result;
}

EExpr Encoding::MakeDistinct(const std::vector<EExpr>& vec) {
    return AsEExpr(z3::distinct(AsVector(vec, CTX)));
}
EExpr Encoding::MakeAnd(const std::vector<EExpr>& vec) {
    if (vec.empty()) return Bool(true);
    return AsEExpr(z3::mk_and(AsVector(vec, CTX)));
}
EExpr Encoding::MakeOr(const std::vector<EExpr>& vec) {
    if (vec.empty()) return Bool(false);
    return AsEExpr(z3::mk_or(AsVector(vec, CTX)));
}
EExpr Encoding::MakeAtMost(const std::vector<EExpr>& vec, unsigned int count) {
    return AsEExpr(z3::atmost(AsVector(vec, CTX), count));
}


EExpr Encoding::MakeQuantifiedVariable(Sort sort) {
    return AsEExpr(CTX.constant("__qv", EncodeSort(sort, CTX)));
}


template<typename K, typename V, typename F>
inline V GetOrCreate(std::map<K, V>& map, K key, F create) {
    auto find = map.find(key);
    if (find != map.end()) return find->second;
    auto newValue = create();
    map.emplace(key, newValue);
    return newValue;
}

EExpr Encoding::Encode(const VariableDeclaration& decl) {
    return GetOrCreate(variableEncoding, &decl, [this,&decl](){
        auto name = "__" + decl.name;
        auto expr = CTX.constant(name.c_str(), EncodeSort(decl.type.sort, CTX));
        return AsEExpr(expr);
    });
}

EExpr Encoding::Encode(const SymbolDeclaration& decl) {
    switch (decl.order) {
        case Order::FIRST:
            return GetOrCreate(symbolEncoding, &decl, [this, &decl]() {
                // create symbol
                auto name = "_v" + decl.name;
                auto expr = CTX.constant(name.c_str(), EncodeSort(decl.type.sort, CTX));
                // add implicit bounds on first order data values
                switch (decl.type.sort) {
                    case Sort::DATA:
                        SOL.add(AsExpr(Min()) <= expr);
                        SOL.add(expr <= AsExpr(Max()));
                        break;
                    case Sort::TID:
                        SOL.add(AsExpr(TidUnlocked()) <= expr);
                        break;
                    default: break;
                }
                // add implicit bounds on second order data values
                return AsEExpr(expr);
            });
    
        case Order::SECOND:
            return GetOrCreate(symbolEncoding, &decl, [this, &decl]() {
                // create symbol
                auto name = "_V" + decl.name;
                auto expr = CTX.function(name.c_str(), EncodeSort(decl.type.sort, CTX), CTX.bool_sort());
                // add implicit bounds on data values
                assert(decl.type.sort == Sort::DATA);
                auto qv = AsExpr(MakeQuantifiedVariable(decl.type.sort));
                SOL.add(z3::forall(qv, z3::implies(qv < AsExpr(Min()), !expr(qv))));
                SOL.add(z3::forall(qv, z3::implies(qv > AsExpr(Max()), !expr(qv))));
                return AsEExpr(expr);
            });
    }
    throw;
}

EExpr Encoding::EncodeExists(Sort sort, const std::function<EExpr(EExpr)>& makeInner) {
    auto qv = MakeQuantifiedVariable(sort);
    auto inner = makeInner(EExpr(qv));
    return AsEExpr(z3::exists(AsExpr(qv), AsExpr(inner)));
}

EExpr Encoding::EncodeForAll(Sort sort, const std::function<EExpr(EExpr)>& makeInner) {
    auto qv = MakeQuantifiedVariable(sort);
    auto inner = makeInner(EExpr(qv));
    return AsEExpr(z3::forall(AsExpr(qv), AsExpr(inner)));
}

EExpr Encoding::EncodeFlowContains(const SymbolDeclaration& flow, const EExpr& expr) {
    return Encode(flow)(expr);
}

EExpr Encoding::EncodeIsNonNull(const EExpr& expr) {
    return expr != Null();
}

EExpr Encoding::EncodeIsNonNull(const SymbolDeclaration& decl) {
    return Encode(decl) != Null();
}

EExpr Encoding::EncodeIsNonNull(const VariableDeclaration& decl) {
    return Encode(decl) != Null();
}

EExpr Encoding::EncodeIsNull(const EExpr& expr) {
    return expr == Null();
}

EExpr Encoding::EncodeIsNull(const SymbolDeclaration& decl) {
    return Encode(decl) == Null();
}

EExpr Encoding::EncodeIsNull(const VariableDeclaration& decl) {
    return Encode(decl) == Null();
}

EExpr Encoding::EncodeMemoryEquality(const MemoryAxiom& memory, const MemoryAxiom& other) {
    z3::expr_vector result(CTX);
    result.push_back(AsExpr(Encode(memory.node->Decl()) == Encode(other.node->Decl())));
    result.push_back(AsExpr(Encode(memory.flow->Decl()) == Encode(other.flow->Decl())));
    assert(memory.node->GetType() == other.node->GetType());
    for (const auto& [field, value] : memory.fieldToValue) {
        result.push_back(AsExpr(Encode(value->Decl()) == Encode(other.fieldToValue.at(field)->Decl())));
    }
    return AsEExpr(z3::mk_and(result));
}

struct FormulaEncoder : public BaseLogicVisitor {
    Encoding& encoding;
    explicit FormulaEncoder(Encoding& encoding) : encoding(encoding) {}
    std::optional<EExpr> result = std::nullopt;
    
    EExpr Encode(const LogicObject& object) {
        result = std::nullopt;
        object.Accept(*this);
        assert(result.has_value());
        return result.value();
    }
    
    void Visit(const LocalMemoryResource& object) override { result = encoding.EncodeIsNonNull(object.node->Decl()); }
    void Visit(const SharedMemoryCore& object) override { result = encoding.EncodeIsNonNull(object.node->Decl()); }
    void Visit(const ObligationAxiom& /*object*/) override { result = encoding.Bool(true); }
    void Visit(const FulfillmentAxiom& /*object*/) override { result = encoding.Bool(true); }
    
    void Visit(const SymbolicVariable& object) override { result = encoding.Encode(object.Decl()); }
    void Visit(const SymbolicBool& object) override { result = encoding.Bool(object.value); }
    void Visit(const SymbolicNull& /*object*/) override { result = encoding.Null(); }
    void Visit(const SymbolicMin& /*object*/) override { result = encoding.Min(); }
    void Visit(const SymbolicMax& /*object*/) override { result = encoding.Max(); }
    void Visit(const SymbolicSelfTid& /*object*/) override { result = encoding.TidSelf(); }
    void Visit(const SymbolicSomeTid& /*object*/) override { result = encoding.TidSome(); }
    void Visit(const SymbolicUnlocked& /*object*/) override { result = encoding.TidUnlocked(); }
    void Visit(const SeparatingConjunction& object) override {
        std::vector<EExpr> conjuncts;
        conjuncts.reserve(object.conjuncts.size());
        for (const auto& elem : object.conjuncts) conjuncts.push_back(Encode(*elem));
        auto localMemory = plankton::Collect<LocalMemoryResource>(object);
        auto allMemory = plankton::Collect<MemoryAxiom>(object);
        for (const auto* local : localMemory) {
            for (const auto* other : allMemory) {
                if (local == other) continue;
                conjuncts.push_back(encoding.Encode(local->node->Decl()) != encoding.Encode(other->node->Decl()));
            }
        }
        result = encoding.MakeAnd(conjuncts);
    }
    void Visit(const EqualsToAxiom& object) override {
        result = encoding.Encode(object.Variable()) == encoding.Encode(object.Value());
    }
    void Visit(const StackAxiom& object) override {
        auto lhs = Encode(*object.lhs);
        auto rhs = Encode(*object.rhs);
        switch (object.op) {
            case BinaryOperator::EQ: result = lhs == rhs; break;
            case BinaryOperator::NEQ: result = lhs != rhs; break;
            case BinaryOperator::LEQ: result = lhs <= rhs; break;
            case BinaryOperator::LT: result = lhs < rhs; break;
            case BinaryOperator::GEQ: result = lhs >= rhs; break;
            case BinaryOperator::GT: result = lhs > rhs; break;
        }
    }
    void Visit(const InflowEmptinessAxiom& object) override {
        auto flow = encoding.Encode(object.flow->Decl());
        result = encoding.EncodeForAll(object.flow->GetSort(), [flow](auto qv){
            return !flow(qv);
        });
        if (!object.isEmpty) result = !result.value();
    }
    void Visit(const InflowContainsValueAxiom& object) override {
        result = encoding.Encode(object.flow->Decl())(encoding.Encode(object.value->Decl()));
    }
    void Visit(const InflowContainsRangeAxiom& object) override {
        auto flow = encoding.Encode(object.flow->Decl());
        auto low = Encode(*object.valueLow);
        auto high = Encode(*object.valueHigh);
        result = encoding.EncodeForAll(object.flow->GetSort(), [flow,low,high](auto qv){
            return ((low <= qv) && (qv <= high)) >> flow(qv);
        });
    }
    void Visit(const NonSeparatingImplication& object) override {
        result = Encode(*object.premise) >> Encode(*object.conclusion);
    }
    void Visit(const ImplicationSet& object) override {
        auto conjuncts = plankton::MakeVector<EExpr>(object.conjuncts.size());
        for (const auto& conjunct : object.conjuncts) conjuncts.push_back(Encode(*conjunct));
        result = encoding.MakeAnd(conjuncts);
    }
};

EExpr Encoding::Encode(const LogicObject& object) {
    FormulaEncoder encoder(*this);
    return encoder.Encode(object);
}

EExpr Encoding::EncodeFormulaWithKnowledge(const Formula& formula, const SolverConfig& config) {
    return Encode(formula)
           && EncodeInvariants(formula, config)
           && EncodePairInvariants(formula, config)
           && EncodeSimpleFlowRules(formula, config)
           && EncodeOwnership(formula)
           && EncodeAcyclicity(formula, config)
           ;
}

EExpr Encoding::EncodeInvariants(const Formula& formula, const SolverConfig& config) {
    auto local = plankton::Collect<LocalMemoryResource>(formula);
    auto shared = plankton::Collect<SharedMemoryCore>(formula);
    auto variables = plankton::Collect<EqualsToAxiom>(formula);
    std::vector<EExpr> result;
    result.reserve(local.size() + shared.size() + variables.size());
    for (const auto* mem : local) result.push_back(Encode(*config.GetLocalNodeInvariant(*mem)));
    for (const auto* mem : shared) result.push_back(Encode(*config.GetSharedNodeInvariant(*mem)));
    for (const auto* var : variables)
        if (var->Variable().isShared) result.push_back(Encode(*config.GetSharedVariableInvariant(*var)));
    return MakeAnd(result);
}

EExpr Encoding::EncodePairInvariants(const Formula& formula, const SolverConfig& config) {
    auto shared = plankton::Collect<SharedMemoryCore>(formula);
    auto result = plankton::MakeVector<EExpr>(shared.size() * shared.size());
    for (const auto* memory : shared) {
        for (const auto* other : shared) {
            if (memory == other) continue;
            result.push_back(Encode(*config.GetSharedNodePairInvariant(*memory, *other)));
        }
    }
    return MakeAnd(result);
}

inline EExpr EncodeAcyclicityFromReach(Encoding& encoding, const ReachSet& reachability) {
    auto result = plankton::MakeVector<EExpr>(reachability.container.size());
    for (const auto& [node, reach] : reachability.container) {
        auto distinct = encoding.Encode(*node);
        for (const auto* other : reach) {
            result.push_back(distinct != encoding.Encode(*other));
        }
    }
    return encoding.MakeAnd(result);
}

EExpr Encoding::EncodeAcyclicity(const Formula& formula, const SolverConfig& config) {
    switch (config.GetGraphAcyclicity()) {
        case SolverConfig::PHYSICAL: return EncodeAcyclicityFromReach(*this, plankton::ComputeReachability(formula));
        case SolverConfig::EFFECTIVE: return EncodeAcyclicityFromReach(*this, plankton::ComputeEffectiveReachability(formula, config));
        case SolverConfig::NONE: return Bool(true);
    }
}

EExpr Encoding::EncodeOutflowContains(const MemoryAxiom& memory, const std::string& field, const EExpr& value, const SolverConfig& config) {
    auto& dummy = SymbolFactory(memory).GetFreshFO(memory.flow->GetType());
    auto predicate = config.GetOutflowContains(memory, field, dummy);
    return Replace(Encode(*predicate), Encode(dummy), value);
}

EExpr Encoding::EncodeOwnership(const Formula& formula) {
    auto local = plankton::Collect<LocalMemoryResource>(formula);
    auto shared = plankton::Collect<SharedMemoryCore>(formula);
    auto variables = plankton::Collect<EqualsToAxiom>(formula);
    auto result = plankton::MakeVector<EExpr>(local.size() * (shared.size() + variables.size()));
    for (const auto* memory : local) {
        auto address = Encode(memory->node->Decl());
        for (const auto* variable : variables) {
            if (!variable->Variable().isShared) continue;
            result.push_back(address != Encode(variable->Variable()));
        }
        for (const auto* other : shared) {
            result.push_back(address != Encode(other->node->Decl()));
            for (const auto& pair : other->fieldToValue) {
                if (pair.second->GetSort() != Sort::PTR) continue;
                result.push_back(address != Encode(pair.second->Decl()));
            }
        }
    }
    return MakeAnd(result);
}

EExpr Encoding::EncodeSimpleFlowRules(const Formula& formula, const SolverConfig& config) {
    auto memories = plankton::Collect<MemoryAxiom>(formula);
    auto& flowType = config.GetFlowValueType();
    auto symbols = plankton::Collect<SymbolDeclaration>(formula, [&flowType](const auto& decl){
        return decl.order == Order::FIRST && decl.type.sort == flowType.sort;
    });

    auto result = plankton::MakeVector<EExpr>(16);
    for (const auto* memory : memories) {
        auto inflowMemory = Encode(*memory->flow);
        for (const auto& [name, value]: memory->fieldToValue) {
            if (value->GetSort() != Sort::PTR) continue;
            for (const auto* other : memories) {
                if (memory == other) continue;
                if (value->Decl() != other->node->Decl()) continue;
                auto inflowOther = Encode(*other->flow);
                for (const auto* symbol : symbols) {
                    auto flowsOut = Encode(*config.GetOutflowContains(*memory, name, *symbol));
                    auto encSym = Encode(*symbol);
                    auto rule1 = (inflowMemory(encSym) && flowsOut) >> inflowOther(encSym);
                    auto rule2 = Bool(true); // inflowMemory(encSym) && inflowOther(encSym)) >> flowsOut; // this relies on inflow uniqueness // TODO: is it even correct?? ~~> most certainly not
                    result.push_back(rule1 && rule2);
                }
            }
        }
    }

    return MakeAnd(result);
}

//EExpr Encoding::EncodeSimpleFlowRules(const Formula& formula, const SolverConfig& config) {
//    auto memories = plankton::Collect<MemoryAxiom>(formula);
//
//    auto result = plankton::MakeVector<EExpr>(16);
//    for (const auto* memory : memories) {
//        for (const auto& field : memory->fieldToValue) {
//            if (field.second->Sort() != Sort::PTR) continue;
//            auto hasOutflow = EncodeExists(memory->flow->Sort(), [&,this](auto qv) -> EExpr {
//                auto& symbol = SymbolFactory(*memory).GetFreshFO(memory->flow->Type());
//                auto predicate = config.GetOutflowContains(*memory, field.first, symbol);
//                return Encode(*memory->flow)(qv) && Replace(Encode(*predicate), Encode(symbol), qv);
//            });
//
//            for (const auto* other : memories) {
//                if (memory == other) continue;
//                auto linked = Encode(*field.second) == Encode(*other->node);
//                auto hasInflow = Encode(*std::make_unique<InflowEmptinessAxiom>(other->flow->Decl(), false));
//                result.push_back((linked && hasOutflow) >> hasInflow);
//            }
//        }
//    }
//
//    return MakeAnd(result);
//}

//EExpr Encoding::EncodeSimpleFlowRules(const Formula& formula, const SolverConfig& config) {
//    auto memories = plankton::Collect<MemoryAxiom>(formula);
//
//    auto result = plankton::MakeVector<EExpr>(16);
//    for (const auto* memory : memories) {
//        for (const auto& field : memory->fieldToValue) {
//            if (field.second->Sort() != Sort::PTR) continue;
//            for (const auto* other : memories) {
//                if (memory == other) continue;
//                auto linked = Encode(*field.second) == Encode(*other->node);
//                auto rule = EncodeForAll(memory->flow->Sort(), [&,this](auto qv) -> EExpr {
//                    auto& symbol = SymbolFactory(*memory).GetFreshFO(memory->flow->Type());
//                    auto predicate = config.GetOutflowContains(*memory, field.first, symbol);
//
//                    auto hasFlow = Encode(*memory->flow)(qv);
//                    auto hasOutflow = Replace(Encode(*predicate), Encode(symbol), qv);
//                    auto hasInflow = Encode(*other->flow)(qv);
//                    return (hasFlow && hasOutflow) >> hasInflow;
//                });
//                result.push_back(linked >> rule);
//            }
//        }
//    }
//
//    return MakeAnd(result);
//}
