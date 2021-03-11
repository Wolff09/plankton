#include "heal/util.hpp"

using namespace cola;
using namespace heal;

struct ExpressionContainsVisitor : public BaseVisitor {
	const Expression& search;
	bool found = false;
	ExpressionContainsVisitor(const Expression& search_) : search(search_) {}

	void handle_expression(const Expression& expression) {
		if (found) return;
		if (SyntacticallyEqual(expression, search)) {
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

	static std::pair<bool, bool> contains(const LogicObject& object, const Expression& search, bool deep=true) {
		ContainsChecker listener(search, deep);
		object.accept(listener);
		return std::make_pair(listener.found, listener.found_inside_obligation);
	}
};

bool heal::ContainsExpression(const Expression& expression, const Expression& search) {
    return ExpressionContainsVisitor(search).contains(expression);
}

bool heal::ContainsExpression(const LogicObject& object, const Expression& search, bool* insideObligation) {
    auto [result, withinObligation] = ContainsChecker::contains(object, search);
    if (insideObligation) *insideObligation = withinObligation;
	return result;
}