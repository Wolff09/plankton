#include "plankton/solver/solverimpl.hpp"

#include "plankton/logger.hpp" // TODO: delete
#include "cola/util.hpp"
#include "plankton/config.hpp"
#include "plankton/error.hpp"

using namespace cola;
using namespace plankton;
using var_list_t = const std::vector<const cola::VariableDeclaration*>&;


inline std::unique_ptr<NullValue> MakeNull() { return std::make_unique<NullValue>(); }
inline std::unique_ptr<BooleanValue> MakeBool(bool val) { return std::make_unique<BooleanValue>(val); }
inline std::unique_ptr<MaxValue> MakeMax() { return std::make_unique<MaxValue>(); }
inline std::unique_ptr<MinValue> MakeMin() { return std::make_unique<MinValue>(); }

inline std::unique_ptr<VariableExpression> MakeExpr(const VariableDeclaration& decl) { return std::make_unique<VariableExpression>(decl); }
inline std::unique_ptr<BinaryExpression> MakeExpr(BinaryExpression::Operator op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs) { return std::make_unique<BinaryExpression>(op, std::move(lhs), std::move(rhs)); }
inline std::unique_ptr<Dereference> MakeDeref(const VariableDeclaration& decl, std::string field) { return std::make_unique<Dereference>(MakeExpr(decl), field); }

inline std::unique_ptr<ExpressionAxiom> MakeAxiom(std::unique_ptr<Expression> expr) { return std::make_unique<ExpressionAxiom>(std::move(expr)); }
inline std::unique_ptr<OwnershipAxiom> MakeOwnership(const VariableDeclaration& decl) { return std::make_unique<OwnershipAxiom>(MakeExpr(decl)); }
inline std::unique_ptr<NegatedAxiom> MakeNot(std::unique_ptr<Axiom> axiom) { return std::make_unique<NegatedAxiom>(std::move(axiom)); }
inline std::unique_ptr<ObligationAxiom> MakeObligation(SpecificationAxiom::Kind kind, const VariableDeclaration& decl) { return std::make_unique<ObligationAxiom>(kind, MakeExpr(decl)); }
inline std::unique_ptr<FulfillmentAxiom> MakeFulfillment(SpecificationAxiom::Kind kind, const VariableDeclaration& decl, bool returnValue) { return std::make_unique<FulfillmentAxiom>(kind, MakeExpr(decl), returnValue); }
inline std::unique_ptr<LogicallyContainedAxiom> MakeLogicallyContained(std::unique_ptr<Expression> expr) { return std::make_unique<LogicallyContainedAxiom>(std::move(expr)); }
inline std::unique_ptr<HasFlowAxiom> MakeHasFlow(std::unique_ptr<Expression> expr) { return std::make_unique<HasFlowAxiom>(std::move(expr)); }
inline std::unique_ptr<KeysetContainsAxiom> MakeKeysetContains(std::unique_ptr<Expression> expr, std::unique_ptr<Expression> other) { return std::make_unique<KeysetContainsAxiom>(std::move(expr), std::move(other)); }
inline std::unique_ptr<FlowContainsAxiom> MakeFlowContains(std::unique_ptr<Expression> expr, std::unique_ptr<Expression> other, std::unique_ptr<Expression> more) { return std::make_unique<FlowContainsAxiom>(std::move(expr), std::move(other), std::move(more)); }


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::unique_ptr<Expression>> MakeAllExpressions(const VariableDeclaration& decl) {
	std::vector<std::unique_ptr<Expression>> result;
	result.reserve(decl.type.fields.size() + 1);

	result.push_back(MakeExpr(decl));
	for (auto [declField, declFieldType] : decl.type.fields) {
		result.push_back(MakeDeref(decl, declField));
	}
	return result;
}

enum struct CombinationKind { SINGLE, PAIR };

struct ExpressionStore {
	// all unordered distinct pairs of expressions
	std::deque<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> expressions;
	CombinationKind kind;
	bool skipNull;

	void AddImmediate(const VariableDeclaration& decl);
	void AddCombination(const VariableDeclaration& decl, const VariableDeclaration& other);
	void AddAllCombinations();
	
	ExpressionStore(var_list_t variables, CombinationKind kind, bool skipNull=false) : skipNull(skipNull) {
		auto begin = variables.begin();
		auto end = variables.end();

		switch (kind) {
			case CombinationKind::SINGLE:
				for (auto iter = begin; iter != end; ++iter) {
					AddImmediate(**iter);
				}
				break;

			case CombinationKind::PAIR:
				for (auto iter = begin; iter != end; ++iter) {
					for (auto other = std::next(iter); other != end; ++other) {
						AddCombination(**iter, **other);
					}
				}
				break;
		}
	}
};

