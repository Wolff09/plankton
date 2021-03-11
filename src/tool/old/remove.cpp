#include "heal/util.hpp"

using namespace heal;

std::unique_ptr<ConjunctionFormula> heal::remove_conjuncts_if(std::unique_ptr<ConjunctionFormula> formula, const unary_t<SimpleFormula>& unaryPredicate, bool* changed) {
	auto& container = formula->conjuncts;
	bool performedUpdate = false;
	container.erase(
		std::remove_if(std::begin(container), std::end(container), [&unaryPredicate,&performedUpdate](const auto& uptr) {
			auto result = unaryPredicate(*uptr);
			performedUpdate |= result;
			return result;
		}),
		std::end(container)
	);
	if (changed) *changed = performedUpdate;
	return formula;
}