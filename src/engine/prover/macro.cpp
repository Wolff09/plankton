#include "engine/prover.hpp"

#include "programs/util.hpp"
#include "logics/util.hpp"
#include "engine/util.hpp"
#include "util/shortcuts.hpp"
#include "util/log.hpp"

using namespace plankton;
using ReturnValues = Prover::ReturnValues;


// inline void CleanAnnotation(Annotation& annotation) {
//     plankton::Simplify(annotation);
//
//     // find symbols that do not occur in variable assignments or memory
//     struct : public LogicListener {
//         std::set<const SymbolDeclaration*> prune;
//         void Enter(const SymbolDeclaration& object) override { prune.erase(&object); }
//         void Visit(const StackAxiom&) override { /* do nothing */ }
//         void Visit(const InflowEmptinessAxiom&) override { /* do nothing */ }
//         void Visit(const InflowContainsValueAxiom&) override { /* do nothing */ }
//         void Visit(const InflowContainsRangeAxiom&) override { /* do nothing */ }
//     } collector;
//     collector.prune = plankton::Collect<SymbolDeclaration>(annotation);
//     annotation.Accept(collector);
//     auto prune = std::move(collector.prune);
//
//     // find unreachable memory addresses
//     auto reach = plankton::ComputeReachability(*annotation.now);
//     std::set<const SymbolDeclaration*> reachable;
//     for (const auto* var : plankton::Collect<EqualsToAxiom>(*annotation.now)) {
//         reachable.insert(&var->Value());
//         plankton::InsertInto(reach.GetReachable(var->Value()), reachable);
//     }
//     auto pruneIfUnreachable = [&reachable,&prune](const auto* mem) {
//         if (plankton::Membership(reachable, &mem->node->Decl())) return;
//         prune.insert(&mem->node->Decl());
//     };
//     for (const auto* mem : plankton::Collect<SharedMemoryCore>(*annotation.now)) pruneIfUnreachable(mem);
//     for (const auto& past : annotation.past) pruneIfUnreachable(past->formula.get());
//
//     // remove parts that are not interesting
//     auto containsPruned = [&prune](const auto& conjunct) {
//         if (dynamic_cast<const LocalMemoryResource*>(conjunct.get())) return false;
//         auto symbols = plankton::Collect<SymbolDeclaration>(*conjunct);
//         return plankton::NonEmptyIntersection(symbols, prune);
//     };
//     plankton::RemoveIf(annotation.now->conjuncts, containsPruned);
//     plankton::RemoveIf(annotation.past, containsPruned);
// }
//
// inline std::optional<ProofGenerator::AnnotationList>
// ProofGenerator::LookupMacroPost(const Macro& macro, const Annotation& annotation) {
//     auto find = macroPostTable.find(&macro.Func());
//     if (find != macroPostTable.end()) {
//         for (const auto&[pre, post]: find->second) {
//             if (!solver.Implies(annotation, *pre)) continue;
//             return plankton::CopyAll(post);
//         }
//     }
//     return std::nullopt;
// }
//
// inline void
// ProofGenerator::AddMacroPost(const Macro& macro, const Annotation& pre, const ProofGenerator::AnnotationList& post) {
//     // DEBUG("%% storing macro post: " << pre << " >>>>> ")
//     // for (const auto& elem : post) DEBUG(*elem)
//     // DEBUG(std::endl)
//     macroPostTable[&macro.Func()].emplace_back(plankton::Copy(pre), plankton::CopyAll(post));
// }
//
// void ProofGenerator::HandleMacroLazy(const Macro& cmd) {
//     HandleMacroProlog(cmd);
//     decltype(current) post;
//
//     // lookup tabulated posts
//     plankton::RemoveIf(current, [this, &cmd, &post](const auto& elem) {
//         auto lookup = LookupMacroPost(cmd, *elem);
//         if (!lookup) return false;
//         DEBUG("    ~~> found tabulated post" << std::endl)
//         plankton::MoveInto(std::move(lookup.value()), post);
//         return true;
//     });
//
//     // compute remaining posts
//     DEBUG("    ~~> " << (current.empty() ? "not descending" : "descending...") << std::endl)
//     if (!current.empty()) {
//         for (auto& elem : current) CleanAnnotation(*elem);
//         auto pre = plankton::CopyAll(current);
//         cmd.Func().Accept(*this);
//         HandleMacroEpilog(cmd);
//         // for (auto& elem : current) CleanAnnotation(*elem);
//         for (const auto& elem : pre) AddMacroPost(cmd, *elem, current);
//     }
//
//     plankton::MoveInto(std::move(post), current);
// }


