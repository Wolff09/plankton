#include "engine/solver.hpp"

#include "logics/util.hpp"
#include "engine/util.hpp"
#include "util/shortcuts.hpp"

using namespace plankton;


struct FutureInfo {
    const Annotation& annotation;
    const SymbolDeclaration& address;
    std::map<std::string, std::unique_ptr<SymbolicExpression>> updates;
    std::deque<const FuturePredicate*> matchingFutures;
    SymbolFactory factory;

    explicit FutureInfo(const Annotation& annotation, const SymbolDeclaration& address) : annotation(annotation), address(address) {}
};

inline std::optional<FutureInfo> MakeFutureInfo(const Annotation& annotation, const MemoryWrite& cmd) {
    assert(cmd.lhs.size() == cmd.rhs.size());
    assert(!cmd.lhs.empty());
    const auto& now = *annotation.now;

    // ensure all updates target same memory location
    const auto& variable = cmd.lhs.front()->variable->Decl();
    auto differentVariable = [&variable](const auto& deref) { return deref->variable->Decl() != variable; };
    if (plankton::Any(cmd.lhs, differentVariable)) return std::nullopt;

    // ensure variable is accessible
    const auto* resource = plankton::TryGetResource(variable, now);
    FutureInfo result(annotation, resource->Value());

    // extract updates
    for (std::size_t index = 0; index < cmd.lhs.size(); ++index) {
        auto newValue = plankton::TryMakeSymbolic(*cmd.rhs.at(index), now);
        if (!newValue) return std::nullopt;
        auto insertion = result.updates.emplace(cmd.lhs.at(index)->fieldName, std::move(newValue));
        assert(insertion.second);
    }

    // extract matching futures
    for (const auto& future : annotation.future) {
        // TODO: match context? associate futures with the command they are made for?
        assert(future->pre->node->Decl() == future->post->node->Decl());
        if (future->pre->node->Decl() != result.address) continue;
        result.matchingFutures.push_back(future.get());
    }

    return std::move(result);
}

inline bool IsCovered(const FutureInfo& info) {
    Encoding encoding(*info.annotation.now);
    return plankton::Any(info.matchingFutures, [&info,&encoding](const auto* future){
        assert(future->pre->node->Decl() == info.address);
        for (const auto& [field, value] : info.updates) {
            auto equal = encoding.Encode(*value) == encoding.Encode(future->post->fieldToValue.at(field)->Decl());
            if (!encoding.Implies(equal)) return false;
        }
        return true;
    });
}

inline std::unique_ptr<FuturePredicate> MakeTrivialFuture(FutureInfo& info, const SolverConfig& config, const UnboundedUpdate& target) {
    auto pre = plankton::MakeSharedMemory(info.address, config.GetFlowValueType(), info.factory);
    auto post = plankton::Copy(*pre);
    auto context = std::make_unique<SeparatingConjunction>();
    for (const auto& condition : target.guards) {
        auto lhs = plankton::TryMakeSymbolic(*condition->lhs, *info.annotation.now);
        auto rhs = plankton::TryMakeSymbolic(*condition->rhs, *info.annotation.now);
        if (!lhs || !rhs) continue;
        context->Conjoin(std::make_unique<StackAxiom>(condition->op, std::move(lhs), std::move(rhs)));
    }
    auto future = std::make_unique<FuturePredicate>(std::move(pre), std::move(post), std::move(context));
    info.matchingFutures.push_back(future.get());
    return future;
}

inline std::unique_ptr<SeparatingConjunction> ExtractStack(const Annotation& annotation) {
    struct : public LogicListener {
        std::set<const Axiom*> axioms;
        void Visit(const EqualsToAxiom& object) override { axioms.insert(&object); }
        void Visit(const StackAxiom& object) override { axioms.insert(&object); }
        void Visit(const InflowEmptinessAxiom& object) override { axioms.insert(&object); }
        void Visit(const InflowContainsValueAxiom& object) override { axioms.insert(&object); }
        void Visit(const InflowContainsRangeAxiom& object) override { axioms.insert(&object); }
    } collector;
    annotation.now->Accept(collector);

    auto result = std::make_unique<SeparatingConjunction>();
    for (const auto* axiom : collector.axioms) result->Conjoin(plankton::Copy(*axiom));
    return result;
}

