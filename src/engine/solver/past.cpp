#include "engine/solver.hpp"

#include "logics/util.hpp"
#include "engine/util.hpp"
#include "engine/encoding.hpp"
#include "util/shortcuts.hpp"
#include "util/timer.hpp"
#include "util/log.hpp"

using namespace plankton;

constexpr const bool INTERPOLATE_POINTERS_ONLY = true;


inline Encoding MakeEncoding(const Annotation& annotation, const SolverConfig& config) {
    Encoding encoding(*annotation.now, config);
    encoding.AddPremise(encoding.EncodeInvariants(*annotation.now, config));
    // encoding.AddPremise(encoding.EncodeSimpleFlowRules(*annotation.now, config));
    for (const auto& elem : annotation.past)
        encoding.AddPremise(encoding.EncodeInvariants(*elem->formula, config));
    return encoding;
}

inline std::unique_ptr<SeparatingConjunction> ExtractKnowledge(const std::set<const SymbolDeclaration*>& search, const Formula& from) {
    struct ContextCollector : public LogicListener {
        const std::set<const SymbolDeclaration*>& search;
        std::unique_ptr<SeparatingConjunction> knowledge;
        explicit ContextCollector(const std::set<const SymbolDeclaration *> &search)
                : search(search), knowledge(std::make_unique<SeparatingConjunction>()) {}
        inline void Handle(const Axiom& object) {
            if (plankton::EmptyIntersection(search, plankton::Collect<SymbolDeclaration>(object))) return;
            knowledge->Conjoin(plankton::Copy(object));
        }
        void Enter(const EqualsToAxiom& object) override { Handle(object); }
        void Enter(const StackAxiom& object) override { Handle(object); }
        void Enter(const InflowEmptinessAxiom& object) override { Handle(object); }
        void Enter(const InflowContainsValueAxiom& object) override { Handle(object); }
        void Enter(const InflowContainsRangeAxiom& object) override { Handle(object); }
    } collector(search);
    from.Accept(collector);
    return std::move(collector.knowledge);
}

// inline std::unique_ptr<SeparatingConjunction> ExtractKnowledge(const MemoryAxiom& axiom, const Formula& from) {
//     std::set<const SymbolDeclaration*> search;
//     search.insert(&axiom.flow->Decl());
//     for (const auto& pair : axiom.fieldToValue) search.insert(&pair.second->Decl());
//     return ExtractKnowledge(search, from);
// }

inline std::unique_ptr<SeparatingConjunction> ExtractKnowledge(const SymbolDeclaration& symbol, const Formula& from) {
    std::set<const SymbolDeclaration*> search;
    search.insert(&symbol);
    return ExtractKnowledge(search, from);
}

inline std::unique_ptr<SeparatingConjunction> MakeStackDuplicable(std::unique_ptr<SeparatingConjunction> formula) {
    formula->RemoveConjunctsIf([](const auto& elem){ return dynamic_cast<const EqualsToAxiom*>(&elem); });
    return formula;
}

inline std::unique_ptr<StackAxiom> MakeEq(const SymbolDeclaration& decl, const SymbolDeclaration& other) {
    return std::make_unique<StackAxiom>(
            BinaryOperator::EQ, std::make_unique<SymbolicVariable>(decl), std::make_unique<SymbolicVariable>(other)
    );
}


//
// Filter
//

