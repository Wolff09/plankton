#include "heal/util.hpp"

using namespace cola;
using namespace heal;

struct ExpressionContainsVisitor : public BaseVisitor {
	const Expression& search;
	bool found = false;
	ExpressionContainsVisitor(const Expression& search_) : search(search_) {}

	void handle_expression(const Expression& expression) {
		if (found) return;
		if (syntactically_equal(expression, search)) {
			found = true;
		} else {
			expression.accept(*this);
		}
	}

	void visit(const NegatedExpression& node) override {
		handle_expression(*node.expr);
	}

	void visit(const BinaryExpression& node) override {
		handle_expression(*node.lhs);
		handle_expression(*node.rhs);
	}

	void visit(const Dereference& node) override {
		handle_expression(*node.expr);
	}

	void visit(const BooleanValue& /*node*/) override { /* do nothing */ }
	void visit(const NullValue& /*node*/) override { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) override { /* do nothing */ }
	void visit(const MaxValue& /*node*/) override { /* do nothing */ }
	void visit(const MinValue& /*node*/) override { /* do nothing */ }
	void visit(const NDetValue& /*node*/) override { /* do nothing */ }
	void visit(const VariableExpression& /*node*/) override { /* do nothing */ }

	bool contains(const Expression& search) {
		found = false;
		handle_expression(search);
		return found;
	}
};


struct ContainsChecker : public DefaultListener {
	ExpressionContainsVisitor visitor;
	bool deep_search;
	ContainsChecker(const Expression& search, bool deep) : visitor(search), deep_search(deep) {}

	bool found = false;
	bool inside_obligation = false;
	bool found_inside_obligation = false;

	template<typename T>
	void handle_expression(const std::unique_ptr<T>& expression) {
		if (found && !deep_search) return;
		if (found_inside_obligation) return;
		if (found && !inside_obligation) return;

		bool contained = visitor.contains(*expression);
		found |= contained;
		found_inside_obligation |= (contained && inside_obligation);
	}

	void enter(const ExpressionAxiom& formula) override {
		handle_expression(formula.expr);
	}

	void enter(const OwnershipAxiom& formula) override {
		handle_expression(formula.expr);
	}

	void enter(const DataStructureLogicallyContainsAxiom& formula) override {
		handle_expression(formula.value);
	}

	void enter(const NodeLogicallyContainsAxiom& formula) override {
		handle_expression(formula.node);
		handle_expression(formula.value);
	}

	void enter(const KeysetContainsAxiom& formula) override {
		handle_expression(formula.node);
		handle_expression(formula.value);
	}

	void enter(const HasFlowAxiom& formula) override {
		handle_expression(formula.expr);
	}

	void enter(const FlowContainsAxiom& formula) override {
		handle_expression(formula.node);
		handle_expression(formula.value_low);
		handle_expression(formula.value_high);
	}

	void enter(const UniqueInflowAxiom& formula) override {
		handle_expression(formula.node);
	}

	void enter(const ObligationAxiom& formula) override {
		bool was_inside_obligation = inside_obligation;
		inside_obligation = true;
		handle_expression(formula.key);
		inside_obligation = was_inside_obligation;
	}

	void enter(const FulfillmentAxiom& formula) override {
		handle_expression(formula.key);
	}

	void enter(const FuturePredicate& formula) override {
		handle_expression(formula.command->lhs);
		handle_expression(formula.command->rhs);
	}

	static std::pair<bool, bool> contains(const Formula& formula, const Expression& search, bool deep=true) {
		ContainsChecker listener(search, deep);
		formula.accept(listener);
		return std::make_pair(listener.found, listener.found_inside_obligation);
	}
};


bool heal::contains_expression(const Formula& formula, const Expression& search) {
	return ContainsChecker::contains(formula, search, false).first;
}

bool heal::contains_expression(const Expression& expression, const Expression& search) {
	return ExpressionContainsVisitor(search).contains(expression);
}

std::pair<bool, bool> heal::contains_expression_obligation(const Formula& formula, const Expression& search) {
	return ContainsChecker::contains(formula, search);
}

template<typename T>
bool chk_contains_conjunct(const T& formula, const SimpleFormula& other) {
	for (const auto& conjunct : formula.conjuncts) {
		if (heal::syntactically_equal(*conjunct, other)) {
			return true;
		}
	}
	return false;	
}

bool heal::syntactically_contains_conjunct(const ConjunctionFormula& formula, const SimpleFormula& other) {
	return chk_contains_conjunct(formula, other);
}


struct ContainsConjunctChecker : public BaseLogicVisitor {
	const SimpleFormula& search;
	bool result;

	ContainsConjunctChecker(const SimpleFormula& search) : search(search) {}

	bool IsContainedIn(const Formula& formula) {
		result = false;
		formula.accept(*this);
		return result;
	}

	void visit(const AxiomConjunctionFormula& formula) override { result = chk_contains_conjunct(formula, search); }
	void visit(const ConjunctionFormula& formula) override { result = chk_contains_conjunct(formula, search); }

	void visit(const ImplicationFormula& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const NegatedAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const ExpressionAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const OwnershipAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const DataStructureLogicallyContainsAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const NodeLogicallyContainsAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const KeysetContainsAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const HasFlowAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const FlowContainsAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const UniqueInflowAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const ObligationAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	void visit(const FulfillmentAxiom& formula) override { result = heal::syntactically_equal(formula, search); }
	
	void visit(const PastPredicate& /*formula*/) override { result = false; }
	void visit(const FuturePredicate& /*formula*/) override { result = false; }
	void visit(const Annotation& formula) override { result = chk_contains_conjunct(*formula.now, search); }
};

bool heal::syntactically_contains_conjunct(const Formula& formula, const SimpleFormula& other) {
	return ContainsConjunctChecker(other).IsContainedIn(formula);
}