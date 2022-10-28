#include "engine/proof.hpp"

#include "programs/util.hpp"
#include "logics/util.hpp"
#include "util/shortcuts.hpp"
#include "util/timer.hpp"
#include "test.hpp"

using namespace plankton;


//
// Programs
//

void ProofGenerator::GenerateProof() {
    // TODO: remove debug
    // newInterference = plankton::MakeTestInterference(*program.types.at(0));
    // ConsolidateNewInterference();
    // throw std::logic_error("---point du break---");
    // TODO: check initializer

    INFO(infoPrefix << "Proof generation for '" << program.name << "' initiated." << std::endl)
    INFO(infoPrefix << "Starting with empty interference set." << std::endl)
    futureSuggestions.clear(); // TODO: remove <<<<<<<<<<<<<<<<<<<<<<<<<<=====================================================================||||||||||||||||
    if (futureSuggestions.empty()) {
        INFO(infoPrefix << "Using no future suggestions." << std::endl)
    } else {
        INFO(infoPrefix << "Using the following future suggestions (" << futureSuggestions.size() << "): " << std::endl)
        for (const auto& suggestion : futureSuggestions) INFO(infoPrefix << "   " << *suggestion << std::endl)
    }

    // check API functions
    firstRound = true;
    for (std::size_t counter = 0; counter < setup.proofMaxIterations; ++counter) {
        infoPrefix.Push("iter-", counter);
        INFO(infoPrefix << "Starting iteration " << counter << " of fixed-point iteration..." << std::endl)

        program.Accept(*this);
        if (!ConsolidateNewInterference() & !firstRound) {
            INFO(infoPrefix << "Fixed-point reached." << std::endl)
            infoPrefix.Pop();
            INFO(infoPrefix << "Proof generation was successful!" << std::endl)

            return;
        }

        macroPostTable.clear();
        infoPrefix.Pop();
        if (!firstRound) throw std::logic_error("--- breakpoint ---");
        firstRound = false;
    }
    throw std::logic_error("Aborting: proof does not seem to stabilize."); // TODO: remove / better error handling
}

void ProofGenerator::Visit([[maybe_unused]] const Program& object) {
    assert(&object == &program);
    for (const auto& function : program.apiFunctions) {
        HandleInterfaceFunction(*function);
    }
}

//
// Common api/macro function handling
//

void ProofGenerator::Visit(const Function& func) {
    func.body->Accept(*this);
    assert(breaking.empty());
    if (!current.empty() && !plankton::IsVoid(func)) {
        throw std::logic_error("Detected non-returning path through non-void function '" + func.name + "'."); // TODO: better error handling
    }
    for (auto& annotation : current) {
        returning.emplace_back(std::move(annotation), nullptr);
    }
    current.clear();
}

//
// Interface function
//

inline std::optional<Specification> GetKind(const Function& function) { // TODO: move to utils?
    if (function.name == "contains") {
        return Specification::CONTAINS;
    } else if (function.name == "insert" || function.name == "add") {
        return Specification::INSERT;
    } else if (function.name == "delete" || function.name == "remove") {
        return Specification::DELETE;
    } else if (function.name.rfind("maintenance", 0) == 0) {
        return std::nullopt;
    } else {
        throw std::logic_error("Specification for function '" + function.name + "' unknown, expected one of: 'contains', 'insert', 'add', 'delete', 'remove', 'maintenance'."); // TODO: better error handling
    }
}

inline std::unique_ptr<SymbolicVariable> GetSearchKeyValue(const Annotation& annotation, const Function& function) {
    if (function.parameters.size() != 1) {
        throw std::logic_error("Expected one parameter to function '" + function.name + "', got " + std::to_string(function.parameters.size()) + "."); // TODO: better error handling
    }
    auto& param = *function.parameters.front().get();
    auto variables = plankton::Collect<EqualsToAxiom>(*annotation.now);
    auto find = FindIf(variables, [&param](auto* elem){ return elem->Variable() == param; });
    if (find != variables.end()) return std::make_unique<SymbolicVariable>((**find).value->Decl());
    throw std::logic_error("Could not find search key"); // TODO: better error handling
}

inline std::unique_ptr<ObligationAxiom> MakeObligation(std::unique_ptr<SymbolicVariable> key, const Function& function) {
    auto kind = GetKind(function);
    if (!kind) return nullptr;
    return std::make_unique<ObligationAxiom>(*kind, std::move(key));
}