//inline std::optional<EExpr>
inline std::unique_ptr<SeparatingConjunction>
MakeSubsumptionCheck(Encoding& encoding, const Formula& formula, const MemoryAxiom& weaker, const MemoryAxiom& stronger) {
    //// TODO: why does it not work as intended??
    //if (&stronger == &weaker) return std::nullopt;
    //if (weaker.node->Decl() != stronger.node->Decl()) return std::nullopt;
    //auto renaming = plankton::MakeMemoryRenaming(stronger, weaker);
    //auto renamed = plankton::Copy(formula);
    //plankton::RenameSymbols(*renamed, renaming);
    ////DEBUG("  past subsumption check" << std::endl << "     for " << stronger << std::endl << "      => " << weaker << std::endl << "     chk: " << *renamed << std::endl)
    //return encoding.Encode(*renamed);

    //if (&stronger == &weaker) return std::nullopt;
    //if (weaker.node->Decl() != stronger.node->Decl()) return std::nullopt;
    //auto strongerInfo = ExtractKnowledge(stronger, formula);
    //auto check = ExtractKnowledge(weaker, formula);
    ////auto renaming = plankton::MakeMemoryRenaming(stronger, weaker);
    ////plankton::RenameSymbols(*strongerInfo, renaming);
    //DEBUG("#### past subsumption check: " << stronger << "  ==>  " << weaker << std::endl << "        " << *strongerInfo << std::endl << "     => " << std::endl << "        " << *check << std::endl)
    //auto context = encoding.EncodeMemoryEquality(stronger, weaker);
    //return (encoding.Encode(*strongerInfo) && context) >> encoding.Encode(*check);

    //auto check = ExtractKnowledge(weaker, formula);
    //auto renaming = plankton::MakeMemoryRenaming(weaker, stronger);
    //plankton::RenameSymbols(*check, renaming);

    if (&stronger == &weaker) return nullptr;
    if (weaker.node->Decl() != stronger.node->Decl()) return nullptr;
    auto check = std::make_unique<SeparatingConjunction>();
    auto handle = [&check,&formula](const auto& weakerDecl, const auto& strongerDecl) {
        auto fieldCheck = ExtractKnowledge(weakerDecl, formula);
        auto renaming = [&](const auto& decl) -> const SymbolDeclaration& { return decl == weakerDecl ? strongerDecl : decl; };
        plankton::RenameSymbols(*fieldCheck, renaming);
        check->Conjoin(std::move(fieldCheck));
    };
    handle(weaker.flow->Decl(), stronger.flow->Decl());
    for (const auto& [field, value] : weaker.fieldToValue) {
        handle(value->Decl(), stronger.fieldToValue.at(field)->Decl());
    }
    return check;
}

void FilterPasts(Annotation& annotation, const SolverConfig& config) {
    MEASURE("Solver::PrunePast ~> FilterPasts")
    if (annotation.past.empty()) return;
    DEBUG("<<REDUCE PAST>>" << std::endl)

    // // TODO: is this clever? ~~> no it is not... :*(
    // plankton::RemoveIf(annotation.past,[&annotation](const auto& elem){
    //    return !plankton::TryGetResource(elem->formula->node->Decl(), *annotation.now);
    // });
    // if (annotation.past.empty()) return;

    plankton::InlineAndSimplify(annotation);
    auto encoding = MakeEncoding(annotation, config);
    //DEBUG("## FILTER " << annotation << std::endl)

    std::map<const PastPredicate*, std::set<const PastPredicate*>> subsumes;
    std::deque<std::unique_ptr<Formula>> store;
    auto newKnowledge = std::make_unique<SeparatingConjunction>();
    for (const auto& stronger : annotation.past) {
        for (const auto& weaker : annotation.past) {
            auto check = MakeSubsumptionCheck(encoding, *annotation.now, *weaker->formula, *stronger->formula);
            if (!check) continue;
            store.push_back(std::move(check));
            auto& form = store.back();
            encoding.AddCheck(encoding.Encode(*form), [&](bool holds) {
               //DEBUG("  past subsumption=" << holds << std::endl << "     for " << *stronger << std::endl << "      => " << *weaker << std::endl)
               if (!holds) return;
               subsumes[stronger.get()].insert(weaker.get());
               newKnowledge->Conjoin(std::move(form));
            });
            // if (encoding.Implies(*check)) {
            //     //DEBUG("#### past subsumption holds for: " << std::endl << "          " << *stronger << std::endl << "     ==>  " << *weaker << std::endl << "          Reason: " << std::endl << "          " << *check << std::endl)
            //     subsumes[stronger.get()].insert(weaker.get());
            //     newKnowledge->Conjoin(std::move(check));
            // }
        }
    }
    encoding.Check();

    for (const auto& past : annotation.past) {
        if (!past) continue;
        auto subsumption = subsumes[past.get()];
        if (subsumption.empty()) continue;
        for (auto& other : annotation.past) {
            if (!other) continue;
            if (past.get() == other.get()) continue;
            if (!plankton::Membership(subsumption, other.get())) continue;
            other.reset(nullptr);
        }
    }
    plankton::RemoveIf(annotation.past, [](const auto& elem){ return !elem; });

    plankton::Simplify(*newKnowledge);
    newKnowledge = MakeStackDuplicable(std::move(newKnowledge));
    annotation.Conjoin(std::move(newKnowledge));
    plankton::InlineAndSimplify(annotation);
}

