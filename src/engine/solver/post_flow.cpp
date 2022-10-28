#include "engine/solver.hpp"

#include "logics/util.hpp"
#include "engine/util.hpp"
#include "util/timer.hpp"

using namespace plankton;


inline std::unique_ptr<StackAxiom> ConvertToLogic(const BinaryExpression& condition, const Formula& context) {
    return std::make_unique<StackAxiom>(condition.op, plankton::MakeSymbolic(*condition.lhs, context),
                                        plankton::MakeSymbolic(*condition.rhs, context));
}

PostImage Solver::Post(std::unique_ptr<Annotation> pre, const AssertFlow& cmd) const {
    MEASURE("Solver::Post (AssertFlow)")
    DEBUG("<<POST ASSERT>>" << std::endl << *pre << " " << cmd << std::endl)
    PrepareAccess(*pre, cmd);
    plankton::InlineAndSimplify(*pre);

    auto val = plankton::MakeSymbolic(*cmd.value, *pre->now);
    auto& obj = plankton::GetResource(cmd.object->Decl(), *pre->now);
    auto& res = plankton::GetResource(obj.Value(), *pre->now);

    SymbolFactory factory(*pre);
    auto& symbol = factory.GetFreshFO(cmd.value->GetType());
    pre->now->Conjoin(std::make_unique<StackAxiom>(BinaryOperator::EQ, std::move(val), std::make_unique<SymbolicVariable>(symbol)));

    InflowContainsValueAxiom chk(res.flow->Decl(), symbol);
    Encoding encoding(*pre->now, config);
    if (!encoding.Implies(chk)) {
        throw std::logic_error("Flow-assertion '" + plankton::ToString(cmd) + "' potentially fails."); // TODO: that's a hack
    }
    return PostImage(std::move(pre));
}

PostImage Solver::Post(std::unique_ptr<Annotation> pre, const AssumeFlow& cmd) const {
    MEASURE("Solver::Post (AssumeFlow)")
    DEBUG("<<POST ASSUME>>" << std::endl << *pre << " " << cmd << std::endl)
    PrepareAccess(*pre, cmd);
    plankton::InlineAndSimplify(*pre);

    auto val = plankton::MakeSymbolic(*cmd.value, *pre->now);
    auto& obj = plankton::GetResource(cmd.object->Decl(), *pre->now);
    auto& res = plankton::GetResource(obj.Value(), *pre->now);

    SymbolFactory factory(*pre);
    auto& symbol = factory.GetFreshFO(cmd.value->GetType());
    pre->now->Conjoin(std::make_unique<StackAxiom>(BinaryOperator::EQ, std::move(val), std::make_unique<SymbolicVariable>(symbol)));
    pre->now->Conjoin(std::make_unique<InflowContainsValueAxiom>(res.flow->Decl(), symbol));
    if (IsUnsatisfiable(*pre)) {
        DEBUG("{ false }" << std::endl << std::endl)
        return PostImage();
    }
    plankton::ExtendStack(*pre, config, ExtensionPolicy::FAST);
    plankton::InlineAndSimplify(*pre);
    DEBUG(*pre << std::endl << std::endl)
    return PostImage(std::move(pre));
}
