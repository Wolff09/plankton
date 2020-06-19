#include "plankton/post.hpp"

#include <iostream> // TODO: remove
#include <sstream>
#include <type_traits>
#include "cola/util.hpp"
#include "plankton/util.hpp"
#include "plankton/error.hpp"

using namespace cola;
using namespace plankton;


struct POST_PTR_T {};
struct POST_DATA_T {};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct PurityChecker {
	std::deque<std::unique_ptr<VariableDeclaration>> dummy_decls;
	std::deque<std::unique_ptr<Formula>> dummy_formulas;

	const Type dummy_data_type = Type("post#dummy-data-type", Sort::DATA);
	const VariableDeclaration dummy_search_key = VariableDeclaration("post#dummy-search-key", dummy_data_type, false);

	using tuple_t = std::pair<std::unique_ptr<ConjunctionFormula>, std::unique_ptr<ConjunctionFormula>>;
	using triple_t = std::tuple<const Type*, const Type*, const Type*>;
	using quintuple_t = std::tuple<const VariableDeclaration*, const VariableDeclaration*, const VariableDeclaration*, const VariableDeclaration*, const VariableDeclaration*>;
	std::map<triple_t, quintuple_t> types2decls;
	std::map<triple_t, const ConjunctionFormula*> types2premise;
	std::map<triple_t, std::pair<const ConjunctionFormula*, const ConjunctionFormula*>> types2pure;
	std::map<triple_t, std::pair<const ConjunctionFormula*, const ConjunctionFormula*>> types2inserted;
	std::map<triple_t, std::pair<const ConjunctionFormula*, const ConjunctionFormula*>> types2deleted;


	static std::unique_ptr<Axiom> from_expr(std::unique_ptr<Expression>&& expr) {
		return std::make_unique<ExpressionAxiom>(std::move(expr));
	}

	static std::unique_ptr<Expression> from_decl(const VariableDeclaration& decl) {
		return std::make_unique<VariableExpression>(decl);
	}

	static std::unique_ptr<Expression> from_decl(const VariableDeclaration& decl, std::string field) {
		return std::make_unique<Dereference>(from_decl(decl), field);
	}

	static triple_t get_triple(const Type& keyType, const Type& nodeType, std::string fieldname) {
		auto fieldType = nodeType.field(fieldname);
		assert(fieldType);
		return std::make_tuple(&keyType, &nodeType, fieldType);
	}

	static triple_t get_triple(const VariableDeclaration& searchKey, const Dereference& deref) {
		return get_triple(searchKey.type, deref.expr->type(), deref.fieldname);
	}

	static std::unique_ptr<ImplicationFormula> make_implication(std::unique_ptr<Axiom> premise, std::unique_ptr<Axiom> conclusion) {
		auto result = std::make_unique<ImplicationFormula>();
		result->premise->conjuncts.push_back(std::move(premise));
		result->conclusion->conjuncts.push_back(std::move(conclusion));
		return result;
	}

	quintuple_t get_decls(triple_t mapKey) {
		auto find = types2decls.find(mapKey);
		if (find != types2decls.end()) {
			return find->second;
		}

		auto [keyType, nodeType, valueType] = mapKey;
		auto keyNow = std::make_unique<VariableDeclaration>("tmp#searchKey-now", *keyType, false);
		auto keyNext = std::make_unique<VariableDeclaration>("tmp#searchKey-next", *keyType, false);
		auto nodeNow = std::make_unique<VariableDeclaration>("tmp#node-now", *nodeType, false);
		auto nodeNext = std::make_unique<VariableDeclaration>("tmp#node-next", *nodeType, false);
		auto value = std::make_unique<VariableDeclaration>("tmp#value", *valueType, false);

		auto mapValue = std::make_tuple(keyNow.get(), keyNext.get(), nodeNow.get(), nodeNext.get(), value.get());
		dummy_decls.push_back(std::move(keyNow));
		dummy_decls.push_back(std::move(keyNext));
		dummy_decls.push_back(std::move(nodeNow));
		dummy_decls.push_back(std::move(nodeNext));
		dummy_decls.push_back(std::move(value));
		types2decls[mapKey] = mapValue;

		return mapValue;
	}

