#include "engine/solver.hpp"

#include "logics/util.hpp"
#include "engine/encoding.hpp"
#include "engine/util.hpp"
#include "util/shortcuts.hpp"
#include "util/log.hpp"
#include "util/timer.hpp"

using namespace plankton;

using EffectPairDeque = std::deque<std::pair<const HeapEffect*, const HeapEffect*>>;


//
// Halo Helplers
//

void RenameHeapExpression(SymbolicHeapExpression& expr, const SymbolRenaming& renaming) {
    if (auto eq = dynamic_cast<SymbolicHeapEquality*>(&expr)) {
        plankton::RenameSymbols(*eq->lhsSymbol, renaming);
        plankton::RenameSymbols(*eq->rhs, renaming);
    } else if (auto fl = dynamic_cast<SymbolicHeapFlow*>(&expr)) {
        plankton::RenameSymbols(*fl->symbol, renaming);
    } else {
        throw std::logic_error("Internal error: failed to rename effect.");
    }
}

bool IsHeapEqual(const SymbolicHeapExpression& halo, const SymbolicHeapExpression& other) {
    if (auto haloEq = dynamic_cast<const SymbolicHeapEquality *>(&halo)) {
        if (auto otherEq = dynamic_cast<const SymbolicHeapEquality *>(&other)) {
            return plankton::SyntacticalEqual(*haloEq->lhsSymbol, *otherEq->lhsSymbol)
                   && haloEq->lhsFieldName == otherEq->lhsFieldName
                   && plankton::SyntacticalEqual(*haloEq->rhs, *otherEq->rhs);
        } else return false;
    } else if (auto haloFl = dynamic_cast<const SymbolicHeapFlow *>(&halo)) {
        if (auto otherFl = dynamic_cast<const SymbolicHeapFlow *>(&other)) {
            return plankton::SyntacticalEqual(*haloFl->symbol, *otherFl->symbol)
                   && haloFl->isEmpty == otherFl->isEmpty;
        } else return false;
    } else {
        throw std::logic_error("Internal error: failed to compare effects.");
    }
}

bool IsHeapEqual(const std::vector <std::unique_ptr<SymbolicHeapExpression>>& halo, const std::vector <std::unique_ptr<SymbolicHeapExpression>>& other) {
    if (halo.size() != other.size()) return false;
    for (std::size_t index = 0; index < halo.size(); ++index) {
        if (!IsHeapEqual(*halo.at(index), *other.at(index))) return false;
    }
    return true;
}

struct MemoryMap {
    SymbolFactory& factory;
    const Type& flowType;
    std::map<const SymbolDeclaration*, std::unique_ptr<SharedMemoryCore>> memMap;
    explicit MemoryMap(SymbolFactory& factory, const Type& flowType) : factory(factory), flowType(flowType) {}
    const SharedMemoryCore& Get(const SymbolDeclaration& decl) {
        auto find = memMap.find(&decl);
        if (find != memMap.end()) return *find->second;
        auto insertion = memMap.emplace(&decl, plankton::MakeSharedMemory(decl, flowType, factory));
        return *insertion.first->second;
    }
};

EExpr EncodeHalo(Encoding& encoding, const std::vector<std::unique_ptr<SymbolicHeapExpression>>& halo, MemoryMap& memMap) {
    auto symbolic = plankton::TryMakeSymbolic(halo, [&memMap](const SymbolDeclaration& adr) -> std::unique_ptr<MemoryAxiom> {
        return plankton::Copy(memMap.Get(adr));
    });
    if (symbolic) return encoding.Encode(*symbolic);
    throw std::logic_error("Internal error: failed to encode effect halo.");
}

EExpr EncodeMemoryEqualities(const MemoryMap& map, Encoding& encoding) {
    std::vector<EExpr> result;
    for (auto it = map.memMap.begin(); it != map.memMap.end(); ++it) {
        for (auto ot = std::next(it); ot != map.memMap.end(); ++ot) {
            auto& itAdr = it->second->node->Decl();
            auto& otAdr = ot->second->node->Decl();
            if (itAdr == otAdr) continue;
            auto sameAdr = encoding.Encode(itAdr) == encoding.Encode(otAdr);
            auto sameMem = encoding.EncodeMemoryEquality(*it->second, *ot->second);
            result.push_back(sameAdr >> sameMem);
        }
    }
    return encoding.MakeAnd(result);
}


//
// Implication among effects
//

inline bool CheckContext(const Formula& context) {
    struct : public LogicListener {
        bool result = true;
        void Enter(const LocalMemoryResource& /*object*/) override { result = false; }
        void Enter(const SharedMemoryCore& /*object*/) override { result = false; }
        // void Enter(const EqualsToAxiom& /*object*/) override { result = false; }
        void Enter(const ObligationAxiom& /*object*/) override { result = false; }
        void Enter(const FulfillmentAxiom& /*object*/) override { result = false; }
    } visitor;
    context.Accept(visitor);
    return visitor.result;
}

