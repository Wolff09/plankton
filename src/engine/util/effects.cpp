#include "engine/util.hpp"
#include "util/shortcuts.hpp"
#include "logics/util.hpp"

using namespace plankton;


bool plankton::UpdatesFlow(const HeapEffect& effect) {
    return effect.pre->flow->Decl() != effect.post->flow->Decl();
}

bool plankton::UpdatesField(const HeapEffect& effect, const std::string& field) {
    return effect.pre->fieldToValue.at(field)->Decl() != effect.post->fieldToValue.at(field)->Decl();
}

struct EffectHeapSymbolCollector : public EffectVisitor {
    std::set<const SymbolDeclaration*> result;
    void Visit(const plankton::SymbolicHeapEquality &obj) override {
        result.insert(&obj.lhsSymbol->Decl());
        plankton::InsertInto(plankton::Collect<SymbolDeclaration>(*obj.rhs), result);
    }
    void Visit(const plankton::SymbolicHeapFlow &obj) override {
        result.insert(&obj.symbol->Decl());
    }
};

void plankton::AvoidEffectSymbols(SymbolFactory& factory, const HeapEffect& effect) {
    factory.Avoid(*effect.pre);
    factory.Avoid(*effect.post);
    factory.Avoid(*effect.context);

    EffectHeapSymbolCollector collector;
    for (const auto& elem : effect.preHalo) elem->Accept(collector);
    for (const auto& elem : effect.postHalo) elem->Accept(collector);
    for (const auto* symbol : collector.result) {
        SymbolicVariable dummy(*symbol);
        factory.Avoid(dummy);
    }
}

void plankton::AvoidEffectSymbols(SymbolFactory& factory, const std::deque<std::unique_ptr<HeapEffect>>& effects) {
    for (const auto& effect : effects) plankton::AvoidEffectSymbols(factory, *effect);
}

