#include <utility>

#include "engine/solver.hpp"

#include "programs/util.hpp"

using namespace plankton;


PostImage::PostImage() = default;

PostImage::PostImage(std::unique_ptr<Annotation> post) {
    assert(post);
    annotations.push_back(std::move(post));
}

PostImage::PostImage(std::deque<std::unique_ptr<Annotation>> posts) : annotations(std::move(posts)) {
    assert(plankton::AllNonNull(annotations));
}

PostImage::PostImage(std::unique_ptr<Annotation> post, std::deque<std::unique_ptr<HeapEffect>> effects_) : effects(std::move(effects_)) {
    assert(post);
    annotations.push_back(std::move(post));
    assert(plankton::AllNonNull(effects));
}

PostImage::PostImage(std::deque<std::unique_ptr<Annotation>> posts, std::deque<std::unique_ptr<HeapEffect>> effects_)
        : annotations(std::move(posts)), effects(std::move(effects_)) {
    assert(plankton::AllNonNull(annotations));
    assert(plankton::AllNonNull(effects));
}

FutureSuggestion::FutureSuggestion(std::unique_ptr<MemoryWrite> command_) : guard(std::make_unique<Guard>()), command(std::move(command_)) {
    assert(command);
}

FutureSuggestion::FutureSuggestion(std::unique_ptr<MemoryWrite> command_, std::unique_ptr<Guard> guard_)
        : guard(std::move(guard_)), command(std::move(command_)) {
    assert(command);
    assert(guard);
}


struct AssumptionChecker : DefaultProgramVisitor {
    void Visit(const Malloc& object) override {
        if (!object.lhs->Decl().isShared) return;
        throw std::logic_error("Allocation to shared variable '" + object.lhs->Decl().name + "' not supported.");
    }
    
    template<typename T>
    inline void CheckShared(const T& object) {
        for (const auto& var : object.lhs) {
            if (!var->Decl().isShared) continue;
            throw std::logic_error("Assignment to shared variable '" + var->Decl().name + "' not supported.");
        }
    }
    
    template<typename T>
    inline void CheckDuplicates(const T& object) {
        for (auto it = object.lhs.begin(); it != object.lhs.end(); ++it) {
            auto exprString = plankton::ToString(**it);
            for (auto ot = std::next(it); ot != object.lhs.end(); ++ot) {
                if (exprString != plankton::ToString(**ot)) continue;
                throw std::logic_error("Malformed assignment: multiple occurrences of '" + exprString + "' " +
                "at left-hand side of '" + plankton::ToString(object) + "'.");
            }
        }
    }
    
    void Visit(const VariableAssignment& object) override { CheckShared(object); CheckDuplicates(object); }
    void Visit(const MemoryWrite& object) override { CheckDuplicates(object); }
    void Visit(const Macro& object) override {
        CheckShared(object);
        CheckDuplicates(object);
        if (object.function.get().kind == Function::MACRO) return;
        throw std::logic_error("Cannot call non-macro function '" + object.function.get().name + "'."); // TODO: better error handling
    }
};

Solver::Solver(const Program& program, const SolverConfig& config) : program(program), config(config), dataFlow(program) {
    // sanity check
    AssumptionChecker checker;
    program.Accept(checker);
}