	std::pair<quintuple_t, std::unique_ptr<ConjunctionFormula>> get_dummy_premise_base(triple_t mapKey, std::string field) {
		auto quint = get_decls(mapKey);

		// see if we have something prepared already
		auto find = types2premise.find(mapKey);
		if (find != types2premise.end()) {
			return std::make_pair(quint, plankton::copy(*find->second));
		}

		// prepare something new
		auto [keyNow, keyNext, nodeNow, nodeNext, value] = quint;
		auto premise = std::make_unique<ConjunctionFormula>();
		premise->conjuncts.push_back(from_expr( // nodeNow = nodeNext
			std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, from_decl(*nodeNow), from_decl(*nodeNext))
		));
		premise->conjuncts.push_back(from_expr( // nodeNext->{field} = value
			std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, from_decl(*nodeNext, field), from_decl(*value))
		));
		for (auto pair : nodeNow->type.fields) {
			auto name = pair.second.get().name;
			if (name == field) continue;
			premise->conjuncts.push_back(from_expr( // nodeNow->{name} = nodeNext->{name}
				std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, from_decl(*nodeNow, name), from_decl(*nodeNext, name))
			));	
		}

		// return
		auto result = std::make_pair(quint, plankton::copy(*premise));
		types2premise.insert(std::make_pair(mapKey, premise.get()));
		dummy_formulas.push_back(std::move(premise));
		return result;
	}

	std::unique_ptr<Axiom> get_key_formula(quintuple_t decls) {
		auto& keyNow = *std::get<0>(decls);
		auto& keyNext = *std::get<1>(decls);
		return from_expr(std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, from_decl(keyNow), from_decl(keyNext)));
	}

	std::pair<quintuple_t, std::unique_ptr<ConjunctionFormula>> get_dummy_premise(triple_t mapKey, std::string fieldname) {
		auto result = get_dummy_premise_base(mapKey, fieldname);
		result.second->conjuncts.push_back(get_key_formula(result.first));
		return result;
	}

	std::unique_ptr<Axiom> make_contains_predicate(const VariableDeclaration& /*node*/, const VariableDeclaration& /*key*/) {
		// return contains(node, key)
		throw std::logic_error("not yet implemented: make_contains_next");
	}

	std::unique_ptr<ConjunctionFormula> make_pure_conclusion(quintuple_t decls) {
		auto& keyNext = *std::get<1>(decls);
		auto& nodeNow = *std::get<2>(decls);
		auto& nodeNext = *std::get<3>(decls);

		auto make_equality = [](auto formula, auto other) {
			auto result = std::make_unique<ConjunctionFormula>();
			result->conjuncts.push_back(make_implication(plankton::copy(*formula), plankton::copy(*other)));
			result->conjuncts.push_back(make_implication(std::move(other), std::move(formula)));
			return result;
		};

		return make_equality(make_contains_predicate(nodeNow, keyNext), make_contains_predicate(nodeNext, keyNext));
	}

	std::unique_ptr<ConjunctionFormula> make_satisfaction_conclusion(quintuple_t decls, bool deletion) {
		auto& keyNow = *std::get<0>(decls);
		auto& nodeNow = *std::get<2>(decls);
		auto& nodeNext = *std::get<3>(decls);

		auto result = make_pure_conclusion(decls);

		auto lhs = make_contains_predicate(nodeNow, keyNow);
		auto rhs = make_contains_predicate(nodeNext, keyNow);
		if (!deletion) {
			lhs.swap(rhs);
		}
		result->conjuncts.push_back(std::move(lhs));
		result->conjuncts.push_back(std::make_unique<NegatedAxiom>(std::move(rhs)));

		return result;
	}

	template<typename M, typename F>
	tuple_t get_or_create_dummy(M& map, triple_t mapKey, F makeNew) {
		auto find = map.find(mapKey);
		if (find != map.end()) {
			auto [premise, conclusion] = find->second;
			return std::make_pair(plankton::copy(*premise), plankton::copy(*conclusion));

		} else {
			auto [premise, conclusion] = makeNew();
			auto result = std::make_pair(plankton::copy(*premise), plankton::copy(*conclusion));
			map[mapKey] = std::make_pair(premise.get(), conclusion.get());
			dummy_formulas.push_back(std::move(premise));
			dummy_formulas.push_back(std::move(conclusion));
			return result;
		}
	}

	tuple_t get_dummy_pure(triple_t mapKey, std::string fieldname) {
		return get_or_create_dummy(types2pure, mapKey, [&,this](){
			auto [decls, premise] = get_dummy_premise_base(mapKey, fieldname); // do not restrict keyNext
			auto conclusion = make_pure_conclusion(decls);
			return std::make_pair(std::move(premise), std::move(conclusion));
		});
	}

	tuple_t get_dummy_satisfies_insert(triple_t mapKey, std::string fieldname) {
		return get_or_create_dummy(types2inserted, mapKey, [&,this](){
			auto [decls, premise] = get_dummy_premise(mapKey, fieldname);
			auto conclusion = make_satisfaction_conclusion(decls, false);
			return std::make_pair(std::move(premise), std::move(conclusion));
		});
	}

	tuple_t get_dummy_satisfies_delete(triple_t mapKey, std::string fieldname) {
		return get_or_create_dummy(types2deleted, mapKey, [&,this](){
			auto [decls, premise] = get_dummy_premise(mapKey, fieldname);
			auto conclusion = make_satisfaction_conclusion(decls, true);
			return std::make_pair(std::move(premise), std::move(conclusion));
		});
	}


	std::unique_ptr<ConjunctionFormula> extend_dummy_with_state(std::unique_ptr<ConjunctionFormula> dummy, const ConjunctionFormula& state, triple_t mapKey, const VariableDeclaration& searchKey, const VariableDeclaration& node, const Expression& value) {
		auto premise = plankton::copy(state); // TODO: one could probably avoid this copy somehow
		for (auto& conjunct : dummy->conjuncts) {
			premise->conjuncts.push_back(std::move(conjunct));
		}

		auto [dummyKeyNow, dummyKeyNext, dummyNodeNow, dummyNodeNext, dummyValue] = get_decls(mapKey);
		premise->conjuncts.push_back(from_expr(
			std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, from_decl(*dummyNodeNow), from_decl(node))
		));
		premise->conjuncts.push_back(from_expr(
			std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, from_decl(*dummyKeyNow), from_decl(searchKey))
		));
		premise->conjuncts.push_back(from_expr(
			std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, from_decl(*dummyValue), cola::copy(value))
		));

		return premise;
	}

	tuple_t get_pure(const ConjunctionFormula& state, const Dereference& deref, const VariableDeclaration& derefNode, const Expression& value) {
		auto mapKey = get_triple(dummy_search_key, deref);
		auto [premise, conclusion] = get_dummy_pure(mapKey, deref.fieldname);
		premise = extend_dummy_with_state(std::move(premise), state, mapKey, dummy_search_key, derefNode, value);
		return std::make_pair(std::move(premise), std::move(conclusion));
	}

	tuple_t get_satisfies_insert(const ConjunctionFormula& state, const VariableDeclaration& searchKey, const Dereference& deref, const VariableDeclaration& derefNode, const Expression& value) {
		auto mapKey = get_triple(searchKey, deref);
		auto [premise, conclusion] = get_dummy_satisfies_insert(mapKey, deref.fieldname);
		premise = extend_dummy_with_state(std::move(premise), state, mapKey, searchKey, derefNode, value);
		return std::make_pair(std::move(premise), std::move(conclusion));
	}

	tuple_t get_satisfies_delete(const ConjunctionFormula& state, const VariableDeclaration& searchKey, const Dereference& deref, const VariableDeclaration& derefNode, const Expression& value) {
		auto mapKey = get_triple(searchKey, deref);
		auto [premise, conclusion] = get_dummy_satisfies_delete(mapKey, deref.fieldname);
		premise = extend_dummy_with_state(std::move(premise), state, mapKey, searchKey, derefNode, value);
		return std::make_pair(std::move(premise), std::move(conclusion));
	}


	static bool implication_holds(tuple_t&& implication) {
		return plankton::implies(*implication.first, *implication.second);
	}

	bool is_pure(const ConjunctionFormula& state, const Dereference& deref, const VariableDeclaration& derefNode, const Expression& value) {
		return implication_holds(get_pure(state, deref, derefNode, value));
	}

	bool satisfies_insert(const ConjunctionFormula& state, const VariableDeclaration& searchKey, const Dereference& deref, const VariableDeclaration& derefNode, const Expression& value) {
		return implication_holds(get_satisfies_insert(state, searchKey, deref, derefNode, value));
	}

	bool satisfies_delete(const ConjunctionFormula& state, const VariableDeclaration& searchKey, const Dereference& deref, const VariableDeclaration& derefNode, const Expression& value) {
		return implication_holds(get_satisfies_delete(state, searchKey, deref, derefNode, value));
	}

} thePurityChecker;


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct Pruner : public BaseLogicNonConstVisitor { // TODO: is this thingy correct?
	const Expression& prune_expr;
	bool prune_child = false;
	bool prune_value = true;
	Pruner(const Expression& expr) : prune_expr(expr) {}

	template<typename T> std::unique_ptr<T> make_pruned() {
		return std::make_unique<ExpressionAxiom>(std::make_unique<BooleanValue>(prune_value));
	}
	
	template<> std::unique_ptr<AxiomConjunctionFormula> make_pruned<AxiomConjunctionFormula>() {
		auto result = std::make_unique<AxiomConjunctionFormula>();
		result->conjuncts.push_back(std::make_unique<ExpressionAxiom>(std::make_unique<BooleanValue>(prune_value)));
		return result;
	}

	template<typename T>
	void handle_formula(std::unique_ptr<T>& formula) {
		formula->accept(*this);
		if (prune_child) {
			formula = make_pruned<T>();
			prune_child = false;
		}
	}

	void visit(ConjunctionFormula& formula) override {
		for (auto& conjunct : formula.conjuncts) {
			handle_formula(conjunct);
		}
	}

	void visit(AxiomConjunctionFormula& formula) override {
		for (auto& conjunct : formula.conjuncts) {
			handle_formula(conjunct);
		}
	}

	void visit(NegatedAxiom& formula) override {
		bool old_value = prune_value;
		prune_value = !prune_value;
		handle_formula(formula.axiom);
		prune_value = old_value;
	}

	void visit(ImplicationFormula& formula) override {
		assert(prune_value);
		handle_formula(formula.conclusion);
	}

	void visit(OwnershipAxiom& formula) override {
		if (!prune_value) return;
		prune_child = plankton::syntactically_equal(*formula.expr, prune_expr);
	}

	void visit(LogicallyContainedAxiom& /*formula*/) override { prune_child = true; }
	void visit(KeysetContainsAxiom& /*formula*/) override { prune_child = true; }
	void visit(FlowAxiom& /*formula*/) override { prune_child = true; }

	void visit(ExpressionAxiom& /*formula*/) override { prune_child = false; }
	void visit(ObligationAxiom& /*formula*/) override { prune_child = false; }
	void visit(FulfillmentAxiom& /*formula*/) override { prune_child = false; }
};

