#pragma once
#ifndef PLANKTON_OLDPOST
#define PLANKTON_OLDPOST


#include <memory>
#include "cola/ast.hpp"
#include "plankton/logic.hpp"


namespace plankton {

	/** Computes a strongest post annotation for executing 'cmd' under 'pre'.
	  */
	std::unique_ptr<Annotation> post_full(std::unique_ptr<Annotation> pre, const cola::Assume& cmd);
	
	/** Computes a strongest post annotation for executing 'cmd' under 'pre'.
	  */
	std::unique_ptr<Annotation> post_full(std::unique_ptr<Annotation> pre, const cola::Malloc& cmd);

	/** Computes a strongest post annotation for executing 'cmd' under 'pre'.
	  */
	std::unique_ptr<Annotation> post_full(std::unique_ptr<Annotation> pre, const cola::Assignment& cmd);


	/** Tests whether 'cmd' maintains 'maintained' under 'pre & maintained'.
	  */
	bool post_maintains_formula(const ConjunctionFormula& pre, const ConjunctionFormula& maintained, const cola::Assignment& cmd);

	/** Tests whether 'cmd' maintains 'forall n. invariant(n)' under 'pre & (forall n. invariant(n))'.
	  */
	bool post_maintains_invariant(const Annotation& pre, const cola::Assignment& cmd);

	/** Tests whether 'cmd' maintains 'forall n. invariant(n)' under 'pre & (forall n. invariant(n))'.
	  */
	bool post_maintains_invariant(const Annotation& pre, const cola::Malloc& cmd);

} // namespace plankton

#endif