void Solver::ReducePast(Annotation& annotation) const {
    MEASURE("Solver::PrunePast")
    // DEBUG("<<REDUCE PAST>>" << std::endl)
    // DEBUG(annotation << std::endl)
    FilterPasts(annotation, config);
    // DEBUG(" ~> " << annotation << std::endl << std::endl)
}

std::unique_ptr<Annotation> Solver::ReducePast(std::unique_ptr<Annotation> annotation) const {
    ReducePast(*annotation);
    // DEBUG(*annotation << std::endl << std::endl)
    return annotation;
}


//
// Interpolate
//

inline std::set<const SymbolDeclaration*> GetActiveReferences(const Annotation& annotation) {
    std::set<const SymbolDeclaration*> result;
    for (const auto* variable : plankton::Collect<EqualsToAxiom>(*annotation.now)) {
        result.insert(&variable->Value());
    }
    return result;
}

using ImmutabilityLookup = std::map<std::pair<const SymbolDeclaration*, std::string>, const SymbolDeclaration*>;

inline ImmutabilityLookup
MakeImmutabilityLookup(const Annotation& annotation, const std::deque<std::unique_ptr<HeapEffect>>& interference) {
    std::map<std::pair<const SymbolDeclaration*, std::string>, const SymbolDeclaration*> result;
    auto memories = plankton::Collect<SharedMemoryCore>(*annotation.now);
    for (const auto* memory : memories) {
        for (const auto& [field, value] : memory->fieldToValue) {
            if (plankton::Any(interference, [field=field](const auto& elem){
                return elem->pre->fieldToValue.at(field)->Decl() != elem->post->fieldToValue.at(field)->Decl();
            })) continue;
            result[{&memory->node->Decl(), field}] = &value->Decl();
        }
    }
    return result;
}

struct Interpolator {
    const std::deque<std::unique_ptr<HeapEffect>>& interference;
    const SolverConfig& config;
    Annotation& annotation;
    SymbolFactory factory;
    std::optional<ImmutabilityLookup> immutability;

    Interpolator(Annotation& annotation, const std::deque<std::unique_ptr<HeapEffect>>& interference,
                 const SolverConfig& config) : interference(interference), config(config), annotation(annotation) {
        plankton::Simplify(annotation);
        plankton::AvoidEffectSymbols(factory, interference);
        plankton::RenameSymbols(annotation, factory);
        for (auto& elem : annotation.past) ApplyImmutability(*elem->formula, annotation.now.get());
    }

    void Interpolate() {
        Filter();
        ExpandHistoryMemory();
        // DEBUG(" && after first expansion: " << annotation << std::endl)
        Filter();
        // DEBUG(" && after first filter: " << annotation << std::endl)
        InterpolatePastToNow();
        AddTrivialPast();
        // DEBUG(" && after interpolation: " << annotation << std::endl)
        Filter();
        // DEBUG(" && after second filter: " << annotation << std::endl)
        // ExpandHistoryMemory();
        // // DEBUG(" && after last expansion: " << annotation << std::endl)
        // Filter();
        // // DEBUG(" && after last filter: " << annotation << std::endl)
        PostProcess();
    }