std::unique_ptr<StackAxiom> MakeEq(const SymbolDeclaration& lhs, std::unique_ptr<SymbolicExpression> rhs) {
    return std::make_unique<StackAxiom>(BinaryOperator::EQ, std::make_unique<SymbolicVariable>(lhs), std::move(rhs));
}

inline std::unique_ptr<Annotation> LeaveCallerScopes(std::unique_ptr<Annotation> annotation, const Macro& /*com*/, const Solver& /*solver*/) {
    /* TODO: handle caller/callee scopes properly
     *
     * PROBLEM:
     * removing variable bindings from the annotation removes any information about those variables
     * this information cannot be rebuilt after the macro call is finished
     * the reason is that symbolic variables might be renamed, but the new name for the old values of the variables is not known
     * one could add ghost resources that track the renaming of the necessary symbolic variables (but do not participate in further reasoning)
     */
    // for (const auto* scope = com.parent.get_or(nullptr); scope != nullptr; scope = scope.parent.get_or(nullptr)) {
    //     annotation = solver.PostLeave(std::move(annotation), *scope);
    // }
    // solver.PostLeave(std::move(annotation), com.Func());

    return annotation;
}

inline std::unique_ptr<Annotation> ReenterCallerScopes(std::unique_ptr<Annotation> annotation, const Macro& /*com*/, const Solver& /*solver*/) {
    // TODO: undo LeaveCallerScopes
    return annotation;
}


//
// Prolog
//

inline std::unique_ptr<Annotation> PerformProlog(std::unique_ptr<Annotation> annotation, const Macro& cmd, const Solver& solver) {
    // get actual parameters
    auto parameters = MakeVector<std::unique_ptr<SymbolicExpression>>(cmd.arguments.size());
    for (const auto& expr : cmd.arguments) parameters.push_back(plankton::MakeSymbolic(*expr, *annotation->now));

    // leave caller scope
    auto prolog = LeaveCallerScopes(std::move(annotation), cmd, solver);

    // enter function scope, populate parameters
    prolog = solver.PostEnter(std::move(prolog), cmd.Func());
    SymbolFactory factory(*prolog);
    for (std::size_t index = 0; index < cmd.arguments.size(); ++index) {
        auto& resource = plankton::GetResource(*cmd.Func().parameters.at(index), *prolog->now);
        auto& actual = parameters.at(index);
        auto& symbol = factory.GetFreshFO(actual->GetType());
        prolog->Conjoin(MakeEq(symbol, std::move(actual)));
        resource.value->decl = symbol;
    }

    prolog = solver.Widen(std::move(prolog));
    plankton::InlineAndSimplify(*prolog);
    return prolog;
}


//
// Epilog
//

inline std::unique_ptr<Annotation> PerformEpilog(std::unique_ptr<Annotation> annotation, ReturnValues returnValues, const Macro& cmd, const Solver& solver) {
    // reenter caller scope (callee scope is terminated by the inside the macro return)
    auto epilog = ReenterCallerScopes(std::move(annotation), cmd, solver);

    // check that non-void macro returned values
    if (cmd.lhs.size() != returnValues.size()) {
        throw std::logic_error("Unexpected number of return values: expected " + std::to_string(cmd.lhs.size()) + " got " + std::to_string(returnValues.size()) + "."); // TODO: better error handling
    }

    // update return values
    SymbolFactory factory(*epilog);
    for (std::size_t index = 0; index < returnValues.size(); ++index) {
        auto& resource = plankton::GetResource(cmd.lhs.at(index)->Decl(), *epilog->now);
        auto& symbol = factory.GetFreshFO(resource.value->GetType());
        epilog->Conjoin(MakeEq(symbol, std::move(returnValues.at(index))));
        resource.value->decl = symbol;
    }

    epilog = solver.Widen(std::move(epilog));
    plankton::InlineAndSimplify(*epilog);
    return epilog;
}


//
// Macro
//

void Prover::HandleMacro(const Macro& cmd) {
    Callback(&ProofListener::BeforeHandleMacro, std::cref(cmd));

    // save caller context
    auto outerBreaking = std::move(breaking);
    auto outerReturning = std::move(returning);

    // TODO: frame away local things (beware renaming issues; cf. 'LeaveCallerScopes')

    // prolog
    Transform([this, &cmd](auto annotation){
        return PerformProlog(std::move(annotation), cmd, *solver);
    });

    // execute macro
    HandleMacroFunction(cmd.Func());

    // epilog
    current.clear();
    for (auto&& [annotation, values] : returning) {
        current.push_back(PerformEpilog(std::move(annotation), std::move(values), cmd, *solver));
    }

    // restore caller context
    breaking = std::move(outerBreaking);
    returning = std::move(outerReturning);

    CallbackReverse(&ProofListener::AfterHandleMacro, std::cref(cmd));
}