inline bool UpdateSubset(const HeapEffect& premise, const HeapEffect& conclusion) {
    if (plankton::UpdatesFlow(conclusion) && !plankton::UpdatesFlow(premise)) return false;
    const auto& fields = premise.pre->node->GetType().fields;
    return plankton::All(fields, [&premise, &conclusion](const auto& field) {
        return !plankton::UpdatesField(conclusion, field.first) || plankton::UpdatesField(premise, field.first);
    });
}

inline void AddEffectImplicationCheck(Encoding& encoding, const HeapEffect& premise, const HeapEffect& conclusion, std::function<void()>&& eureka) {
    // give up if context contains resources
    if (!CheckContext(*premise.context) || !CheckContext(*conclusion.context)) return;
    
    // ensure that the premise updates at least the fields updated by the conclusion
    if (!UpdateSubset(premise, conclusion)) return;

    // // don't deal with halo
    // if (!IsHeapEqual(premise.preHalo, conclusion.preHalo)) return;
    // if (!IsHeapEqual(premise.postHalo, conclusion.postHalo)) return;
    SymbolFactory factory;
    AvoidEffectSymbols(factory, premise);
    AvoidEffectSymbols(factory, conclusion);
    auto& flowType = premise.pre->flow->GetType();
    MemoryMap preMap(factory, flowType), postMap(factory, flowType);

    // encode
    auto premisePre = encoding.Encode(*premise.pre) && encoding.Encode(*premise.context);
    auto premisePost = encoding.Encode(*premise.post) && encoding.Encode(*premise.context);
    auto conclusionPre = encoding.Encode(*conclusion.pre) && encoding.Encode(*conclusion.context);
    auto conclusionPost = encoding.Encode(*conclusion.post) && encoding.Encode(*conclusion.context);
    auto premisePreHalo = EncodeHalo(encoding, premise.preHalo, preMap);
    auto premisePostHalo = EncodeHalo(encoding, premise.postHalo, postMap);
    auto conclusionPreHalo = EncodeHalo(encoding, conclusion.preHalo, preMap);
    auto conclusionPostHalo = EncodeHalo(encoding, conclusion.postHalo, postMap);
    auto samePre = encoding.EncodeMemoryEquality(*premise.pre, *conclusion.pre);
    auto samePost = encoding.EncodeMemoryEquality(*premise.post, *conclusion.post);
    auto sameHalo = EncodeMemoryEqualities(preMap, encoding) && EncodeMemoryEqualities(postMap, encoding);

    // check
    auto isImplied = ((samePre && samePost && conclusionPre) >> premisePre)
                         && ((samePre && samePost && conclusionPost) >> premisePost)
                         && ((samePre && samePost && sameHalo && conclusionPreHalo) >> premisePreHalo)
                         && ((samePre && samePost && sameHalo && conclusionPostHalo) >> premisePostHalo)
                         ; // TODO: correct?
    encoding.AddCheck(isImplied, [eureka=std::move(eureka)](bool holds) { if (holds) eureka(); });
}


inline EffectPairDeque ComputeEffectImplications(const EffectPairDeque& effectPairs) {
    EffectPairDeque result;
    Encoding encoding;
    for (const auto& pair : effectPairs) {
        auto eureka = [&result, pair]() { result.push_back(pair); };
        AddEffectImplicationCheck(encoding, *pair.first, *pair.second, std::move(eureka));
    }
    encoding.Check();
    return result;
}


//
// Adding new interference
//

inline void RenameEffect(HeapEffect& effect, SymbolFactory& factory) {
    auto renaming = plankton::MakeDefaultRenaming(factory);
    plankton::RenameSymbols(*effect.pre, renaming);
    plankton::RenameSymbols(*effect.post, renaming);
    plankton::RenameSymbols(*effect.context, renaming);
    for (auto& elem : effect.preHalo) RenameHeapExpression(*elem, renaming);
    for (auto& elem : effect.postHalo) RenameHeapExpression(*elem, renaming);
}

inline bool IsEffectEmpty(const HeapEffect& effect) {
    assert(effect.pre->node->GetType() == effect.post->node->GetType());
    if (effect.pre->flow->Decl() != effect.post->flow->Decl()) return false;
    for (const auto& [field, value] : effect.pre->fieldToValue) {
        if (value->Decl() != effect.post->fieldToValue.at(field)->Decl()) return false;
    }
    return true;
}