std::unique_ptr<ConjunctionFormula> destroy_ownership_and_non_local_knowledge(std::unique_ptr<ConjunctionFormula> formula, const Expression& expr) {
	// remove ownership of 'expr' in 'formula' as well as all non-local knowledge (like flow, keysets, node contents)
	Pruner visitor(expr);
	formula->accept(visitor);
	return formula;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
T container_search_and_destroy(T&& uptrcontainer, const Expression& destroy) {
	// TODO important: are we are deleting to much -> should we avoid deleting things in the premise of implications?

	// remove knowledge about 'destroy' in 'uptrcontainer'
	T new_container;
	for (auto& elem : uptrcontainer) {
		if (!plankton::contains_expression(*elem, destroy)) {
			new_container.push_back(std::move(elem));
		}
	}
	return std::move(new_container);
}

template<typename T>
T container_search_and_inline(T&& uptrcontainer, const Expression& search, const Expression& replacement) {
	// TODO important: must we avoid inlining in the premise of implications?

	// copy knowledge about 'search' in 'uptrcontainer' over to 'replacement'
	T new_elements;
	for (auto& elem : uptrcontainer) {
		auto [contained, contained_within_obligation] = plankton::contains_expression_obligation(*elem, search);
		if (contained && !contained_within_obligation) {
			new_elements.push_back(replace_expression(plankton::copy(*elem), search, replacement));
		}
	}
	uptrcontainer.insert(uptrcontainer.end(), std::make_move_iterator(new_elements.begin()), std::make_move_iterator(new_elements.end()));
	return std::move(uptrcontainer);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Annotation> search_and_destroy_and_inline_var(std::unique_ptr<Annotation> pre, const VariableExpression& lhs, const Expression& rhs) {
	// destroy knowledge about lhs
	auto now = destroy_ownership_and_non_local_knowledge(std::move(pre->now), rhs);
	now->conjuncts = container_search_and_destroy(std::move(now->conjuncts), lhs);
	pre->time = container_search_and_destroy(std::move(pre->time), lhs);

	// copy knowledge about rhs over to lhs
	now->conjuncts = container_search_and_inline(std::move(now->conjuncts), rhs, lhs);
	pre->time = container_search_and_inline(std::move(pre->time), rhs, lhs); // TODO important: are those really freely duplicable?

	// add new knowledge
	auto equality = std::make_unique<ExpressionAxiom>(std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, cola::copy(lhs), cola::copy(rhs)));
	now->conjuncts.push_back(std::move(equality));
	pre->now = std::move(now);

	// done
	return pre;
}

inline void check_purity(const VariableExpression& lhs) {
	if (lhs.decl.is_shared) {
		// assignment to 'lhs' potentially impure (may require obligation/fulfillment transformation)
		// TODO: implement
		throw UnsupportedConstructError("assignment to shared variable '" + lhs.decl.name + "'; assignment potentially impure.");
	}
}

std::unique_ptr<Annotation> post_full_assign_var_expr(std::unique_ptr<Annotation> pre, const Assignment& /*cmd*/, const VariableExpression& lhs, const Expression& rhs) {
	check_purity(lhs);
	return search_and_destroy_and_inline_var(std::move(pre), lhs, rhs);
}

std::unique_ptr<Annotation> post_full_assign_var_derefptr(std::unique_ptr<Annotation> pre, const Assignment& /*cmd*/, const VariableExpression& lhs, const Dereference& rhs, const VariableExpression& /*rhsVar*/) {
	check_purity(lhs);
	auto post = search_and_destroy_and_inline_var(std::move(pre), lhs, rhs);
	// TODO: infer flow/contains and put it into a past predicate
	// TODO: no self loops? Add lhs != rhs?
	// TODO: use invariant to find out stuff?
	throw std::logic_error("not yet implemented: post_full_assign_var_derefptr");
}

std::unique_ptr<Annotation> post_full_assign_var_derefdata(std::unique_ptr<Annotation> pre, const Assignment& /*cmd*/, const VariableExpression& lhs, const Dereference& rhs, const VariableExpression& /*rhsVar*/) {
	check_purity(lhs);
	// TODO: use invariant to find out stuff?
	return search_and_destroy_and_inline_var(std::move(pre), lhs, rhs);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct DereferenceExtractor : public BaseVisitor, public DefaultListener {
	using BaseVisitor::visit;
	using DefaultListener::visit;
	using DefaultListener::enter;

	std::string search_field;
	std::deque<std::unique_ptr<Expression>> result;
	DereferenceExtractor(std::string search) : search_field(search) {}

	static std::deque<std::unique_ptr<Expression>> extract(const Formula& formula, std::string fieldname) {
		DereferenceExtractor visitor(fieldname);
		formula.accept(visitor);
		return std::move(visitor.result);
	}

	void visit(const Dereference& node) override {
		node.expr->accept(*this);
		result.push_back(cola::copy(node));
	}
	void visit(const BooleanValue& /*node*/) override { /* do nothing */ }
	void visit(const NullValue& /*node*/) override { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) override { /* do nothing */ }
	void visit(const MaxValue& /*node*/) override { /* do nothing */ }
	void visit(const MinValue& /*node*/) override { /* do nothing */ }
	void visit(const NDetValue& /*node*/) override { /* do nothing */ }
	void visit(const VariableExpression& /*node*/) override { /* do nothing */ }
	void visit(const NegatedExpression& node) override { node.expr->accept(*this); }
	void visit(const BinaryExpression& node) override { node.lhs->accept(*this); node.rhs->accept(*this); }
	
	void enter(const ExpressionAxiom& formula) override { formula.expr->accept(*this); }
	void enter(const OwnershipAxiom& formula) override { formula.expr->accept(*this); }
	void enter(const LogicallyContainedAxiom& formula) override { formula.expr->accept(*this); }
	void enter(const KeysetContainsAxiom& formula) override { formula.node->accept(*this); formula.value->accept(*this); }
	void enter(const FlowAxiom& formula) override { formula.expr->accept(*this); }
	void enter(const ObligationAxiom& formula) override { formula.key->accept(*this); }
	void enter(const FulfillmentAxiom& formula) override { formula.key->accept(*this); }
};

std::unique_ptr<ConjunctionFormula> search_and_destroy_derefs(std::unique_ptr<ConjunctionFormula> now, const Dereference& lhs) {
	auto expr = std::make_unique<BinaryExpression>(
		BinaryExpression::Operator::NEQ, std::make_unique<Dereference>(std::make_unique<NullValue>(), lhs.fieldname), cola::copy(lhs)
	);
	auto& uptr = expr->lhs;
	auto inequality = std::make_unique<ExpressionAxiom>(std::move(expr));
	

	// find all dereferences that may be affected (are not guaranteed to be destroyed)
	auto dereferences = DereferenceExtractor::extract(*now, lhs.fieldname);
	for (auto& deref : dereferences) {
		uptr = std::move(deref);
		deref.reset();
		if (!plankton::implies(*now, *inequality)) {
			deref = std::move(uptr);
		}
	}

	// delete knowlege about potentially affected dereferences
	for (auto& deref : dereferences) {
		if (deref) {
			now->conjuncts = container_search_and_destroy(std::move(now->conjuncts), *deref);
		}
	}

	return now;
}

std::unique_ptr<Annotation> search_and_destroy_and_inline_deref(std::unique_ptr<Annotation> pre, const Dereference& lhs, const Expression& rhs) {
	// destroy knowledge about lhs (do not modify TimePredicates)
	auto now = destroy_ownership_and_non_local_knowledge(std::move(pre->now), rhs);
	now = search_and_destroy_derefs(std::move(now), lhs);
	// do not modify 

	// copy knowledge about rhs over to lhs (do not modify TimePredicates)
	now->conjuncts = container_search_and_inline(std::move(now->conjuncts), rhs, lhs);

	// add new knowledge;
	now->conjuncts.push_back(std::make_unique<ExpressionAxiom>(
		std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, cola::copy(lhs), cola::copy(rhs))
	));

	// done
	pre->now = std::move(now);
	return pre;
}

bool is_owned(const Formula& now, const VariableExpression& var) {
	OwnershipAxiom ownership(std::make_unique<VariableExpression>(var.decl));
	return plankton::implies(now, ownership);
}

bool has_no_flow(const Formula& now, const VariableExpression& var) {
	FlowAxiom flow(cola::copy(var), FlowValue::empty());
	return plankton::implies(now, flow);
}

bool has_enabled_future_predicate(const Formula& now, const Annotation& time, const Assignment& cmd) {
	for (const auto& pred : time.time) {
		auto [is_future, future_pred] = plankton::is_of_type<FuturePredicate>(*pred);
		if (!is_future) continue;
		if (!plankton::syntactically_equal(*future_pred->command->lhs, *cmd.lhs)) continue;
		if (!plankton::syntactically_equal(*future_pred->command->rhs, *cmd.rhs)) continue;
		if (plankton::implies(now, *future_pred->pre)) {
			return true;
		}
	}
	return false;
}

bool keyset_contains(const Formula& now, const VariableExpression& node, const VariableDeclaration& searchKey) {
	KeysetContainsAxiom keyset(std::make_unique<VariableExpression>(node.decl), std::make_unique<VariableExpression>(searchKey));
	return plankton::implies(now, keyset);
}

std::pair<bool, std::unique_ptr<FulfillmentAxiom>> try_fulfill_impure_obligation(const ConjunctionFormula& now, const ObligationAxiom& obligation, const Dereference& lhs, const VariableExpression& lhsVar, const Expression& rhs) {
	std::pair<bool, std::unique_ptr<FulfillmentAxiom>> result;
	if (obligation.kind == ObligationAxiom::Kind::CONTAINS) {
		return result;
	}

	auto& searchKey = obligation.key->decl;
	if (keyset_contains(now, lhsVar, searchKey)) {
		// 'searchKey' in keyset of 'lhsVar.decl' ==> we are guaranteed that 'lhsVar' is the correct node to modify
		result.first = (obligation.kind == ObligationAxiom::Kind::INSERT && thePurityChecker.satisfies_insert(now, searchKey, lhs, lhsVar.decl, rhs))
		            || (obligation.kind == ObligationAxiom::Kind::DELETE && thePurityChecker.satisfies_delete(now, searchKey, lhs, lhsVar.decl, rhs));
	}

	if (result.first) {
		result.second = std::make_unique<FulfillmentAxiom>(obligation.kind, std::make_unique<VariableExpression>(obligation.key->decl), true);
	}

	return result;
}

std::pair<bool, std::unique_ptr<ConjunctionFormula>> ensure_pure_or_spec_data(std::unique_ptr<ConjunctionFormula> now, const Dereference& lhs, const VariableExpression& lhsVar, const Expression& rhs) {
	// pure assignment ==> do nothing
	if (thePurityChecker.is_pure(*now, lhs, lhsVar.decl, rhs)) {
		return std::make_pair(true, std::move(now));
	}

	// check impure specification
	for (auto& conjunct : now->conjuncts) {
		auto [is_obligation, obligation] = plankton::is_of_type<ObligationAxiom>(*conjunct);
		if (is_obligation) {
			auto [success, fulfillment] = try_fulfill_impure_obligation(*now, *obligation, lhs, lhsVar, rhs);
			if (success) {
				conjunct = std::move(fulfillment);
				return std::make_pair(true, std::move(now));
			}
		}
	}

	// neither pure nor satisfies spec
	return std::make_pair(false, std::move(now));
}

std::pair<bool, std::unique_ptr<ConjunctionFormula>> ensure_pure_or_spec_ptr(std::unique_ptr<ConjunctionFormula> now, const Dereference& lhs, const VariableExpression& lhsVar, const Expression& rhs) {
	/*
		Distinguish cases:
		(1) pure deletion: lhsVar->next = y /\ y ->next = rhs /\ content(y)=<empty>
		(2) impure insertion: lhsVar->next = y /\ rhs->next = y /\ contains(rhs, k) /\ OBL(insert, k) /\ "rhs bekommt keyset mit k"

		(3) impure deletion: lhsVar->next = y /\ y ->next = rhs /\ contains(y, k) /\ k \in keyset(y) /\ OBL(delete, k)

	*/
	throw std::logic_error("not yet implemented");
}

template<bool is_ptr>
std::unique_ptr<Annotation> handle_purity(std::unique_ptr<Annotation> pre, const Assignment& cmd, const Dereference& lhs, const VariableExpression& lhsVar, const Expression& rhs) {
	auto now = std::move(pre->now);
	bool success = false;

	// the content of 'lhsVar' is intersected with the empty flow ==> changes are irrelevant
	success = is_owned(*now, lhsVar) || has_no_flow(*now, lhsVar);

	// ensure 'cmd' is pure or satisfies spec ==> depends on 'lhs' being of data or pointer sort
	if (!success) {
		auto result = (is_ptr ? ensure_pure_or_spec_ptr : ensure_pure_or_spec_data)(std::move(now), lhs, lhsVar, rhs);
		success = result.first;
		now = std::move(result.second);
	}

	// the existence of a future predicate guarantees purity
	success = success || has_enabled_future_predicate(*now, *pre, cmd);

	if (!success) {
		throw VerificationError("Impure assignment violates specification or proof obligation.");
	}

	pre->now = std::move(now);
	return pre;
}

std::unique_ptr<Annotation> post_full_assign_derefptr_varimmi(std::unique_ptr<Annotation> pre, const Assignment& cmd, const Dereference& lhs, const VariableExpression& lhsVar, const Expression& rhs) {
	auto post = handle_purity<true>(std::move(pre), cmd, lhs, lhsVar, rhs);
	post = search_and_destroy_and_inline_deref(std::move(pre), lhs, rhs);
	return post;
}

std::unique_ptr<Annotation> post_full_assign_derefdata_varimmi(std::unique_ptr<Annotation> pre, const Assignment& cmd, const Dereference& lhs, const VariableExpression& lhsVar, const Expression& rhs) {
	auto post = handle_purity<false>(std::move(pre), cmd, lhs, lhsVar, rhs);
	post = search_and_destroy_and_inline_deref(std::move(pre), lhs, rhs);
	return post;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct AssignmentExpressionAnalyser : public BaseVisitor {
	const VariableExpression* decl = nullptr;
	const Dereference* deref = nullptr;
	bool derefNested = false;

	void reset() {
		decl = nullptr;
		deref = nullptr;
		derefNested = false;
	}

	void visit(const VariableExpression& node) override {
		decl = &node;
	}

	void visit(const Dereference& node) override {
		if (!deref) {
			deref = &node;
			node.expr->accept(*this);
		} else {
			derefNested = true;
		}
	}

	void visit(const NullValue& /*node*/) override { if (deref) derefNested = true; }
	void visit(const BooleanValue& /*node*/) override {if (deref) derefNested = true; }
	void visit(const EmptyValue& /*node*/) override { if (deref) derefNested = true; }
	void visit(const MaxValue& /*node*/) override { if (deref) derefNested = true; }
	void visit(const MinValue& /*node*/) override { if (deref) derefNested = true; }
	void visit(const NDetValue& /*node*/) override { if (deref) derefNested = true; }
	void visit(const NegatedExpression& /*node*/) override { if (deref) derefNested = true; }
	void visit(const BinaryExpression& /*node*/) override { if (deref) derefNested = true; }

};

inline void throw_unsupported(std::string where, const Expression& expr) {
	std::stringstream msg;
	msg << "compound expression '";
	cola::print(expr, msg);
	msg << "' as " << where << "-hand side of assignment not supported.";
	throw UnsupportedConstructError(msg.str());
}

std::unique_ptr<Annotation> plankton::post_full(std::unique_ptr<Annotation> pre, const Assignment& cmd) {
	std::cout << "Post for assignment:  ";
	cola::print(cmd, std::cout);
	std::cout << "under:" << std::endl;
	plankton::print(*pre, std::cout);
	std::cout << std::endl << std::endl;

	// TODO: make the following 🤮 nicer
	AssignmentExpressionAnalyser analyser;
	cmd.lhs->accept(analyser);
	const VariableExpression* lhsVar = analyser.decl;
	const Dereference* lhsDeref = analyser.deref;
	bool lhsDerefNested = analyser.derefNested;
	analyser.reset();
	cmd.rhs->accept(analyser);
	const VariableExpression* rhsVar = analyser.decl;
	const Dereference* rhsDeref = analyser.deref;
	bool rhsDerefNested = analyser.derefNested;

	if (lhsDerefNested || (lhsDeref == nullptr && lhsVar == nullptr)) {
		// lhs is a compound expression
		std::stringstream msg;
		msg << "compound expression '";
		cola::print(*cmd.lhs, msg);
		msg << "' as left-hand side of assignment not supported.";
		throw UnsupportedConstructError(msg.str());

	} else if (lhsDeref) {
		// lhs is a dereference
		assert(lhsVar);
		if (!rhsDeref) {
			if (lhsDeref->sort() == Sort::PTR) {
				return post_full_assign_derefptr_varimmi(std::move(pre), cmd, *lhsDeref, *lhsVar, *cmd.rhs);
			} else {
				return post_full_assign_derefdata_varimmi(std::move(pre), cmd, *lhsDeref, *lhsVar, *cmd.rhs);
			}
		} else {
			// deref on both sides not supported
			std::stringstream msg;
			msg << "right-hand side '";
			cola::print(*cmd.rhs, msg);
			msg << "' of assignment not supported with '";
			cola::print(*cmd.lhs, msg);
			msg << "' as left-hand side.";
			throw UnsupportedConstructError(msg.str());
		}

	} else if (lhsVar) {
		// lhs is a variable or an immidiate value
		if (rhsDeref) {
			if (rhsDerefNested) {
				// lhs is a compound expression
				std::stringstream msg;
				msg << "compound expression '";
				cola::print(*cmd.rhs, msg);
				msg << "' as right-hand side of assignment not supported.";
				throw UnsupportedConstructError(msg.str());

			} else if (rhsDeref->sort() == Sort::PTR) {
				return post_full_assign_var_derefptr(std::move(pre), cmd, *lhsVar, *rhsDeref, *rhsVar);
			} else {
				return post_full_assign_var_derefdata(std::move(pre), cmd, *lhsVar, *rhsDeref, *rhsVar);
			}
		} else {
			return post_full_assign_var_expr(std::move(pre), cmd, *lhsVar, *cmd.rhs);
		}
	}

	throw std::logic_error("Conditional was expected to be complete.");
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Annotation> plankton::post_full(std::unique_ptr<Annotation> /*pre*/, const Assume& /*cmd*/) {
	throw std::logic_error("not yet implemented: plankton::post(std::unique_ptr<Annotation>, const Assume&)");
}

std::unique_ptr<Annotation> plankton::post_full(std::unique_ptr<Annotation> /*pre*/, const Malloc& /*cmd*/) {
	// TODO: extend_current_annotation(... knowledge about all members of new object, flow...)
	// TODO: destroy knowledge about current lhs (and everything that is not guaranteed to survive the assignment)
	throw std::logic_error("not yet implemented: plankton::post(std::unique_ptr<Annotation>, const Malloc&)");
}

bool plankton::post_maintains_formula(const Formula& /*pre*/, const Formula& /*maintained*/, const cola::Assignment& /*cmd*/) {
	throw std::logic_error("not yet implemented: plankton::post_maintains_formula(const Formula&, const Formula&, const cola::Assignment&)");
}

bool plankton::post_maintains_invariant(const Annotation& /*pre*/, const Formula& /*invariant*/, const cola::Assignment& /*cmd*/) {
	throw std::logic_error("not yet implemented: plankton::post_maintains_invariant(const Annotation&, const Formula&, const cola::Assignment&)");
}
