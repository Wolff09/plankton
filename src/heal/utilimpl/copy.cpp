#include "heal/util.hpp"

#include "cola/util.hpp"

using namespace cola;
using namespace heal;


std::unique_ptr<AxiomConjunctionFormula> heal::copy(const AxiomConjunctionFormula& formula) {
	auto copy = std::make_unique<AxiomConjunctionFormula>();
	for (const auto& conjunct : formula.conjuncts) {
		copy->conjuncts.push_back(heal::copy(*conjunct));
	}
	return copy;
}

std::unique_ptr<ImplicationFormula> heal::copy(const ImplicationFormula& formula) {
	return MakeImplication(heal::copy(*formula.premise), heal::copy(*formula.conclusion));
}

std::unique_ptr<ConjunctionFormula> heal::copy(const ConjunctionFormula& formula) {
	auto copy = std::make_unique<ConjunctionFormula>();
	for (const auto& conjunct : formula.conjuncts) {
		copy->conjuncts.push_back(heal::copy(*conjunct));
	}
	return copy;
}

std::unique_ptr<NegatedAxiom> heal::copy(const NegatedAxiom& formula) {
	return std::make_unique<NegatedAxiom>(heal::copy(*formula.axiom));
}

std::unique_ptr<ExpressionAxiom> heal::copy(const ExpressionAxiom& formula) {
	return std::make_unique<ExpressionAxiom>(cola::copy(*formula.expr));
}

std::unique_ptr<OwnershipAxiom> heal::copy(const OwnershipAxiom& formula) {
	return std::make_unique<OwnershipAxiom>(std::make_unique<VariableExpression>(formula.expr->decl));
}

std::unique_ptr<NodeLogicallyContainsAxiom> heal::copy(const NodeLogicallyContainsAxiom& formula) {
	return std::make_unique<NodeLogicallyContainsAxiom>(cola::copy(*formula.node), cola::copy(*formula.value));
}

std::unique_ptr<DataStructureLogicallyContainsAxiom> heal::copy(const DataStructureLogicallyContainsAxiom& formula) {
	return std::make_unique<DataStructureLogicallyContainsAxiom>(cola::copy(*formula.value));
}

std::unique_ptr<KeysetContainsAxiom> heal::copy(const KeysetContainsAxiom& formula) {
	return std::make_unique<KeysetContainsAxiom>(cola::copy(*formula.node), cola::copy(*formula.value));
}

std::unique_ptr<HasFlowAxiom> heal::copy(const HasFlowAxiom& formula) {
	return std::make_unique<HasFlowAxiom>(cola::copy(*formula.expr));
}

std::unique_ptr<FlowContainsAxiom> heal::copy(const FlowContainsAxiom& formula) {
	return std::make_unique<FlowContainsAxiom>(cola::copy(*formula.node), cola::copy(*formula.value_low), cola::copy(*formula.value_high));
}

std::unique_ptr<UniqueInflowAxiom> heal::copy(const UniqueInflowAxiom& formula) {
	return std::make_unique<UniqueInflowAxiom>(cola::copy(*formula.node));
}

std::unique_ptr<ObligationAxiom> heal::copy(const ObligationAxiom& formula) {
	return std::make_unique<ObligationAxiom>(formula.kind, std::make_unique<VariableExpression>(formula.key->decl));
}

std::unique_ptr<FulfillmentAxiom> heal::copy(const FulfillmentAxiom& formula) {
	return std::make_unique<FulfillmentAxiom>(formula.kind, std::make_unique<VariableExpression>(formula.key->decl), formula.return_value);
}

std::unique_ptr<PastPredicate> heal::copy(const PastPredicate& formula) {
	return std::make_unique<PastPredicate>(heal::copy(*formula.formula));
}

std::unique_ptr<FuturePredicate> heal::copy(const FuturePredicate& formula) {
	return std::make_unique<FuturePredicate>(
		heal::copy(*formula.pre),
		std::make_unique<Assignment>(cola::copy(*formula.command->lhs), cola::copy(*formula.command->rhs)),
		heal::copy(*formula.post)
	);
}

std::unique_ptr<Annotation> heal::copy(const Annotation& formula) {
	auto copy = std::make_unique<Annotation>(heal::copy(*formula.now));
	for (const auto& pred : formula.time) {
		copy->time.push_back(heal::copy(*pred));
	}
	return copy;
}


template<typename T>
struct CopyVisitor : public LogicVisitor {
	std::unique_ptr<T> result;

	template<typename U>
	std::enable_if_t<std::is_base_of_v<T, U>> set_result(std::unique_ptr<U> uptr) {
		result = std::move(uptr);
	}

	template<typename U>
	std::enable_if_t<!std::is_base_of_v<T, U>> set_result(std::unique_ptr<U> /*uptr*/) {
		throw std::logic_error("Wargelgarbel: could not copy formula... sorry.");
	}

	void visit(const ConjunctionFormula& formula) { set_result(heal::copy(formula)); }
	void visit(const Annotation& formula) { set_result(heal::copy(formula)); }
	void visit(const AxiomConjunctionFormula& formula) { set_result(heal::copy(formula)); }

	void visit(const ImplicationFormula& formula) { set_result(heal::copy(formula)); }

	void visit(const NegatedAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const ExpressionAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const OwnershipAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const DataStructureLogicallyContainsAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const NodeLogicallyContainsAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const KeysetContainsAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const HasFlowAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const FlowContainsAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const UniqueInflowAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const ObligationAxiom& formula) { set_result(heal::copy(formula)); }
	void visit(const FulfillmentAxiom& formula) { set_result(heal::copy(formula)); }

	void visit(const PastPredicate& formula) { set_result(heal::copy(formula)); }
	void visit(const FuturePredicate& formula) { set_result(heal::copy(formula)); }
};

template<typename T>
std::unique_ptr<T> do_copy(const T& formula) {
	CopyVisitor<T> visitor;
	formula.accept(visitor);
	return std::move(visitor.result);
}

std::unique_ptr<Formula> heal::copy(const Formula& formula) {
	return do_copy<Formula>(formula);
}

std::unique_ptr<NowFormula> heal::copy(const NowFormula& formula) {
	return do_copy<NowFormula>(formula);
}

std::unique_ptr<SimpleFormula> heal::copy(const SimpleFormula& formula) {
	return do_copy<SimpleFormula>(formula);
}

std::unique_ptr<Axiom> heal::copy(const Axiom& formula) {
	return do_copy<Axiom>(formula);
}

std::unique_ptr<TimePredicate> heal::copy(const TimePredicate& formula) {
	return do_copy<TimePredicate>(formula);
}