void ExpressionStore::AddImmediate(const VariableDeclaration& decl) {
	auto declExpressions = MakeAllExpressions(decl);
	for (auto& expr : declExpressions) {
		if (!skipNull) expressions.push_back({ cola::copy(*expr), MakeNull() });
		expressions.push_back({ cola::copy(*expr), MakeBool(true) });
		expressions.push_back({ cola::copy(*expr), MakeBool(false) });
		expressions.push_back({ cola::copy(*expr), MakeMin() });
		expressions.push_back({ std::move(expr), MakeMax() });
	}
}

void ExpressionStore::AddCombination(const VariableDeclaration& decl, const VariableDeclaration& other) {
	if (&decl == &other) return;
	auto declExpressions = MakeAllExpressions(decl);
	auto otherExpressions = MakeAllExpressions(other);
	for (const auto& declExpr : declExpressions) {
		for (const auto& otherExpr : otherExpressions) {
			expressions.push_back({ cola::copy(*declExpr), cola::copy(*otherExpr) });
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct CandidateGenerator {
	Encoder& encoder;
	var_list_t variables;
	CombinationKind kind;
	std::deque<std::unique_ptr<Axiom>> candidateAxioms;

	CandidateGenerator(Encoder& encoder, var_list_t variables, CombinationKind kind) : encoder(encoder), variables(variables), kind(kind) {}

	void AddExpressions();
	void PopulateAxiom(std::unique_ptr<Axiom> axiom, bool addNegated=true);
	void AddOwnershipAxioms(const VariableDeclaration& decl);
	void AddSpecificationAxioms(const VariableDeclaration& decl);
	void AddLogicallyContainsAxioms(std::unique_ptr<Expression> expr);
	void AddHasFlowAxioms(std::unique_ptr<Expression> expr);
	void AddKeysetContainsAxioms(std::unique_ptr<Expression> expr, std::unique_ptr<Expression> other);
	void AddFlowContainsAxioms(std::unique_ptr<Expression> expr, std::unique_ptr<Expression> other);
	void AddSingleAxioms();
	void AddPairAxioms();
	void AddAxioms();

	std::vector<Candidate> MakeCandidates();
};

inline std::vector<BinaryExpression::Operator> MakeOperators(const Expression& expr, const Expression& other) {
	using OP = BinaryExpression::Operator;
	if (expr.sort() != other.sort()) return {};
	if (expr.sort() == Sort::DATA) return { OP::EQ, OP::LT, OP::LEQ, OP::GT, OP::GEQ, OP::NEQ };
	return { OP::EQ, OP::NEQ };
}

void CandidateGenerator::AddExpressions() {
	ExpressionStore store(variables, kind);
	for (const auto& [lhs, rhs] : store.expressions) {
		assert(lhs);
		assert(rhs);
		// if (plankton::syntactically_equal(*lhs, *rhs)) continue;
		for (auto op : MakeOperators(*lhs, *rhs)) {
			if (plankton::syntactically_equal(*lhs, *rhs)) continue;
			candidateAxioms.push_back(MakeAxiom(MakeExpr(op, cola::copy(*lhs), cola::copy(*rhs))));
		}
	}
}

void CandidateGenerator::PopulateAxiom(std::unique_ptr<Axiom> axiom, bool addNegated) {
	if (addNegated) {
		candidateAxioms.push_back(plankton::copy(*axiom));
		axiom = MakeNot(std::move(axiom));
	}

	candidateAxioms.push_back(std::move(axiom));
}

void CandidateGenerator::AddOwnershipAxioms(const VariableDeclaration& decl) {
	if (decl.type.sort != Sort::PTR) return;
	PopulateAxiom(MakeOwnership(decl));
}

void CandidateGenerator::AddSpecificationAxioms(const VariableDeclaration& decl) {
	if (decl.type.sort != Sort::DATA) return;
	for (auto kind : SpecificationAxiom::KindIteratable) {
		PopulateAxiom(MakeObligation(kind, decl), false);
		PopulateAxiom(MakeFulfillment(kind, decl, true), false);
		PopulateAxiom(MakeFulfillment(kind, decl, false), false);
	}
}

void CandidateGenerator::AddLogicallyContainsAxioms(std::unique_ptr<Expression> expr) {
	if (expr->sort() != Sort::DATA) return;
	PopulateAxiom(MakeLogicallyContained(std::move(expr)));
}

void CandidateGenerator::AddHasFlowAxioms(std::unique_ptr<Expression> expr) {
	if (expr->sort() != Sort::PTR) return;
	PopulateAxiom(MakeHasFlow(std::move(expr)));
}

void CandidateGenerator::AddKeysetContainsAxioms(std::unique_ptr<Expression> expr, std::unique_ptr<Expression> other) {
	if (expr->sort() != Sort::PTR || other->sort() != Sort::DATA) return;
	PopulateAxiom(MakeKeysetContains(std::move(expr), std::move(other)));
}

void CandidateGenerator::AddFlowContainsAxioms(std::unique_ptr<Expression> expr, std::unique_ptr<Expression> other) {
	if (expr->sort() != Sort::PTR || other->sort() != Sort::DATA) return;
	PopulateAxiom(MakeFlowContains(std::move(expr), cola::copy(*other), std::move(other)));
}

void CandidateGenerator::AddSingleAxioms() {
	for (auto var : variables) {
		AddOwnershipAxioms(*var);
		AddSpecificationAxioms(*var);
	}

	for (auto var : variables) {
		auto expressions = MakeAllExpressions(*var);
		for (auto& expr : expressions) {
			AddLogicallyContainsAxioms(cola::copy(*expr));
			AddHasFlowAxioms(std::move(expr));
		}
	}
}

void CandidateGenerator::AddPairAxioms() {
	ExpressionStore store(variables, CombinationKind::PAIR, true);
	for (auto& [expr, other] : store.expressions) {
		AddKeysetContainsAxioms(cola::copy(*expr), cola::copy(*other));
		AddKeysetContainsAxioms(cola::copy(*other), cola::copy(*expr));
		AddFlowContainsAxioms(cola::copy(*expr), cola::copy(*other));
		AddFlowContainsAxioms(std::move(other), std::move(expr));
	}
}

void CandidateGenerator::AddAxioms() {
	switch (kind) {
		case CombinationKind::SINGLE: AddSingleAxioms(); break;
		case CombinationKind::PAIR: AddPairAxioms(); break;
	}
}

std::vector<Candidate> CandidateGenerator::MakeCandidates() {
	log() << "### Creating candidates (" << (kind == CombinationKind::SINGLE ? "single" : "pair") << ")..." << std::flush;

	// populate 'candidateAxioms'
	AddExpressions();
	AddAxioms();

	// make candidates
	std::vector<Candidate> result;
	result.reserve(candidateAxioms.size());
	for (const auto& axiom : candidateAxioms) {
		log() << "." << std::flush;
		ImplicationCheckerImpl checker(encoder, *axiom);

		// find other candiate axioms that are implied by 'axiom'
		auto implied = std::make_unique<ConjunctionFormula>();
		for (const auto& other : candidateAxioms) {
			if (!checker.Implies(*other)) continue;
			implied->conjuncts.push_back(plankton::copy(*other));
		}

		// put together
		result.emplace_back(plankton::copy(*axiom), std::move(implied));
	}
	log() << result.size() << std::endl;

	// sort
	std::sort(result.begin(), result.end(), [](const Candidate& lhs, const Candidate& rhs){
		if (lhs.GetImplied().conjuncts.size() > rhs.GetImplied().conjuncts.size()) return true;
		return &lhs.GetCheck() > &rhs.GetCheck();
	});
	
	// done
	return result;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct CandidateEntry final {
	std::array<std::unique_ptr<VariableDeclaration>, 2> dummyVariables;
	std::vector<Candidate> candidates;
};

class CandidateStore {
	private:
		const SolverImpl& solver;
		std::map<std::pair<const Type*, const Type*>, CandidateEntry> candidateMap;

		CandidateStore(const SolverImpl& solver) : solver(solver) {}
		const CandidateEntry& GetEntry(const Type& type, const Type& other, CombinationKind kind);
		transformer_t MakeTransformer(std::vector<const VariableDeclaration*> from, std::vector<const VariableDeclaration*> to);
		std::vector<Candidate> InstantiateRaw(const VariableDeclaration& decl, const VariableDeclaration& other, CombinationKind kind);
		std::vector<Candidate> MakeSingleCandidates(const VariableDeclaration& decl);
		std::vector<Candidate> MakePairCandidates(const VariableDeclaration& decl, const VariableDeclaration& other);
		std::vector<Candidate> MakeCandidates();

	public:
		static CandidateStore& GetStore(const SolverImpl& solver);
		static std::vector<Candidate> MakeCandidates(const SolverImpl& solver);
};

const CandidateEntry& CandidateStore::GetEntry(const Type& type, const Type& other, CombinationKind kind) {
	// search for existing
	auto key = std::make_pair(&type, &other);
	auto find = candidateMap.find(key);
	if (find != candidateMap.end()) return find->second;

	// create new
	CandidateEntry entry;
	entry.dummyVariables[0] = std::make_unique<VariableDeclaration>("CandidateStore#dummy-0", type, false);
	entry.dummyVariables[1] = std::make_unique<VariableDeclaration>("CandidateStore#dummy-1", other, false);
	std::vector<const VariableDeclaration*> vars = { entry.dummyVariables.at(0).get(), entry.dummyVariables.at(1).get() };
	CandidateGenerator generator(solver.GetEncoder(), vars, kind);
	entry.candidates = generator.MakeCandidates();

	// add
	auto insert = candidateMap.emplace(key, std::move(entry));
	assert(insert.second);
	return insert.first->second;
}

transformer_t CandidateStore::MakeTransformer(std::vector<const VariableDeclaration*> from, std::vector<const VariableDeclaration*> to) {
	assert(from.size() == to.size());
	return [search=std::move(from),replace=std::move(to)](const Expression& expr){
		std::pair<bool,std::unique_ptr<cola::Expression>> result;
		auto [isVar, var] = plankton::is_of_type<VariableExpression>(expr);
		if (isVar) {
			for (std::size_t index = 0; index < search.size(); ++index) {
				if (&var->decl != search.at(index)) continue;
				result.first = true;
				result.second = std::make_unique<VariableExpression>(*replace.at(index));
				break;
			}
		}
		return result;
	};
}

std::vector<Candidate> CandidateStore::InstantiateRaw(const VariableDeclaration& decl, const VariableDeclaration& other, CombinationKind kind) {
	const auto& entry = GetEntry(decl.type, other.type, kind);
	const auto& dummies = entry.dummyVariables;
	auto transformer = MakeTransformer({ dummies.at(0).get(), dummies.at(1).get() }, { &decl, &other });
	std::vector<Candidate> result;
	result.reserve(entry.candidates.size());
	for (const auto& candidate : entry.candidates) {
		result.emplace_back(plankton::replace_expression(plankton::copy(*candidate.repr), transformer));
	}
	return result;
}

std::vector<Candidate> CandidateStore::MakeSingleCandidates(const VariableDeclaration& decl) {
	static Type dummyType("CandidateStore#MakeSingleCandidates#dummy-type", Sort::VOID);
	static VariableDeclaration dummyDecl("CandidateStore#MakeSingleCandidates#dummy-decl", dummyType, false);
	return InstantiateRaw(decl, dummyDecl, CombinationKind::SINGLE);
}

std::vector<Candidate> CandidateStore::MakePairCandidates(const VariableDeclaration& decl, const VariableDeclaration& other) {
	if (&decl.type > &other.type) return MakePairCandidates(other, decl);
	return InstantiateRaw(decl, other, CombinationKind::PAIR);
}

std::vector<Candidate> CandidateStore::MakeCandidates() {
	const auto& variables = solver.GetVariablesInScope();
	std::deque<Candidate> candidates;
	auto AddCandidates = [&candidates](std::vector<Candidate>&& newCandidates) {
		candidates.insert(candidates.end(), std::make_move_iterator(newCandidates.begin()), std::make_move_iterator(newCandidates.end()));
	};

	// singleton candidates
	for (auto var : variables) {
		AddCandidates(MakeSingleCandidates(*var));
	}

	// pair candidates (distinct unsorted pairs)
	for (auto iter = variables.begin(); iter != variables.end(); ++iter) {
		for (auto other = std::next(iter); other != variables.end(); ++other) {
			AddCandidates(MakePairCandidates(**iter, **other));
		}
	}

	// create false
	std::vector<Candidate> result;
	result.reserve(candidates.size() + 1);
	bool addFalse = plankton::config.add_false_candidate;
	if (addFalse) result.emplace_back(std::make_unique<ExpressionAxiom>(std::make_unique<BooleanValue>(false)));

	// prune candidates and complete false
	auto IsContained = [&result](const Candidate& search) {
		return std::find_if(result.cbegin(), result.cend(), [&search](const Candidate& elem) {
			return plankton::syntactically_equal(search.GetCheck(), elem.GetCheck());
		}) != result.end();
	};
	for (auto& candidate : candidates) {
		if (IsContained(candidate)) continue;
		if (addFalse) result.front().repr = plankton::conjoin(std::move(result.front().repr), plankton::copy(*candidate.repr));
		result.push_back(std::move(candidate));
	}

	// done
	return result;
}

CandidateStore& CandidateStore::GetStore(const SolverImpl& solver) {
	static std::map<const SolverImpl*, CandidateStore> lookup;
	auto find = lookup.find(&solver);
	if (find != lookup.end()) return find->second;
	auto insertion = lookup.emplace(&solver, CandidateStore(solver));
	return insertion.first->second;
}

std::vector<Candidate> CandidateStore::MakeCandidates(const SolverImpl& solver) {
	return GetStore(solver).MakeCandidates();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr std::string_view ERR_ENTER_PROGRAM = "Cannot enter program scope: program scope must be the outermost scope.";
static constexpr std::string_view ERR_ENTER_NOSCOPE = "Cannot enter scope: there must be an outermost program scope added first.";
static constexpr std::string_view ERR_LEAVE_NOSCOPE = "Cannot leave scope: there is no scope left to leave.";

inline void fail_if(bool fail, const std::string_view msg) {
	if (fail) {
		throw SolvingError(std::string(msg));
	}
}

void SolverImpl::ExtendCurrentScope(const std::vector<std::unique_ptr<cola::VariableDeclaration>>& vars) {
	assert(!candidates.empty());
	assert(!variablesInScope.empty());
	assert(!instantiatedInvariants.empty());

	for (const auto& decl : vars) {
		variablesInScope.back().push_back(decl.get());

		if (cola::assignable(config.invariant->vars.at(0)->type, decl->type)) {
			instantiatedInvariants.back() = plankton::conjoin(std::move(instantiatedInvariants.back()), config.invariant->instantiate(*decl));
		}
	}

	// TODO important: ensure that the candidates can generate the invariant
	// TODO: reuse old candidates?
	auto newCandidates = CandidateStore::MakeCandidates(*this);
	candidates.back() = std::move(newCandidates);
}

void SolverImpl::EnterScope(const cola::Program& program) {
	fail_if(!candidates.empty(), ERR_ENTER_PROGRAM);
	candidates.emplace_back();
	variablesInScope.emplace_back();
	instantiatedInvariants.push_back(std::make_unique<ConjunctionFormula>());

	ExtendCurrentScope(program.variables);
}

inline std::vector<Candidate> CopyCandidates(const std::vector<Candidate>& candidates) {
	std::vector<Candidate> result;
	result.reserve(candidates.size());
	for (const auto& candidate : candidates) {
		result.emplace_back(plankton::copy(*candidate.repr));
	}
	return result;
}

void SolverImpl::PushOuterScope() {
	// copy from outermost scope (front)
	fail_if(candidates.empty(), ERR_ENTER_NOSCOPE);
	candidates.push_back(CopyCandidates(candidates.front()));
	instantiatedInvariants.push_back(plankton::copy(*instantiatedInvariants.front()));
	variablesInScope.push_back(std::vector<const VariableDeclaration*>(variablesInScope.front()));
}

void SolverImpl::PushInnerScope() {
	// copy from innermost scope (back)
	fail_if(candidates.empty(), ERR_ENTER_NOSCOPE);
	candidates.push_back(CopyCandidates(candidates.back()));
	instantiatedInvariants.push_back(plankton::copy(*instantiatedInvariants.back()));
	variablesInScope.push_back(std::vector<const VariableDeclaration*>(variablesInScope.back()));
}

void SolverImpl::EnterScope(const cola::Function& function) {
	PushOuterScope();
	ExtendCurrentScope(function.args);
	ExtendCurrentScope(function.returns);
}

void SolverImpl::EnterScope(const cola::Scope& scope) {
	PushInnerScope();
	ExtendCurrentScope(scope.variables);
}

void SolverImpl::LeaveScope() {
	fail_if(candidates.empty(), ERR_LEAVE_NOSCOPE);
	instantiatedInvariants.pop_back();
	candidates.pop_back();
	variablesInScope.pop_back();
}