    inline void ApplyImmutability(MemoryAxiom& memory, SeparatingConjunction* addEq = nullptr) {
        if (!immutability) immutability = MakeImmutabilityLookup(annotation, interference);
        for (auto& [field, value] : memory.fieldToValue) {
            assert(immutability);
            auto newValue = immutability.value()[{ &memory.node->Decl(), field }];
            if (!newValue) continue;
            if (addEq) addEq->Conjoin(MakeEq(*newValue, value->Decl()));
            value->decl = *newValue;
        }
    }

    void Filter() {
        FilterPasts(annotation, config);
        immutability = std::nullopt; // needs to be recomputed since FilterPasts may change symbols
    }

    inline void AddDerivedCandidates(Encoding& encoding, std::deque<std::unique_ptr<Axiom>> candidates) {
        for (auto& candidate : candidates) {
            encoding.AddCheck(encoding.Encode(*candidate), [this,&candidate](bool holds){
                if (holds) annotation.Conjoin(std::move(candidate));
            });
        }
        encoding.Check();
    }

    [[nodiscard]] inline std::unique_ptr<Annotation> MakeStackBlueprint() const {
        auto result = std::make_unique<Annotation>(plankton::Copy(*annotation.now));
        plankton::RemoveIf(result->now->conjuncts, [](const auto& conjunct){
            return dynamic_cast<const SharedMemoryCore*>(conjunct.get());
        });
        return result;
    }

    [[nodiscard]] inline std::set<const SymbolDeclaration*> GetPointerFields(const SharedMemoryCore& memory) const {
        return plankton::Collect<SymbolDeclaration>(memory, [](auto& obj){
            return obj.type.sort == Sort::PTR;
        });
    }

    void ExpandHistoryMemory() {
        MEASURE("Solver::ImprovePast ~> ExpandHistoryMemory")

        auto& flowType = config.GetFlowValueType();
        auto pasts = std::move(annotation.past);
        annotation.past.clear();

        auto referenced = GetActiveReferences(annotation);
        auto getExpansion = [this,&referenced](const auto& past) {
            auto ptrFields = GetPointerFields(*past.formula);
            for (auto it = ptrFields.begin(); it != ptrFields.end();) {
                if (plankton::Membership(referenced, *it)) ++it;
                else it = ptrFields.erase(it);
            }
            return ptrFields;
        };

        for (auto& elem : pasts) {
            auto& pastAddress = elem->formula->node->Decl();
            // if (!plankton::Membership(referenced, &pastAddress)) continue;

            auto expansion = getExpansion(*elem);
            // DEBUG("  -- expanding { ") DEBUG_FOREACH(expansion, DEBUG(elem->name << " ")) DEBUG(" } for " << *elem << std::endl)
            auto past = MakeStackBlueprint();
            past->Conjoin(std::move(elem->formula));

            Encoding encoding = MakeEncoding(*past, config);
            plankton::MakeMemoryAccessible(*past->now, expansion, flowType, factory, encoding);
            encoding = MakeEncoding(*past, config); // add the invariants for the newly accessible memory

            std::deque<std::unique_ptr<Axiom>> candidates;
            for (auto memory : plankton::CollectMutable<SharedMemoryCore>(*past->now)) {
                ApplyImmutability(*memory, past->now.get());
                annotation.Conjoin(std::make_unique<PastPredicate>(plankton::Copy(*memory)));

                if (memory->node->Decl() == pastAddress) continue;
                auto memoryCandidates = plankton::MakeStackCandidates(*past->now, *memory, ExtensionPolicy::FAST);
                plankton::MoveInto(std::move(memoryCandidates), candidates);
                plankton::MoveInto(plankton::MakeStackCandidates(*memory, ExtensionPolicy::FAST), candidates);
            }

            if (candidates.empty()) continue;
            encoding.AddPremise(*past->now);
            encoding.AddPremise(encoding.EncodeInvariants(*past->now, config));
            encoding.AddPremise(encoding.EncodeSimpleFlowRules(*past->now, config));
            // AddDerivedCandidates(encoding, plankton::MakeStackCandidates(*past, ExtensionPolicy::FAST));
            AddDerivedCandidates(encoding, std::move(candidates));
        }
    }