inline std::pair<const SharedMemoryCore*, const SharedMemoryCore*>
GetImmutabilityModification(const Annotation& annotation, const SymbolDeclaration& address) {
    auto now = dynamic_cast<const SharedMemoryCore*>(plankton::TryGetResource(address, *annotation.now));
    const SharedMemoryCore* past;
    for (const auto& elem : annotation.past) {
        if (elem->formula->node->Decl() != address) continue;
        past = elem->formula.get();
        break;
    }
    if (now && past) return { now, past };
    throw std::logic_error("Internal error: failed to make stable memory."); // TODO: better error handling
}

inline std::unique_ptr<SharedMemoryCore>
MakeImmutable(const SharedMemoryCore& memory, const Formula& context, const Solver& solver, SymbolFactory& factory) {
    auto annotation = std::make_unique<Annotation>();
    annotation->Conjoin(plankton::Copy(memory));
    annotation->Conjoin(plankton::Copy(context));
    plankton::Simplify(*annotation);
    annotation = solver.MakeInterferenceStable(std::move(annotation));

    // TODO: this is really ugly and fragile
    auto result = plankton::Copy(memory);
    auto [now, past] = GetImmutabilityModification(*annotation, memory.node->Decl());
    auto makeImmutable = [&factory](auto& sym){ sym->decl = factory.GetFresh(sym->Type(), sym->Order()); };
    if (now->flow->Decl() != past->flow->Decl()) makeImmutable(result->flow);
    for (const auto& [field, value] : now->fieldToValue) {
        if (value->Decl() != past->fieldToValue.at(field)->Decl()) makeImmutable(result->fieldToValue.at(field));
    }

    return result;
}

inline std::unique_ptr<SeparatingConjunction> ExtractImmutableState(FutureInfo& info, const Solver& solver) {
    auto memories = plankton::Collect<SharedMemoryCore>(*info.annotation.now);
    for (const auto& past : info.annotation.past) memories.insert(past->formula.get());

    auto stack = ExtractStack(info.annotation);
    auto result = std::make_unique<Annotation>();
    for (const auto* mem : memories) {
        result->Conjoin(MakeImmutable(*mem, *stack, solver, info.factory));
    }

    // for (const auto* mem : memories) {
    //     auto immutable = plankton::Copy(*mem);
    //     immutable->flow->decl = info.factory.GetFreshSO(immutable->flow->Type());
    //     if (!solver.IsMemoryImmutable(*immutable, *stack)) continue;
    //     result->Conjoin(std::move(immutable));
    // }

    plankton::InlineAndSimplify(*result);
    return std::move(result->now);
}

PostImage Solver::ImproveFuture(std::unique_ptr<Annotation> pre, const UnboundedUpdate& target) const {
    PostImage result(std::move(pre));
    auto& annotation = *result.annotations.front();

    plankton::InlineAndSimplify(annotation);
    auto info = MakeFutureInfo(annotation, *target.command);
    if (!info || IsCovered(*info)) return result;

    annotation.Conjoin(MakeTrivialFuture(*info, config, target));
    auto immutable = ExtractImmutableState(*info, *this);

    for (const auto* future : info->matchingFutures) {
        auto check = std::make_unique<Annotation>();
        check->Conjoin(plankton::Copy(*future->post));
        check->Conjoin(plankton::Copy(*future->context));
        check->Conjoin(plankton::Copy(*immutable));

        std::optional<PostImage> post;
        try {
            post = Post(std::move(check), *target.command);
        } catch (std::logic_error& err) { // TODO: catch proper error
            continue;
        }

        bool foundPost = false;
        for (auto& postAnnotation : post->annotations) {
            auto& postState = *postAnnotation->now;
            auto* postMemory = dynamic_cast<const SharedMemoryCore*>(plankton::TryGetResource(info->address, postState));
            if (!postMemory) continue;
            foundPost = true;
            auto newFuture = plankton::Copy(*future);
            newFuture->post = plankton::Copy(*postMemory);
            annotation.Conjoin(std::move(newFuture));
        }
        if (foundPost) plankton::MoveInto(std::move(post->effects), result.effects);
    }

    return result;
}