#include "engine/util.hpp"

#include "logics/util.hpp"
#include "util/shortcuts.hpp"
#include "util/log.hpp"

using namespace plankton;


struct Generator {
    ExtensionPolicy policy;
    std::set<const SymbolDeclaration*> symbols;
    std::set<const SymbolDeclaration*> flows;
    std::deque<std::unique_ptr<Axiom>> result;
    
    explicit Generator(ExtensionPolicy policy) : policy(policy) {}
    
    inline void AddSymbolsFrom(const LogicObject& object) {
        for (const auto* elem : plankton::CollectUsefulSymbols(object)) {
            switch (elem->order) {
                case Order::FIRST: symbols.insert(elem); break;
                case Order::SECOND: flows.insert(elem); break;
            }
        }
    }
    
    inline std::deque<std::unique_ptr<Axiom>> Generate() {
        result.clear();
        MakeCandidates();
        return std::move(result);
    }
    
private:
    inline void MakeCandidates() {
        // symbols
        auto isPtr = [](auto it){ return (**it).type.sort == Sort::PTR; };
        for (auto symbol = symbols.begin(); symbol != symbols.end(); ++symbol) {
            if (!isPtr(symbol) && !AtLeast(ExtensionPolicy::FAST)) continue;
            AddUnarySymbolCandidates(**symbol);
            for (auto other = std::next(symbol); other != symbols.end(); ++other) {
                AddBinarySymbolCandidates(**symbol, **other);
            }
        }
        
        // flows
        if (!AtLeast(ExtensionPolicy::FAST)) return;
        for (const auto* flow : flows) {
            AddUnaryFlowCandidates(*flow);
            for (auto symbol = symbols.begin(); symbol != symbols.end(); ++symbol) {
                AddBinaryFlowCandidates(*flow, **symbol);
                if (!AtLeast(ExtensionPolicy::FULL)) continue;
                AddMoreBinaryFlowCandidates(*flow, **symbol);
                for (auto other = std::next(symbol); other != symbols.end(); ++other) {
                    AddTernaryFlowCandidates(*flow, **symbol, **other);
                }
            }
        }
    }
    
    [[nodiscard]] inline bool AtLeast(ExtensionPolicy min) const {
        return min <= policy;
    }

    inline static std::unique_ptr<SymbolicVariable> MkVar(const SymbolDeclaration& decl) {
        return std::make_unique<SymbolicVariable>(decl);
    }

    inline static const std::vector<BinaryOperator>& GetOps(Sort sort) {
        static std::vector<BinaryOperator> ptrOps = { BinaryOperator::EQ, BinaryOperator::NEQ };
        static std::vector<BinaryOperator> dataOps = {BinaryOperator::EQ, BinaryOperator::NEQ,
                                                      BinaryOperator::LEQ, BinaryOperator::LT,
                                                      BinaryOperator::GEQ, BinaryOperator::GT};
        return sort == Sort::DATA ? dataOps : ptrOps;
    }

    inline static std::vector<std::unique_ptr<SymbolicExpression>> GetImmis(const Type& type) {
        std::vector<std::unique_ptr<SymbolicExpression>> result;
        result.reserve(2);
        switch (type.sort) {
            case Sort::VOID: break;
            case Sort::BOOL: result.push_back(std::make_unique<SymbolicBool>(true)); result.push_back(std::make_unique<SymbolicBool>(false)); break;
            case Sort::DATA: result.push_back(std::make_unique<SymbolicMin>()); result.push_back(std::make_unique<SymbolicMax>()); break;
            case Sort::TID: result.push_back(std::make_unique<SymbolicSelfTid>()); result.push_back(std::make_unique<SymbolicSomeTid>()); result.push_back(std::make_unique<SymbolicUnlocked>()); break;
            case Sort::PTR: result.push_back(std::make_unique<SymbolicNull>()); break;
        }
        return result;
    }

    inline void AddUnarySymbolCandidates(const SymbolDeclaration& symbol) {
        for (auto op : GetOps(symbol.type.sort)) {
            for (auto&& immi : GetImmis(symbol.type)) {
                result.push_back(std::make_unique<StackAxiom>(op, MkVar(symbol), std::move(immi)));
            }
        }
    }

    inline void AddBinarySymbolCandidates(const SymbolDeclaration& symbol, const SymbolDeclaration& other) {
        if (symbol.type.sort != other.type.sort) return;
        for (auto op : GetOps(symbol.type.sort)) {
            result.push_back(std::make_unique<StackAxiom>(op, MkVar(symbol), MkVar(other)));
        }
    }

