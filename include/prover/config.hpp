#pragma once
#ifndef PROVER_CONFIG
#define PROVER_CONFIG


namespace prover {

	struct PlanktonConfig {
		bool post_simplify = true;
		bool conjoin_simplify = true;
		bool implies_holistic_check = true;
		bool z3_handle_unknown_result = false;
		bool interference_after_unification = false; // TODO: required?
		bool interference_exhaustive_repetition = false;
		bool post_maintains_formula_quick_check = true;
		bool semantic_unification = true;
		// bool filter_candidates_by_invariant = false;
		bool add_false_candidate = true;

	};

	extern PlanktonConfig config;

} // namespace prover

#endif