inline std::unique_ptr<Formula> MakeKeyBounds(const SymbolDeclaration& key) {
    static auto mkLt = [](auto lhs, auto rhs){
        return std::make_unique<StackAxiom>(BinaryOperator::LT, std::move(lhs), std::move(rhs));
    };
    auto result = std::make_unique<SeparatingConjunction>();
    result->Conjoin(mkLt(std::make_unique<SymbolicMin>(), std::make_unique<SymbolicVariable>(key)));
    result->Conjoin(mkLt(std::make_unique<SymbolicVariable>(key), std::make_unique<SymbolicMax>()));
    return result;
}

inline std::pair<std::unique_ptr<Annotation>, bool> MakeInterfaceAnnotation(const Program& program, const Function& function, const Solver& solver) {
    auto result = std::make_unique<Annotation>();
    result = solver.PostEnter(std::move(result), program);
    result = solver.PostEnter(std::move(result), function);
    auto key = GetSearchKeyValue(*result, function);
    result->Conjoin(MakeKeyBounds(key->Decl()));
    auto obligation = MakeObligation(std::move(key), function);
    bool isMaintenance = obligation.get() == nullptr;
    if (obligation) result->Conjoin(std::move(obligation)); // maintenance does not have an obligation
    plankton::Simplify(*result);
    return std::make_pair(std::move(result), isMaintenance);
}

inline bool IsFulfilled(const Annotation& annotation, const Return& command) {
    assert(command.expressions.size() == 1); // TODO: throw?
    auto returnExpression = command.expressions.front().get();
    bool returnValue;
    if (dynamic_cast<const TrueValue*>(returnExpression)) returnValue = true;
    else if (dynamic_cast<const FalseValue*>(returnExpression)) returnValue = false;
    else throw std::logic_error("Cannot detect return value"); // TODO: better error handling
    auto fulfillments = plankton::Collect<FulfillmentAxiom>(*annotation.now);
    return std::any_of(fulfillments.begin(), fulfillments.end(),
                       [returnValue](auto* elem) { return elem->returnValue == returnValue; });
}

void ProofGenerator::HandleInterfaceFunction(const Function& function) {
    assert(function.kind == Function::Kind::API);

    infoPrefix.Push("fun-", function.name);
    INFO(infoPrefix << "Handling function '" << function.name << "'..." << std::endl)
	DEBUG(std::endl << std::endl << std::endl << std::endl << std::endl)
	DEBUG("############################################################" << std::endl)
	DEBUG("############################################################" << std::endl)
	DEBUG("#################### " << function.name << std::endl)
	DEBUG("############################################################" << std::endl)
	DEBUG("############################################################" << std::endl)
	DEBUG(std::endl)

	// reset
    insideAtomic = false;
    returning.clear();
    breaking.clear();
    current.clear();

    // descent into function
    auto [init, isMaintenance] = MakeInterfaceAnnotation(program, function, solver);
    current.push_back(std::move(init));
    function.Accept(*this);
    PruneReturning();

    // check post annotations
    if (isMaintenance) {
        DEBUG(std::endl << std::endl << "=== NO POST CHECK " << function.name << std::endl)
    } else if (firstRound) {
        INFO(infoPrefix << "Skipping linearizability check for function '" << function.name << "' in first round" << std::endl)
        DEBUG(std::endl << std::endl << "=== NO POST CHECK " << function.name << std::endl)
    } else {
        INFO(infoPrefix << "Checking linearizability of function '" << function.name << "'..." << std::endl)
        DEBUG(std::endl << std::endl << "=== CHECKING POST ANNOTATION OF " << function.name << "  " << returning.size() << std::endl)
        for (auto&[annotation, command]: returning) {
            assert(command);
            if (IsFulfilled(*annotation, *command)) continue;
            annotation = solver.ImprovePast(std::move(annotation));
            annotation = solver.TryAddFulfillment(std::move(annotation));
            if (IsFulfilled(*annotation, *command)) continue;
            annotation = solver.TryAddFulfillment(std::move(annotation)); // retry in case Z3 is funny
            if (IsFulfilled(*annotation, *command)) continue; // retry in case Z3 is funny
            DEBUG("Linearizability fail for " << *command << "  in  " << *annotation << std::endl)
            throw std::logic_error(
                    "Could not establish linearizability for function '" + function.name + "'."); // TODO: better error handling
        }
        // throw std::logic_error("--- breakpoint lin ---");
    }
    infoPrefix.Pop();
}
