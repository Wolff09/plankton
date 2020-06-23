#include "plankton/util.hpp"

using namespace cola;
using namespace plankton;


struct ExpressionContainsVisitor : public BaseVisitor {
	const Expression& search;
	bool found = false;
	ExpressionContainsVisitor(const Expression& search_) : search(search_) {}

	bool contains(const Expression& search) {
		bool found = false;
		search.accept(*this);
		return found;
	}

	template<typename T>
	void handle_expression(const std::unique_ptr<T>& expression) {
		if (found) return;
		if (syntactically_equal(*expression, search)) {
			found = true;
		} else {
			expression->accept(*this);
		}
	}

	void visit(const NegatedExpression& node) override {
		handle_expression(node.expr);
	}

	void visit(const BinaryExpression& node) override {
		handle_expression(node.lhs);
		handle_expression(node.rhs);
	}

	void visit(const Dereference& node) override {
		handle_expression(node.expr);
	}

	void visit(const BooleanValue& /*node*/) override { /* do nothing */ }
	void visit(const NullValue& /*node*/) override { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) override { /* do nothing */ }
	void visit(const MaxValue& /*node*/) override { /* do nothing */ }
	void visit(const MinValue& /*node*/) override { /* do nothing */ }
	void visit(const NDetValue& /*node*/) override { /* do nothing */ }
	void visit(const VariableExpression& /*node*/) override { /* do nothing */ }
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

	void enter(const LogicallyContainedAxiom& formula) override {
		handle_expression(formula.expr);
	}

	void enter(const KeysetContainsAxiom& formula) override {
		handle_expression(formula.node);
		handle_expression(formula.value);
	}

	void enter(const FlowAxiom& formula) override {
		handle_expression(formula.expr);
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


bool plankton::contains_expression(const Formula& formula, const Expression& search) {
	return ContainsChecker::contains(formula, search, false).first;
}

std::pair<bool, bool> plankton::contains_expression_obligation(const Formula& formula, const Expression& search) {
	return ContainsChecker::contains(formula, search);
}