    [[nodiscard]] inline std::deque<std::unique_ptr<Axiom>> MakeInterpolationCandidates(const SharedMemoryCore& memory) const {
        std::deque<std::unique_ptr<Axiom>> result;
        for (const auto& effect : interference) {
            auto axioms = plankton::Collect<Axiom>(*effect->context);
            auto preRenaming = plankton::MakeMemoryRenaming(*effect->pre, memory);
            auto postRenaming = plankton::MakeMemoryRenaming(*effect->post, memory);

            for (const auto* axiom : axioms) {
                auto candidate = plankton::Copy(*axiom);
                plankton::RenameSymbols(*candidate, preRenaming);
                plankton::RenameSymbols(*candidate, postRenaming);
                result.push_back(std::move(candidate));
            }
        }
        return result;
    }

    inline void InterpolateEffectPastToNow(const SharedMemoryCore& past, const std::string& field, const SymbolDeclaration& interpolatedValue) {
        auto& pastValue = past.fieldToValue.at(field)->Decl();
        if (interpolatedValue == pastValue) return;

        auto newHistory = plankton::MakeSharedMemory(past.node->Decl(), config.GetFlowValueType(), factory);
        ApplyImmutability(*newHistory);
        newHistory->fieldToValue.at(field)->decl = interpolatedValue;

        auto encoding = MakeEncoding(*MakeStackBlueprint(), config);
        encoding.AddPremise(encoding.EncodeInvariants(*newHistory, config));

        auto vector = plankton::MakeVector<EExpr>(interference.size() + 1);
        vector.push_back( // 'field' was never changed
                encoding.Encode(interpolatedValue) == encoding.Encode(pastValue) &&
                encoding.EncodeMemoryEquality(past, *newHistory)
        );
        for (const auto& effect : interference) {
            if (!plankton::UpdatesField(*effect, field)) continue;
            auto& effectValue = effect->post->fieldToValue.at(field)->Decl();
            vector.push_back( // last update to 'field' is due to 'effect'
                    encoding.Encode(interpolatedValue) == encoding.Encode(effectValue) &&
                    encoding.Encode(*effect->context) &&
                    encoding.EncodeMemoryEquality(*effect->post, *newHistory)
            );
        }
        encoding.AddPremise(encoding.MakeOr(vector));

        AddDerivedCandidates(encoding, MakeInterpolationCandidates(*newHistory));
        // AddDerivedCandidates(encoding, plankton::MakeStackCandidates(*annotation.now, *newHistory, ExtensionPolicy::FAST));
        annotation.Conjoin(std::make_unique<PastPredicate>(std::move(newHistory)));
    }