    inline void AddUnaryFlowCandidates(const SymbolDeclaration& flow) {
        for (auto value : {true, false}) {
            result.push_back(std::make_unique<InflowEmptinessAxiom>(flow, value));
        }
    }
    
    inline void AddBinaryFlowCandidates(const SymbolDeclaration& flow, const SymbolDeclaration& symbol) {
        if (flow.type != symbol.type) return;
        result.push_back(std::make_unique<InflowContainsValueAxiom>(flow, symbol));
    }
    
    inline void AddMoreBinaryFlowCandidates(const SymbolDeclaration& flow, const SymbolDeclaration& symbol) {
        if (flow.type != symbol.type) return;
        result.push_back(std::make_unique<InflowContainsRangeAxiom>(MkVar(flow), MkVar(symbol), std::make_unique<SymbolicMax>()));
        result.push_back(std::make_unique<InflowContainsRangeAxiom>(MkVar(flow), std::make_unique<SymbolicMin>(), MkVar(symbol)));
    }

    inline void AddTernaryFlowCandidates(const SymbolDeclaration& flow, const SymbolDeclaration& symbol, const SymbolDeclaration& other) {
        if (flow.type != symbol.type) return;
        if (flow.type != other.type) return;
        result.push_back(std::make_unique<InflowContainsRangeAxiom>(flow, symbol, other));
        result.push_back(std::make_unique<InflowContainsRangeAxiom>(flow, other, symbol));
    }
};


std::deque<std::unique_ptr<Axiom>> plankton::MakeStackCandidates(const LogicObject& object, ExtensionPolicy policy) {
    Generator candidates(policy);
    candidates.AddSymbolsFrom(object);
    return candidates.Generate();
}

std::deque<std::unique_ptr<Axiom>> plankton::MakeStackCandidates(const LogicObject& object, const LogicObject& other,
                                                                 ExtensionPolicy policy) {
    auto objectSymbols = plankton::CollectUsefulSymbols(object); // TODO: all or useful symbols?
    auto otherSymbols = plankton::CollectUsefulSymbols(other); // TODO: all or useful symbols?
    plankton::DiscardIf(objectSymbols, [&otherSymbols](const auto* elem) { return plankton::Membership(otherSymbols, elem); });
    plankton::DiscardIf(otherSymbols, [&objectSymbols](const auto* elem) { return plankton::Membership(objectSymbols, elem); });

    Generator candidates(policy);
    candidates.AddSymbolsFrom(object);
    candidates.AddSymbolsFrom(other);

    auto result = candidates.Generate();
    plankton::RemoveIf(result, [&objectSymbols, &otherSymbols](const auto& elem) {
        auto symbols = plankton::Collect<SymbolDeclaration>(*elem);
        return plankton::EmptyIntersection(objectSymbols, symbols)
               || plankton::EmptyIntersection(otherSymbols, symbols);
        // return plankton::EmptyIntersection(objectSymbols, symbols)
        //        && plankton::EmptyIntersection(otherSymbols, symbols);
    });
    return result;
}

void plankton::ExtendStack(Annotation& annotation, Encoding& encoding, ExtensionPolicy policy) {
    // Generator generator(policy);
    // generator.AddSymbolsFrom(annotation);
    // auto candidates = generator.Generate();
    auto candidates = plankton::MakeStackCandidates(*annotation.now, policy);
    for (const auto& past : annotation.past) {
        auto pastCandidates = plankton::MakeStackCandidates(*annotation.now, *past, policy);
        plankton::MoveInto(std::move(pastCandidates), candidates);
    }
    for (const auto& future : annotation.future) {
        auto futureCandidates = plankton::MakeStackCandidates(*annotation.now, *future, policy);
        plankton::MoveInto(std::move(futureCandidates), candidates);
    }
    // DEBUG("plankton::ExtendStack for " << candidates.size() << " candidates" << std::endl)
    
    for (auto& candidate : candidates) {
        encoding.AddCheck(encoding.Encode(*candidate), [&candidate,&annotation](bool holds){
            assert(candidate);
            // DEBUG("  --[" << holds << "] " << *candidate << std::endl)
            if (holds) annotation.Conjoin(std::move(candidate));
        });
    }
    encoding.Check();
}

void plankton::ExtendStack(Annotation& annotation, const SolverConfig& config, ExtensionPolicy policy) {
    Encoding encoding(*annotation.now, config);
    // for (const auto& past : annotation.past) {
    //     encoding.AddPremise(encoding.EncodeInvariants(*past->formula, config));
    // }
    plankton::ExtendStack(annotation, encoding, policy);
}
