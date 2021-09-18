#include "logics/util.hpp"

#include <set>
#include <sstream>
#include "util/shortcuts.hpp"

using namespace plankton;


struct SymbolReplacementListener : public MutableLogicListener {
    const SymbolDeclaration& search;
    const SymbolDeclaration& replace;
    
    explicit SymbolReplacementListener(const SymbolDeclaration& search, const SymbolDeclaration& replace)
            : search(search), replace(replace) {}
    
    void Enter(SymbolicVariable& object) override {
        if (object.Decl() != search) return;
        object.decl = replace;
    }
};

inline void ApplyRenaming(LogicObject& object, const SymbolDeclaration& search, const SymbolDeclaration& replace) {
    SymbolReplacementListener replacer(search, replace);
    object.Accept(replacer);
}

//
// Flatten
//

struct FlattenVisitor final : public MutableDefaultLogicVisitor {
    
    void Visit(SeparatingConjunction& object) override {
        // find all nested separating conjunctions
        std::set<SeparatingConjunction*> pullUp;
        for (auto& conjunct : object.conjuncts) {
            conjunct->Accept(*this);
    
            if (auto conjunction = dynamic_cast<SeparatingConjunction*>(conjunct.get())) {
                pullUp.insert(conjunction);
            }
        }
        
        // flatten
        for (auto& conjunction : pullUp) MoveInto(std::move(conjunction->conjuncts), object.conjuncts);
        RemoveIf(object.conjuncts, [&pullUp](const auto& elem) { return plankton::Membership(pullUp, elem.get()); });
    }
    
    void Visit(NonSeparatingImplication& object) override { Walk(object); }
    void Visit(ImplicationSet& object) override { Walk(object); }
    void Visit(PastPredicate& object) override { Walk(object); }
    void Visit(FuturePredicate& object) override { Walk(object); }
    void Visit(Annotation& object) override { Walk(object); }
};

inline void Flatten(LogicObject& object) {
    FlattenVisitor visitor;
    object.Accept(visitor);
}


//
// Inline memories
//

struct EqualityCollection {
    std::set<std::pair<const SymbolDeclaration*, const SymbolDeclaration*>> set;
    
    void Add(const SymbolDeclaration& symbol, const SymbolDeclaration& other) {
        assert(symbol.type == other.type);
        assert(symbol.order == other.order);
        if (&symbol <= &other) set.emplace(&symbol, &other);
        else set.emplace(&other, &symbol);
    }
    
    void Handle(const SharedMemoryCore& memory, SharedMemoryCore& other) {
        if (memory.node->Decl() != other.node->Decl()) return;
        Add(memory.node->Decl(), other.node->Decl());
        Add(memory.flow->Decl(), other.flow->Decl());
        assert(memory.node->Type() == other.node->Type());
        for (const auto& [field, value] : memory.fieldToValue) {
            Add(value->Decl(), other.fieldToValue.at(field)->Decl());
        }
    }
};

inline EqualityCollection ExtractEqualities(SeparatingConjunction& object) {
    EqualityCollection equalities;
    auto memories = plankton::CollectMutable<SharedMemoryCore>(object);
    for (auto it = memories.begin(); it != memories.end(); ++it) {
        for (auto ot = std::next(it); ot != memories.end(); ++ot) {
            equalities.Handle(**it, **ot);
        }
    }
    return equalities;
}

inline void InlineMemories(SeparatingConjunction& object) {
    auto equalities = ExtractEqualities(object);
    for (const auto& [symbol, other] : equalities.set) {
        ApplyRenaming(object, *symbol, *other);
    }
}


//
// Inline Equalities
//

struct SyntacticEqualityInliningListener final : public MutableLogicListener {
    std::reference_wrapper<LogicObject> target;
    explicit SyntacticEqualityInliningListener(LogicObject& target) : target(target) {}
    
    void Enter(StackAxiom& object) override {
        if (object.op != BinaryOperator::EQ) return;
        if (auto lhsVar = dynamic_cast<const SymbolicVariable*>(object.lhs.get())) {
            if (auto rhsVar = dynamic_cast<const SymbolicVariable*>(object.rhs.get())) {
                ApplyRenaming(target, lhsVar->Decl(), rhsVar->Decl());
            }
        }
    }
    
    // do not inline across multiple contexts
    inline void VisitWithinNewContext(LogicObject& newContext, LogicObject& toVisit) {
        auto& oldTarget = target;
        target = newContext;
        toVisit.Accept(*this);
        target = oldTarget;
    }
    void Visit(NonSeparatingImplication& object) override {
        VisitWithinNewContext(object, *object.premise);
        VisitWithinNewContext(*object.conclusion, *object.conclusion);
    }
    void Visit(PastPredicate& object) override {
        VisitWithinNewContext(*object.formula, *object.formula);
    }
    void Visit(FuturePredicate& object) override {
        VisitWithinNewContext(*object.pre, *object.pre);
        VisitWithinNewContext(*object.post, *object.post);
    }
};

inline void InlineEqualities(LogicObject& object) {
    SyntacticEqualityInliningListener inlining(object);
    object.Accept(inlining);
}

//
// Remove trivialities and duplicates
//

struct CleanUpVisitor final : public MutableDefaultLogicVisitor {
    bool removeAxiom = false;
    
    void Visit(StackAxiom& object) override {
        if (object.op != BinaryOperator::EQ) return;
        if (!plankton::SyntacticalEqual(*object.lhs, *object.rhs)) return;
        removeAxiom = true;
    }
    
    void Visit(SeparatingConjunction& object) override {
        assert(!removeAxiom);
        
        for (auto it = object.conjuncts.begin(); it != object.conjuncts.end(); ++it) {
            (*it)->Accept(*this);
            if (removeAxiom) {
                it->reset(nullptr);
                removeAxiom = false;
                continue;
            }
            for (auto other = std::next(it); other != object.conjuncts.end(); ++other) {
                if (!plankton::SyntacticalEqual(**it, **other)) continue;
                it->reset(nullptr);
                break;
            }
        }
    
        RemoveIf(object.conjuncts, [](const auto& elem){ return elem.get() == nullptr; });
    }
    
    void Visit(NonSeparatingImplication& object) override { Walk(object); }
    void Visit(ImplicationSet& object) override { Walk(object); }
    void Visit(PastPredicate& object) override { Walk(object); }
    void Visit(FuturePredicate& object) override { Walk(object); }
    void Visit(Annotation& object) override { Walk(object); }
};

inline void RemoveNoise(LogicObject& object) {
    CleanUpVisitor visitor;
    object.Accept(visitor);
}


//
// Overall algorithms
//

void plankton::Simplify(LogicObject& object) {
    Flatten(object);
}

void plankton::InlineAndSimplify(Annotation& object) {
    // TODO: should this be done for annotations only?
    Flatten(object);
    InlineMemories(*object.now);
    InlineEqualities(object);
    RemoveNoise(object);
}



inline std::set<const SymbolDeclaration*> CollectUsefulSymbols(const Annotation& annotation) {
    struct : public LogicListener {
        void Visit(const StackAxiom&) override { /* do nothing */ }
        void Visit(const InflowEmptinessAxiom&) override { /* do nothing */ }
        void Visit(const InflowContainsValueAxiom&) override { /* do nothing */ }
        void Visit(const InflowContainsRangeAxiom&) override { /* do nothing */ }
        
        std::set<const SymbolDeclaration*> result;
        void Enter(const SymbolDeclaration& object) override { result.insert(&object); }
    } collector;
    annotation.Accept(collector);
    return std::move(collector.result);
}
