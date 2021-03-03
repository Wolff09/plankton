#include "heal/logic.hpp"

#include <cassert>

using namespace cola;
using namespace heal;


ImplicationFormula::ImplicationFormula() : premise(std::make_unique<AxiomConjunctionFormula>()), conclusion(std::make_unique<AxiomConjunctionFormula>()) {
}

NegatedAxiom::NegatedAxiom(std::unique_ptr<Axiom> axiom_) : axiom(std::move(axiom_)) {
	assert(axiom);
}

ExpressionAxiom::ExpressionAxiom(std::unique_ptr<cola::Expression> expr_) : expr(std::move(expr_)) {
	assert(expr);
	assert(expr->type() == Type::bool_type());
}

OwnershipAxiom::OwnershipAxiom(std::unique_ptr<VariableExpression> expr_) : expr(std::move(expr_)) {
	assert(expr);
	// assert(!expr->decl.is_shared); // no, we want to have negated ownership of shared, for instance
	assert(expr->decl.type.sort == Sort::PTR);
}

DataStructureLogicallyContainsAxiom::DataStructureLogicallyContainsAxiom(std::unique_ptr<cola::Expression> value_) : value(std::move(value_)) {
	assert(value);
	assert(value->sort() == Sort::DATA);	
}

NodeLogicallyContainsAxiom::NodeLogicallyContainsAxiom(std::unique_ptr<cola::Expression> node_, std::unique_ptr<cola::Expression> value_)
 : node(std::move(node_)), value(std::move(value_)) {
	assert(node);
	assert(node->sort() == Sort::PTR);
	assert(value);
	assert(value->sort() == Sort::DATA);
}

KeysetContainsAxiom::KeysetContainsAxiom(std::unique_ptr<Expression> node_, std::unique_ptr<Expression> value_)
 : node(std::move(node_)), value(std::move(value_)) {
	assert(node);
	assert(node->sort() == Sort::PTR);
	assert(value);
	assert(value->sort() == Sort::DATA);
}

HasFlowAxiom::HasFlowAxiom(std::unique_ptr<Expression> expr_) : expr(std::move(expr_)) {
	assert(expr);
	assert(expr->sort() == Sort::PTR);
}

FlowContainsAxiom::FlowContainsAxiom(std::unique_ptr<cola::Expression> node_, std::unique_ptr<cola::Expression> low_,std::unique_ptr<cola::Expression> high_)
	: node(std::move(node_)), value_low(std::move(low_)), value_high(std::move(high_))
{
	assert(node);
	assert(node->sort() == Sort::PTR);
	assert(value_low);
	assert(value_low->sort() == Sort::DATA);
	assert(value_high);
	assert(value_high->sort() == Sort::DATA);
}

UniqueInflowAxiom::UniqueInflowAxiom(std::unique_ptr<cola::Expression> node_) : node(std::move(node_)) {
	assert(node);
	assert(node->sort() == Sort::PTR);
}

SpecificationAxiom::SpecificationAxiom(Kind kind_, std::unique_ptr<cola::VariableExpression> key_) : kind(kind_), key(std::move(key_)) {
	assert(key);
	assert(key->sort() == Sort::DATA);
}

ObligationAxiom::ObligationAxiom(Kind kind_, std::unique_ptr<VariableExpression> key_) : SpecificationAxiom(kind_, std::move(key_)) {
}

FulfillmentAxiom::FulfillmentAxiom(Kind kind_, std::unique_ptr<VariableExpression> key_, bool return_value_)
 : SpecificationAxiom(kind_, std::move(key_)), return_value(return_value_) {
}

PastPredicate::PastPredicate(std::unique_ptr<ConjunctionFormula> formula_) : formula(std::move(formula_)) {
	assert(formula);
}

FuturePredicate::FuturePredicate(std::unique_ptr<ConjunctionFormula> pre_, std::unique_ptr<Assignment> cmd_, std::unique_ptr<ConjunctionFormula> post_)
 : pre(std::move(pre_)), command(std::move(cmd_)), post(std::move(post_)) {
	assert(pre);
	assert(command);
	assert(post);
}

Annotation::Annotation() : now(std::make_unique<ConjunctionFormula>()) {
}

Annotation::Annotation(std::unique_ptr<ConjunctionFormula> now_) : now(std::move(now_)) {
	assert(now);
}

Annotation::Annotation(std::unique_ptr<ConjunctionFormula> now_, std::deque<std::unique_ptr<TimePredicate>> time_) : now(std::move(now_)), time(std::move(time_)) {
	assert(now);
}

inline std::unique_ptr<Annotation> mk_bool(bool value) {
	auto result = std::make_unique<Annotation>();
	result->now->conjuncts.push_back(std::make_unique<ExpressionAxiom>(std::make_unique<BooleanValue>(value)));
	return result;
}

std::unique_ptr<Annotation> Annotation::make_true() {
	return mk_bool(true);
}

std::unique_ptr<Annotation> Annotation::make_false() {
	return mk_bool(false);
}