    inline void InterpolateDeepPastToNow(const SharedMemoryCore& past, const std::string& field, const SymbolDeclaration& interpolatedValue, SeparatingConjunction& newNowKnowledge) {
        //DEBUG("INTERPOL (deep): [past] " << past << "  [field] " << field << "  [interpolatedValue] " << interpolatedValue << std::endl)
        auto pastKnowledge = std::make_unique<SeparatingConjunction>();
        pastKnowledge->Conjoin(MakeStackDuplicable(ExtractKnowledge(past.flow->Decl(), *annotation.now)));
        for (const auto& pair : past.fieldToValue)
            if (pair.second->GetSort() != Sort::PTR)
                pastKnowledge->Conjoin(MakeStackDuplicable(ExtractKnowledge(pair.second->Decl(), *annotation.now)));
        plankton::Simplify(*pastKnowledge);
        // auto pastKnowledge = MakeStackDuplicable(ExtractKnowledge(past, *annotation.now));
        if (pastKnowledge->conjuncts.empty()) return; //{ DEBUG("  -- nothing to be done" << std::endl) return; }
        //DEBUG("  -- working relative to: " << *pastKnowledge << std::endl)

        Encoding encoding;
        encoding.EncodeInvariants(past, config);
        auto mkKnowledgeRaw = [&pastKnowledge,&past](const auto& with){
            auto copy = plankton::Copy(*pastKnowledge);
            auto renaming = plankton::MakeMemoryRenaming(past, with);
            plankton::RenameSymbols(*copy, renaming);
            return copy;
        };
        auto mkKnowledge = [&encoding,&mkKnowledgeRaw](const auto& with){
            auto knowledge = mkKnowledgeRaw(with);
            return encoding.Encode(*knowledge);
        };
        auto mkContext = [this,&encoding](const auto& effect) {
            return encoding.Encode(*effect.pre) && encoding.EncodeInvariants(*effect.pre, config)
                   && encoding.Encode(*effect.post) && encoding.EncodeInvariants(*effect.post, config)
                   && encoding.Encode(*effect.context); // TODO: correct?
        };

        // interpolation variant 1
        auto vector = plankton::MakeVector<EExpr>(interference.size());
        for (const auto& effect : interference) {
            // { J /\ H /\ !F } com { H }
            auto interpolant = encoding.Encode(*effect->pre->fieldToValue.at(field)) == encoding.Encode(interpolatedValue);
            auto pre = mkKnowledge(*effect->pre);
            auto post = mkKnowledge(*effect->post);
            auto context = mkContext(*effect);
            vector.push_back((context && pre && !interpolant) >> (post));
        }

        // interpolation variant 2
        auto other = plankton::MakeVector<EExpr>(interference.size());
        for (const auto& effect : interference) {
            // { J /\ !F } com { !F \/ H }
            auto preInterpolant = encoding.Encode(*effect->pre->fieldToValue.at(field)) == encoding.Encode(interpolatedValue);
            auto postInterpolant = encoding.Encode(*effect->post->fieldToValue.at(field)) == encoding.Encode(interpolatedValue);
            auto post = mkKnowledge(*effect->post);
            auto context = mkContext(*effect);
            other.push_back((context && !preInterpolant) >> (!postInterpolant || post));
        }

        // interpolate: history between existing history and now
        auto interpolation = encoding.MakeAnd(vector) || encoding.MakeAnd(other);
        if (!encoding.Implies(interpolation)) return; //{ DEBUG("  -- cannot interpolate" << std::endl) return; }
        auto newHistory = plankton::Copy(past);
        newHistory->fieldToValue.at(field)->decl = interpolatedValue;
        //DEBUG("  -- interpolating results in history: " << *newHistory << std::endl)
        annotation.Conjoin(std::make_unique<PastPredicate>(std::move(newHistory)));

        // interpolate: bring history to the now
        auto nowVec = plankton::MakeVector<EExpr>(interference.size());
        for (const auto& effect : interference) {
            // { J /\ !F } com { !F }
            // { J /\ H /\ F } com { H \/ !F }
            auto pre = mkKnowledge(*effect->pre);
            auto post = mkKnowledge(*effect->post);
            auto preInterpolant = encoding.Encode(*effect->pre->fieldToValue.at(field)) == encoding.Encode(interpolatedValue);
            auto postInterpolant = encoding.Encode(*effect->post->fieldToValue.at(field)) == encoding.Encode(interpolatedValue);
            auto context = mkContext(*effect);
            nowVec.push_back((context && !preInterpolant) >> (!postInterpolant));
            nowVec.push_back((context && pre && preInterpolant) >> (post || !postInterpolant));
        }
        if (encoding.Implies(encoding.MakeAnd(nowVec))) {
            if (auto now = plankton::TryGetResource(past.node->Decl(), *annotation.now)) {
                auto intoNow = MakeStackDuplicable(mkKnowledgeRaw(*now));
                //DEBUG("  -- interpolating results in now: " << *intoNow << std::endl)
                newNowKnowledge.Conjoin(std::move(intoNow));
            } //else DEBUG("  -- cannot find now memory" << std::endl)
        } //else DEBUG("  -- cannot bring to now" << std::endl)
    }

