#include "engine/solver.hpp"

#include "logics/util.hpp"
#include "engine/util.hpp"
#include "util/log.hpp"
#include "util/timer.hpp"

using namespace plankton;


std::set<const VariableDeclaration*> CollectVariables(const FuturePredicate& future) {
    struct : public ProgramListener {
        std::set<const VariableDeclaration*> result;
        void Enter(const VariableDeclaration& object) override { result.insert(&object); }
    } collector;
    for (const auto& elem : future.guard->conjuncts) elem->Accept(collector);
    for (const auto& elem : future.update->fields) elem->Accept(collector);
    return std::move(collector.result);
}

PostImage Solver::Post(std::unique_ptr<Annotation> pre, const VariableAssignment& cmd) const {
    MEASURE("Solver::Post (VariableAssignment)")
    DEBUG("<<POST VAR>>" << std::endl << *pre << " " << cmd << std::flush)

    assert(cmd.lhs.size() == cmd.rhs.size());
    PrepareAccess(*pre, cmd); // TODO: ensure no shared variable is updated
    plankton::InlineAndSimplify(*pre);

    // handle futures
    if (!pre->future.empty()) {
        std::set<const VariableDeclaration*> updatedVariables;
        for (const auto& var : cmd.lhs) updatedVariables.insert(&var->Decl());
        plankton::RemoveIf(pre->future, [&updatedVariables](const auto& future){
            return plankton::NonEmptyIntersection(updatedVariables, CollectVariables(*future));
        });
    }
    
    // evaluate rhs
    SymbolFactory factory(*pre);
    std::deque<std::unique_ptr<SymbolicVariable>> symbols;
    for (const auto& expr : cmd.rhs) {
        auto value = plankton::MakeSymbolic(*expr, *pre->now);
        auto& symbol = factory.GetFreshFO(expr->GetType());
        symbols.push_back(std::make_unique<SymbolicVariable>(symbol));
        pre->Conjoin(std::make_unique<StackAxiom>(
                BinaryOperator::EQ, std::make_unique<SymbolicVariable>(symbol), std::move(value)
        ));
    }
    
    // update lhs
    for (std::size_t index = 0; index < cmd.lhs.size(); ++index) {
        auto& decl = cmd.lhs.at(index)->Decl();
        assert(!decl.isShared);
        auto& resource = plankton::GetResource(decl, *pre->now);
        resource.value = std::move(symbols.at(index));
    }
    
    plankton::InlineAndSimplify(*pre);
    plankton::ExtendStack(*pre, config, ExtensionPolicy::FAST);
    DEBUG(*pre << std::endl << std::endl)
    // TODO: extend stack? expand memory? find fulfillments?
    return PostImage(std::move(pre));
}
