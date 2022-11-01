#include "engine/solver.hpp"

#include <map>
#include "logics/util.hpp"
#include "engine/encoding.hpp"
#include "engine/util.hpp"
#include "util/log.hpp"
#include "util/timer.hpp"

using namespace plankton;


struct InterferenceInfo {
    SymbolFactory factory;
    std::unique_ptr<Annotation> annotation;
    const std::deque<std::unique_ptr<HeapEffect>>& interference;
    std::map<SharedMemoryCore*, std::deque<const HeapEffect*>> stabilityUpdates;

    explicit InterferenceInfo(std::unique_ptr<Annotation> annotation_, const std::deque<std::unique_ptr<HeapEffect>>& interference)
            : annotation(std::move(annotation_)), interference(interference) {
        assert(annotation);
        Preprocess();
        Compute();
        Apply();
        Postprocess();
    }

    inline void Preprocess() {
        // make symbols distinct
        // TODO: we rely on effects to have distinct symbols (from one another and the annotation)
        plankton::AvoidEffectSymbols(factory, interference);
        plankton::RenameSymbols(*annotation, factory);

        // DEBUG("<<INTERFERENCE>>" << std::endl)
        // for (const auto& effect : interference) DEBUG("  -- effect: " << *effect << std::endl)
        // DEBUG(" -- pre: " << *annotation << std::endl;)
    }

    inline EExpr EncodePreHalo(Encoding& encoding, const HeapEffect& effect) const {
        auto symbolic = plankton::TryMakeSymbolic(effect.preHalo, *annotation->now, true);
        if (!symbolic) return encoding.Bool(true);
        return encoding.Encode(*symbolic);
    }

    inline void Handle(SharedMemoryCore& memory, const HeapEffect& effect, Encoding& encoding) {
        // TODO: avoid encoding the same annotation/effect multiple times
        if (memory.node->GetType() != effect.pre->node->GetType()) return;
        if (&effect.pre->node->Decl() != &effect.post->node->Decl()) throw std::logic_error("Unsupported effect"); // TODO: better error handling

        auto effectMatch = encoding.EncodeMemoryEquality(memory, *effect.pre)
                           && encoding.Encode(*effect.context)
                           && EncodePreHalo(encoding, effect);
        auto isInterferenceFree = effectMatch >> encoding.Bool(false);
        encoding.AddCheck(isInterferenceFree, [this, &memory, &effect](bool isStable) {
            if (isStable) return;
            stabilityUpdates[&memory].push_back(&effect);
        });
    }

    inline void Compute() {
        Encoding encoding;
        encoding.AddPremise(*annotation->now);
        encoding.AddPremise(encoding.TidSelf() != encoding.TidSome());
        auto resources = plankton::CollectMutable<SharedMemoryCore>(*annotation->now);
        for (auto* memory : resources) {
            for (const auto& effect : interference) {
                Handle(*memory, *effect, encoding);
            }
        }
        encoding.Check();
    }

    inline void Apply() {
        std::deque<std::unique_ptr<SharedMemoryCore>> oldMemory;
        std::deque<std::deque<std::unique_ptr<Axiom>>> candidateList;
        Encoding encoding;

        for (const auto& [axiom, effects] : stabilityUpdates) {
            if (effects.empty()) continue;
            auto newMem = plankton::MakeSharedMemory(axiom->node->Decl(), axiom->flow->GetType(), factory);
            auto postContext = encoding.EncodeMemoryEquality(*newMem, *axiom) && encoding.Encode(*annotation->now);

            for (const auto* effect : effects) {
                // auto update = encoding.Bool(true);
                auto update = encoding.EncodeMemoryEquality(*newMem, *effect->post) && encoding.Encode(*effect->context);
                auto addEquality = [&update,&encoding](const auto& var, const auto& other) {
                    update = update && encoding.Encode(var->decl) == encoding.Encode(other->decl);
                };
                if (!UpdatesFlow(*effect)) addEquality(newMem->flow, axiom->flow);
                for (auto&[field, value] : newMem->fieldToValue) {
                    if (UpdatesField(*effect, field)) continue;
                    addEquality(value, axiom->fieldToValue.at(field));
                }
                postContext = postContext || update;
            }

            Annotation dummy;
            dummy.Conjoin(plankton::Copy(*newMem));
            dummy.Conjoin(std::make_unique<PastPredicate>(plankton::Copy(*axiom)));
            candidateList.push_back(plankton::MakeStackCandidates(dummy, ExtensionPolicy::FAST));
            for (auto& candidate : candidateList.back()) {
                encoding.AddCheck(postContext >> encoding.Encode(*candidate), [this,&candidate](bool holds) {
                    if (holds) annotation->Conjoin(std::move(candidate));
                });
            }

            oldMemory.push_back(plankton::Copy(*axiom));
            axiom->flow = std::move(newMem->flow);
            axiom->fieldToValue = std::move(newMem->fieldToValue);
        }

        encoding.Check();
        for (auto& memory : oldMemory) {
            annotation->Conjoin(std::make_unique<PastPredicate>(std::move(memory)));
        }

        // DEBUG(" -- post: " << *annotation << std::endl;)
        // throw;
    }

    inline void Postprocess() {
        SymbolFactory emptyFactory;
        plankton::RenameSymbols(*annotation, emptyFactory);
    }

    inline std::unique_ptr<Annotation> GetResult() {
        return std::move(annotation);
    }
};

std::unique_ptr<Annotation> Solver::MakeInterferenceStable(std::unique_ptr<Annotation> annotation) const {
    // TODO: should this take a list of annotations?
    if (interference.empty()) return annotation;
    if (plankton::Collect<SharedMemoryCore>(*annotation->now).empty()) return annotation;

    MEASURE("Solver::MakeInterferenceStable")
    DEBUG("<<INTERFERENCE>>" << std::endl)
    plankton::ExtendStack(*annotation, config, ExtensionPolicy::FAST);
    InterferenceInfo info(std::move(annotation), interference);
    auto result = info.GetResult();
    plankton::InlineAndSimplify(*result);
    // DEBUG(*result << std::endl << std::endl)
    return result;
}