inline bool AreEffectsEqual(const HeapEffect& effect, const HeapEffect& other) {
    return plankton::SyntacticalEqual(*effect.pre, *other.pre)
           && plankton::SyntacticalEqual(*effect.post, *other.post)
           && plankton::SyntacticalEqual(*effect.context, *other.context)
           && IsHeapEqual(effect.preHalo, other.preHalo)
           && IsHeapEqual(effect.postHalo, other.postHalo);
}

inline void QuickFilter(std::deque<std::unique_ptr<HeapEffect>>& effects) {
    for (auto& effect : effects) {
        SymbolFactory factory;
        RenameEffect(*effect, factory);
    }

    for (auto& effect : effects) {
        if (!IsEffectEmpty(*effect)) continue;
        effect.reset(nullptr);
    }
    for (auto it = effects.begin(); it != effects.end(); ++it) {
        if (!*it) continue;
        for (auto ot = std::next(it); ot != effects.end(); ++ot) {
            if (!*ot) continue;
            if (!AreEffectsEqual(**it, **ot)) continue;
            ot->reset(nullptr);
        }
    }

    plankton::RemoveIf(effects, [](const auto& elem) { return !elem; });
}

inline void RenameEffects(std::deque<std::unique_ptr<HeapEffect>>& effects, const std::deque<std::unique_ptr<HeapEffect>>& interference) {
    SymbolFactory factory;
    plankton::AvoidEffectSymbols(factory, interference);
    for (auto& effect : effects) RenameEffect(*effect, factory);
}

void ReplaceInterfererTid(StackAxiom& axiom) {
    if (dynamic_cast<const SymbolicSelfTid*>(axiom.lhs.get())) {
        axiom.lhs = std::make_unique<SymbolicSomeTid>();
    }
    if (dynamic_cast<const SymbolicSelfTid*>(axiom.rhs.get())) {
        axiom.rhs = std::make_unique<SymbolicSomeTid>();
    }
}

void ReplaceInterfererTid(SymbolicHeapExpression& expr) {
    if (auto eqExpr = dynamic_cast<SymbolicHeapEquality*>(&expr)) {
        if (dynamic_cast<const SymbolicSelfTid*>(eqExpr->rhs.get())) {
            eqExpr->rhs = std::make_unique<SymbolicSomeTid>();
        }
    }
}

void ReplaceInterfererTid(std::deque<std::unique_ptr<HeapEffect>>& interference) {
    for (const auto& effect : interference) {
        auto axioms = plankton::CollectMutable<StackAxiom>(*effect->context);
        for (const auto& axiom: axioms) ReplaceInterfererTid(*axiom);
        for (auto& elem : effect->preHalo) ReplaceInterfererTid(*elem);
        for (auto& elem : effect->postHalo) ReplaceInterfererTid(*elem);
    }
}

bool Solver::AddInterference(std::deque<std::unique_ptr<HeapEffect>> effects) {
    DEBUG("Solver::AddInterference (" << effects.size() << ")" << std::endl)
    MEASURE("Solver::AddInterference")

    // preprocess
    ReplaceInterfererTid(effects);
    QuickFilter(effects);
    DEBUG("Number of effects after filter: " << effects.size() << std::endl)
    if (effects.empty()) return false;
    RenameEffects(effects, interference);

    auto prune = [this, &effects](const auto& pairs){
        std::set<const HeapEffect*> prune;
        auto implications = ComputeEffectImplications(pairs);

        for (const auto& [premise, conclusion] : implications) {
            if (prune.count(premise) != 0) continue;
            prune.insert(conclusion);
        }

        auto removePrunedEffects = [&prune](const auto& elem){ return plankton::Membership(prune, elem.get()); };
        plankton::RemoveIf(interference, removePrunedEffects);
        plankton::RemoveIf(effects, removePrunedEffects);
    };

    // prune new effects that are already covered
    EffectPairDeque pairsNew;
    for (auto& e : interference) for (auto& o : effects) pairsNew.emplace_back(e.get(), o.get());
    prune(pairsNew);

    // check if new effects exist
    DEBUG("Number of new effects: " << effects.size() << std::endl)
    if (effects.empty()) return false;

    // minimize effects
    EffectPairDeque pairsMin;
    for (auto& e : interference) for (auto& o : effects) pairsMin.emplace_back(e.get(), o.get());
    for (auto& e : effects) for (auto& o : interference) pairsMin.emplace_back(e.get(), o.get());
    for (auto& e : effects) for (auto& o : effects) if (e != o) pairsMin.emplace_back(e.get(), o.get());
    prune(pairsMin);
    DEBUG("Number of new effects after minimization: " << effects.size() << std::endl)
    if (effects.empty()) return false;

    INFO(std::endl << "Adding effects to solver (" << effects.size() << "): " << std::endl)
    for (const auto& effect : effects) INFO("   " << *effect << std::endl)
    INFO(std::endl)

    // add new effects
    plankton::MoveInto(std::move(effects), interference);
    return true;
}
