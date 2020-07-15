#include "plankton/solver/solverimpl.hpp"

#include "plankton/error.hpp"
#include "plankton/util.hpp"
#include "plankton/solver/post.hpp"

using namespace cola;
using namespace plankton;
using parallel_assignment_t = Solver::parallel_assignment_t;


inline std::unique_ptr<Annotation> MakePost(PostInfo info, const Expression& lhs, const Expression& rhs) {
	auto [isLhsVar, lhsVar] = plankton::is_of_type<VariableExpression>(lhs);
	if (isLhsVar) return MakeVarAssignPost(std::move(info), { std::make_pair(std::cref(*lhsVar), std::cref(rhs)) });

	auto [isLhsDeref, lhsDeref] = plankton::is_of_type<Dereference>(lhs);
	if (!isLhsDeref) ThrowUnsupportedAssignmentError(lhs, rhs, "expected left-hand side to be a variable or a dereference");

	auto [isDerefVar, derefVar] = plankton::is_of_type<VariableExpression>(*lhsDeref->expr);
	if (!isDerefVar) ThrowUnsupportedAssignmentError(lhs, rhs, "expected left-hand side to be a dereference of a variable");

	// ensure rhs is a 'SimpleExpression' or the negation of such
	auto [isRhsNegation, rhsNegation] = plankton::is_of_type<NegatedExpression>(rhs);
	auto [isSimple, rhsSimple] = plankton::is_of_type<SimpleExpression>(isRhsNegation ? *rhsNegation->expr : rhs);
	if (!isSimple) ThrowUnsupportedAssignmentError(lhs, rhs, "expected right-hand side to be a variable or an immediate value");

	return MakeDerefAssignPost(std::move(info), *lhsDeref, rhs);
}

std::unique_ptr<Annotation> SolverImpl::Post(const Annotation& pre, parallel_assignment_t assignments) const {
	// ensure that no variable is assigned multiple times
	for (std::size_t index = 0; index < assignments.size(); ++index) {
		for (std::size_t other = index+1; other < assignments.size(); ++other) {
			if (plankton::syntactically_equal(assignments[index].first, assignments[index].second)) {
				throw UnsupportedConstructError("Malformed parallel assignment: variable " + assignments[index].first.get().decl.name + " is assigned multiple times.");
			}
		}
	}

	// compute post
	return plankton::PostProcess(MakeVarAssignPost(PostInfo(*this, pre), assignments), pre);
}

std::unique_ptr<Annotation> SolverImpl::Post(const Annotation& pre, const Assignment& cmd) const {
	return plankton::PostProcess(MakePost(PostInfo(*this, pre), *cmd.lhs, *cmd.rhs), pre);
}

bool SolverImpl::PostEntails(const ConjunctionFormula& pre, const Assignment& cmd, const ConjunctionFormula& post) const {
	auto partialPostImage = MakePost(PostInfo(*this, pre, post), *cmd.lhs, *cmd.rhs);
	return partialPostImage->now->conjuncts.size() == post.conjuncts.size();
}