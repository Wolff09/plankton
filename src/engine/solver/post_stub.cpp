#include "engine/solver.hpp"

#include "util/timer.hpp"
#include "util/log.hpp"
#include "logics/util.hpp"

using namespace plankton;


PostImage Solver::Post(std::unique_ptr<Annotation> pre, const plankton::UpdateStub& cmd) const {
    MEASURE("Solver::Post (UpdateStub)")
    DEBUG("<<POST ASSUME>>" << std::endl << *pre << " " << cmd << std::endl)

    // post annotation
    plankton::InlineAndSimplify(*pre);
    SymbolFactory factory(*pre);
    auto isCmdType = [&cmd](const auto& elem) { return elem.node->GetType() == cmd.type; };
    auto memories = plankton::CollectMutable<MemoryAxiom>(*pre->now, isCmdType);
    std::unique_ptr<SharedMemoryCore> effectPre, effectPost;
    for (auto* memory : memories) {
        assert(memory->fieldToValue.count(cmd.field) != 0);
        auto sharedMemory = dynamic_cast<const SharedMemoryCore*>(memory);
        bool chooseThis = !effectPre && sharedMemory;
        if (chooseThis) effectPre = plankton::Copy(*sharedMemory);
        auto& value = memory->fieldToValue.at(cmd.field);
        value->decl = factory.GetFresh(value->GetType(), value->GetOrder());
        if (chooseThis) effectPost = plankton::Copy(*sharedMemory);
    }

    // effect
    std::deque<std::unique_ptr<HeapEffect>> effects;
    if (effectPre) {
        auto emptyContext = std::make_unique<SeparatingConjunction>();
        auto effect = std::make_unique<HeapEffect>(std::move(effectPre), std::move(effectPost), std::move(emptyContext));
        effects.push_back(std::move(effect));
    }

    DEBUG(*pre << std::endl << std::endl)
    if (!effects.empty()) DEBUG(*effects.front() << std::endl)
    return PostImage(std::move(pre), std::move(effects));
}