    //void InterpolatePastToNow() {
    //    MEASURE("Solver::ImprovePast ~> InterpolatePastToNow")
    //    auto referenced = GetActiveReferences(annotation);
    //    for (const auto* nowMem : plankton::Collect<SharedMemoryCore>(*annotation.now)) {
    //        if (!plankton::Membership(referenced, &nowMem->node->Decl())) continue;
    //        for (const auto& past : annotation.past) {
    //            if (nowMem->node->Decl() != past->formula->node->Decl()) continue;
    //            for (const auto& [field, value] : nowMem->fieldToValue) {
    //                if (INTERPOLATE_POINTERS_ONLY && value->Sort() != Sort::PTR) continue;
    //                InterpolatePastToNow(*past->formula, field, value->Decl());
    //            }
    //        }
    //    }
    //}

    void InterpolatePastToNow() {
        MEASURE("Solver::ImprovePast ~> InterpolatePastToNow")
        //DEBUG("INTERPOLATING... " << annotation << std::endl)
        auto referenced = GetActiveReferences(annotation);
        auto nowMemories = plankton::Collect<SharedMemoryCore>(*annotation.now);
        auto newNowKnowledge = std::make_unique<SeparatingConjunction>();
        for (const auto& past : annotation.past) {
            if (!plankton::Membership(referenced, &past->formula->node->Decl())) continue;
            // std::set<const SymbolDeclaration*> interpolated;
            for (const auto* nowMem : nowMemories) {
                if (nowMem->node->Decl() != past->formula->node->Decl()) continue;
                for (const auto& [field, value] : nowMem->fieldToValue) {
                    // if (INTERPOLATE_POINTERS_ONLY && value->GetSort() != Sort::PTR) continue;
                    if (value->GetSort() == Sort::PTR && !plankton::Membership(referenced, &value->Decl())) continue;
                    // if (plankton::Membership(interpolated, &value->Decl())) continue;
                    // if (value->Decl() == past->formula->fieldToValue.at(field)->Decl()) continue;
                    InterpolateEffectPastToNow(*past->formula, field, value->Decl()); // TODO: still needed?
                    InterpolateDeepPastToNow(*past->formula, field, value->Decl(), *newNowKnowledge);
                    // interpolated.insert(&value->Decl());
                }
            }
        }
        // if (!newNowKnowledge->conjuncts.empty()) DEBUG("DISCARDING new now knowledge: " << *newNowKnowledge << std::endl)
        annotation.now->Conjoin(std::move(newNowKnowledge));
    }

    void AddTrivialPast() {
        for (const auto* memory : plankton::Collect<SharedMemoryCore>(*annotation.now)) {
            annotation.Conjoin(std::make_unique<PastPredicate>(plankton::Copy(*memory)));
        }
    }

    void PostProcess() {
        plankton::InlineAndSimplify(annotation);
    }

};

std::unique_ptr<Annotation> Solver::ImprovePast(std::unique_ptr<Annotation> annotation) const {
    MEASURE("Solver::ImprovePast")
    if (annotation->past.empty()) return annotation;
    DEBUG("<<IMPROVE PAST>>" << std::endl)
    Interpolator(*annotation, interference, config).Interpolate();
    //DEBUG(*annotation << std::endl << std::endl)
    return annotation;
}
