#pragma once
#ifndef PLANKTON_PROGRAMS_VISITORS_HPP
#define PLANKTON_PROGRAMS_VISITORS_HPP

namespace plankton {

    // forward declarations
    struct VariableDeclaration;
    struct VariableExpression;
    struct TrueValue;
    struct FalseValue;
    struct MinValue;
    struct MaxValue;
    struct NullValue;
    struct Dereference;
    struct BinaryExpression;
    struct Skip;
    struct Fail;
    struct Break;
    struct Continue;
    struct Return;
    struct Assume;
    struct Malloc;
    struct Macro;
    struct VariableAssignment;
    struct MemoryWrite;
    struct AcquireLock;
    struct ReleaseLock;
    struct Scope;
    struct Atomic;
    struct Sequence;
    struct NondeterministicLoop;
    struct Choice;
    struct AndExpression;
    struct OrExpression;
    struct IfThenElse;
    struct IfElifElse;
    struct WhileLoop;
    struct DoWhileLoop;
    struct ComplexAssume;
    struct ComplexAssert;
    struct CompareAndSwap;
    struct Function;
    struct Program;
    
    //
    // Const visitors
    //
    
    struct ProgramVisitor {
        virtual void Visit(const VariableExpression& object) = 0;
        virtual void Visit(const TrueValue& object) = 0;
        virtual void Visit(const FalseValue& object) = 0;
        virtual void Visit(const MinValue& object) = 0;
        virtual void Visit(const MaxValue& object) = 0;
        virtual void Visit(const NullValue& object) = 0;
        virtual void Visit(const Dereference& object) = 0;
        virtual void Visit(const BinaryExpression& object) = 0;
        virtual void Visit(const Skip& object) = 0;
        virtual void Visit(const Fail& object) = 0;
        virtual void Visit(const Break& object) = 0;
        virtual void Visit(const Continue& object) = 0;
        virtual void Visit(const Return& object) = 0;
        virtual void Visit(const Assume& object) = 0;
        virtual void Visit(const Malloc& object) = 0;
        virtual void Visit(const Macro& object) = 0;
        virtual void Visit(const VariableAssignment& object) = 0;
        virtual void Visit(const MemoryWrite& object) = 0;
        virtual void Visit(const AcquireLock& object) = 0;
        virtual void Visit(const ReleaseLock& object) = 0;
        virtual void Visit(const Scope& object) = 0;
        virtual void Visit(const Atomic& object) = 0;
        virtual void Visit(const Sequence& object) = 0;
        virtual void Visit(const NondeterministicLoop& object) = 0;
        virtual void Visit(const Choice& object) = 0;
        virtual void Visit(const AndExpression& object) = 0;
        virtual void Visit(const OrExpression& object) = 0;
        virtual void Visit(const IfThenElse& object) = 0;
        virtual void Visit(const IfElifElse& object) = 0;
        virtual void Visit(const WhileLoop& object) = 0;
        virtual void Visit(const DoWhileLoop& object) = 0;
        virtual void Visit(const ComplexAssume& object) = 0;
        virtual void Visit(const ComplexAssert& object) = 0;
        virtual void Visit(const CompareAndSwap& object) = 0;
        virtual void Visit(const Function& object) = 0;
        virtual void Visit(const Program& object) = 0;
    };
    
    /**
     * ProgramVisitor that throws an exception in every function, unless overridden.
     */
    struct BaseProgramVisitor : public ProgramVisitor {
        void Visit(const VariableExpression& object) override;
        void Visit(const TrueValue& object) override;
        void Visit(const FalseValue& object) override;
        void Visit(const MinValue& object) override;
        void Visit(const MaxValue& object) override;
        void Visit(const NullValue& object) override;
        void Visit(const Dereference& object) override;
        void Visit(const BinaryExpression& object) override;
        void Visit(const Skip& object) override;
        void Visit(const Fail& object) override;
        void Visit(const Break& object) override;
        void Visit(const Continue& object) override;
        void Visit(const Return& object) override;
        void Visit(const Assume& object) override;
        void Visit(const Malloc& object) override;
        void Visit(const Macro& object) override;
        void Visit(const VariableAssignment& object) override;
        void Visit(const MemoryWrite& object) override;
        void Visit(const AcquireLock& object) override;
        void Visit(const ReleaseLock& object) override;
        void Visit(const Scope& object) override;
        void Visit(const Atomic& object) override;
        void Visit(const Sequence& object) override;
        void Visit(const NondeterministicLoop& object) override;
        void Visit(const Choice& object) override;
        void Visit(const AndExpression& object) override;
        void Visit(const OrExpression& object) override;
        void Visit(const IfThenElse& object) override;
        void Visit(const IfElifElse& object) override;
        void Visit(const WhileLoop& object) override;
        void Visit(const DoWhileLoop& object) override;
        void Visit(const ComplexAssume& object) override;
        void Visit(const ComplexAssert& object) override;
        void Visit(const CompareAndSwap& object) override;
        void Visit(const Function& object) override;
        void Visit(const Program& object) override;
    };
    
    /**
     * ProgramVisitor that does nothing, unless overridden.
     */
    struct DefaultProgramVisitor : public ProgramVisitor {
        void Visit(const VariableExpression& object) override;
        void Visit(const TrueValue& object) override;
        void Visit(const FalseValue& object) override;
        void Visit(const MinValue& object) override;
        void Visit(const MaxValue& object) override;
        void Visit(const NullValue& object) override;
        void Visit(const Dereference& object) override;
        void Visit(const BinaryExpression& object) override;
        void Visit(const Skip& object) override;
        void Visit(const Fail& object) override;
        void Visit(const Break& object) override;
        void Visit(const Continue& object) override;
        void Visit(const Return& object) override;
        void Visit(const Assume& object) override;
        void Visit(const Malloc& object) override;
        void Visit(const Macro& object) override;
        void Visit(const VariableAssignment& object) override;
        void Visit(const MemoryWrite& object) override;
        void Visit(const AcquireLock& object) override;
        void Visit(const ReleaseLock& object) override;
        void Visit(const Scope& object) override;
        void Visit(const Atomic& object) override;
        void Visit(const Sequence& object) override;
        void Visit(const NondeterministicLoop& object) override;
        void Visit(const Choice& object) override;
        void Visit(const AndExpression& object) override;
        void Visit(const OrExpression& object) override;
        void Visit(const IfThenElse& object) override;
        void Visit(const IfElifElse& object) override;
        void Visit(const WhileLoop& object) override;
        void Visit(const DoWhileLoop& object) override;
        void Visit(const ComplexAssume& object) override;
        void Visit(const ComplexAssert& object) override;
        void Visit(const CompareAndSwap& object) override;
        void Visit(const Function& object) override;
        void Visit(const Program& object) override;
    };
    
    /**
     * ProgramVisitor that walks the AST.
     */
    struct ProgramListener : public ProgramVisitor {
        void Visit(const VariableExpression& object) override;
        void Visit(const TrueValue& object) override;
        void Visit(const FalseValue& object) override;
        void Visit(const MinValue& object) override;
        void Visit(const MaxValue& object) override;
        void Visit(const NullValue& object) override;
        void Visit(const Dereference& object) override;
        void Visit(const BinaryExpression& object) override;
        void Visit(const Skip& object) override;
        void Visit(const Fail& object) override;
        void Visit(const Break& object) override;
        void Visit(const Continue& object) override;
        void Visit(const Return& object) override;
        void Visit(const Assume& object) override;
        void Visit(const Malloc& object) override;
        void Visit(const Macro& object) override;
        void Visit(const VariableAssignment& object) override;
        void Visit(const MemoryWrite& object) override;
        void Visit(const AcquireLock& object) override;
        void Visit(const ReleaseLock& object) override;
        void Visit(const Scope& object) override;
        void Visit(const Atomic& object) override;
        void Visit(const Sequence& object) override;
        void Visit(const NondeterministicLoop& object) override;
        void Visit(const Choice& object) override;
        void Visit(const AndExpression& object) override;
        void Visit(const OrExpression& object) override;
        void Visit(const IfThenElse& object) override;
        void Visit(const IfElifElse& object) override;
        void Visit(const WhileLoop& object) override;
        void Visit(const DoWhileLoop& object) override;
        void Visit(const ComplexAssume& object) override;
        void Visit(const ComplexAssert& object) override;
        void Visit(const CompareAndSwap& object) override;
        void Visit(const Function& object) override;
        void Visit(const Program& object) override;
        
        virtual void Enter(const VariableDeclaration& object);
        virtual void Enter(const VariableExpression& object);
        virtual void Enter(const TrueValue& object);
        virtual void Enter(const FalseValue& object);
        virtual void Enter(const MinValue& object);
        virtual void Enter(const MaxValue& object);
        virtual void Enter(const NullValue& object);
        virtual void Enter(const Dereference& object);
        virtual void Enter(const BinaryExpression& object);
        virtual void Enter(const Skip& object);
        virtual void Enter(const Fail& object);
        virtual void Enter(const Break& object);
        virtual void Enter(const Continue& object);
        virtual void Enter(const Return& object);
        virtual void Enter(const Assume& object);
        virtual void Enter(const Malloc& object);
        virtual void Enter(const Macro& object);
        virtual void Enter(const VariableAssignment& object);
        virtual void Enter(const MemoryWrite& object);
        virtual void Enter(const AcquireLock& object);
        virtual void Enter(const ReleaseLock& object);
        virtual void Enter(const Scope& object);
        virtual void Enter(const Atomic& object);
        virtual void Enter(const Sequence& object);
        virtual void Enter(const NondeterministicLoop& object);
        virtual void Enter(const Choice& object);
        virtual void Enter(const AndExpression& object);
        virtual void Enter(const OrExpression& object);
        virtual void Enter(const IfThenElse& object);
        virtual void Enter(const IfElifElse& object);
        virtual void Enter(const WhileLoop& object);
        virtual void Enter(const DoWhileLoop& object);
        virtual void Enter(const ComplexAssume& object);
        virtual void Enter(const ComplexAssert& object);
        virtual void Enter(const CompareAndSwap& object);
        virtual void Enter(const Function& object);
        virtual void Enter(const Program& object);
    };
    
    //
    // Non-const visitors
    //
    
    struct MutableProgramVisitor {
        virtual void Visit(VariableExpression& object) = 0;
        virtual void Visit(TrueValue& object) = 0;
        virtual void Visit(FalseValue& object) = 0;
        virtual void Visit(MinValue& object) = 0;
        virtual void Visit(MaxValue& object) = 0;
        virtual void Visit(NullValue& object) = 0;
        virtual void Visit(Dereference& object) = 0;
        virtual void Visit(BinaryExpression& object) = 0;
        virtual void Visit(Skip& object) = 0;
        virtual void Visit(Fail& object) = 0;
        virtual void Visit(Break& object) = 0;
        virtual void Visit(Continue& object) = 0;
        virtual void Visit(Return& object) = 0;
        virtual void Visit(Assume& object) = 0;
        virtual void Visit(Malloc& object) = 0;
        virtual void Visit(Macro& object) = 0;
        virtual void Visit(VariableAssignment& object) = 0;
        virtual void Visit(MemoryWrite& object) = 0;
        virtual void Visit(AcquireLock& object) = 0;
        virtual void Visit(ReleaseLock& object) = 0;
        virtual void Visit(Scope& object) = 0;
        virtual void Visit(Atomic& object) = 0;
        virtual void Visit(Sequence& object) = 0;
        virtual void Visit(NondeterministicLoop& object) = 0;
        virtual void Visit(Choice& object) = 0;
        virtual void Visit(AndExpression& object) = 0;
        virtual void Visit(OrExpression& object) = 0;
        virtual void Visit(IfThenElse& object) = 0;
        virtual void Visit(IfElifElse& object) = 0;
        virtual void Visit(WhileLoop& object) = 0;
        virtual void Visit(DoWhileLoop& object) = 0;
        virtual void Visit(ComplexAssume& object) = 0;
        virtual void Visit(ComplexAssert& object) = 0;
        virtual void Visit(CompareAndSwap& object) = 0;
        virtual void Visit(Function& object) = 0;
        virtual void Visit(Program& object) = 0;
    };
    
    /**
     * MutableProgramVisitor that throws an exception in every function, unless overridden.
     */
    struct MutableBaseProgramVisitor : public MutableProgramVisitor {
        void Visit(VariableExpression& object) override;
        void Visit(TrueValue& object) override;
        void Visit(FalseValue& object) override;
        void Visit(MinValue& object) override;
        void Visit(MaxValue& object) override;
        void Visit(NullValue& object) override;
        void Visit(Dereference& object) override;
        void Visit(BinaryExpression& object) override;
        void Visit(Skip& object) override;
        void Visit(Fail& object) override;
        void Visit(Break& object) override;
        void Visit(Continue& object) override;
        void Visit(Return& object) override;
        void Visit(Assume& object) override;
        void Visit(Malloc& object) override;
        void Visit(Macro& object) override;
        void Visit(VariableAssignment& object) override;
        void Visit(MemoryWrite& object) override;
        void Visit(AcquireLock& object) override;
        void Visit(ReleaseLock& object) override;
        void Visit(Scope& object) override;
        void Visit(Atomic& object) override;
        void Visit(Sequence& object) override;
        void Visit(NondeterministicLoop& object) override;
        void Visit(Choice& object) override;
        void Visit(AndExpression& object) override;
        void Visit(OrExpression& object) override;
        void Visit(IfThenElse& object) override;
        void Visit(IfElifElse& object) override;
        void Visit(WhileLoop& object) override;
        void Visit(DoWhileLoop& object) override;
        void Visit(ComplexAssume& object) override;
        void Visit(ComplexAssert& object) override;
        void Visit(CompareAndSwap& object) override;
        void Visit(Function& object) override;
        void Visit(Program& object) override;
    };
    
    /**
     * MutableProgramVisitor that does nothing, unless overridden.
     */
    struct MutableDefaultProgramVisitor : public MutableProgramVisitor {
        void Visit(VariableExpression& object) override;
        void Visit(TrueValue& object) override;
        void Visit(FalseValue& object) override;
        void Visit(MinValue& object) override;
        void Visit(MaxValue& object) override;
        void Visit(NullValue& object) override;
        void Visit(Dereference& object) override;
        void Visit(BinaryExpression& object) override;
        void Visit(Skip& object) override;
        void Visit(Fail& object) override;
        void Visit(Break& object) override;
        void Visit(Continue& object) override;
        void Visit(Return& object) override;
        void Visit(Assume& object) override;
        void Visit(Malloc& object) override;
        void Visit(Macro& object) override;
        void Visit(VariableAssignment& object) override;
        void Visit(MemoryWrite& object) override;
        void Visit(AcquireLock& object) override;
        void Visit(ReleaseLock& object) override;
        void Visit(Scope& object) override;
        void Visit(Atomic& object) override;
        void Visit(Sequence& object) override;
        void Visit(NondeterministicLoop& object) override;
        void Visit(Choice& object) override;
        void Visit(AndExpression& object) override;
        void Visit(OrExpression& object) override;
        void Visit(IfThenElse& object) override;
        void Visit(IfElifElse& object) override;
        void Visit(WhileLoop& object) override;
        void Visit(DoWhileLoop& object) override;
        void Visit(ComplexAssume& object) override;
        void Visit(ComplexAssert& object) override;
        void Visit(CompareAndSwap& object) override;
        void Visit(Function& object) override;
        void Visit(Program& object) override;
    };
    
    /**
     * MutableProgramVisitor that walks the AST.
     */
    struct MutableProgramListener : public MutableProgramVisitor {
        void Visit(VariableExpression& object) override;
        void Visit(TrueValue& object) override;
        void Visit(FalseValue& object) override;
        void Visit(MinValue& object) override;
        void Visit(MaxValue& object) override;
        void Visit(NullValue& object) override;
        void Visit(Dereference& object) override;
        void Visit(BinaryExpression& object) override;
        void Visit(Skip& object) override;
        void Visit(Fail& object) override;
        void Visit(Break& object) override;
        void Visit(Continue& object) override;
        void Visit(Return& object) override;
        void Visit(Assume& object) override;
        void Visit(Malloc& object) override;
        void Visit(Macro& object) override;
        void Visit(VariableAssignment& object) override;
        void Visit(MemoryWrite& object) override;
        void Visit(AcquireLock& object) override;
        void Visit(ReleaseLock& object) override;
        void Visit(Scope& object) override;
        void Visit(Atomic& object) override;
        void Visit(Sequence& object) override;
        void Visit(NondeterministicLoop& object) override;
        void Visit(Choice& object) override;
        void Visit(AndExpression& object) override;
        void Visit(OrExpression& object) override;
        void Visit(IfThenElse& object) override;
        void Visit(IfElifElse& object) override;
        void Visit(WhileLoop& object) override;
        void Visit(DoWhileLoop& object) override;
        void Visit(ComplexAssume& object) override;
        void Visit(ComplexAssert& object) override;
        void Visit(CompareAndSwap& object) override;
        void Visit(Function& object) override;
        void Visit(Program& object) override;
        
        virtual void Enter(const VariableDeclaration& object);
        virtual void Enter(VariableDeclaration& object);
        virtual void Enter(VariableExpression& object);
        virtual void Enter(TrueValue& object);
        virtual void Enter(FalseValue& object);
        virtual void Enter(MinValue& object);
        virtual void Enter(MaxValue& object);
        virtual void Enter(NullValue& object);
        virtual void Enter(Dereference& object);
        virtual void Enter(BinaryExpression& object);
        virtual void Enter(Skip& object);
        virtual void Enter(Fail& object);
        virtual void Enter(Break& object);
        virtual void Enter(Continue& object);
        virtual void Enter(Return& object);
        virtual void Enter(Assume& object);
        virtual void Enter(Malloc& object);
        virtual void Enter(Macro& object);
        virtual void Enter(VariableAssignment& object);
        virtual void Enter(MemoryWrite& object);
        virtual void Enter(AcquireLock& object);
        virtual void Enter(ReleaseLock& object);
        virtual void Enter(Scope& object);
        virtual void Enter(Atomic& object);
        virtual void Enter(Sequence& object);
        virtual void Enter(NondeterministicLoop& object);
        virtual void Enter(Choice& object);
        virtual void Enter(AndExpression& object);
        virtual void Enter(OrExpression& object);
        virtual void Enter(IfThenElse& object);
        virtual void Enter(IfElifElse& object);
        virtual void Enter(WhileLoop& object);
        virtual void Enter(DoWhileLoop& object);
        virtual void Enter(ComplexAssume& object);
        virtual void Enter(ComplexAssert& object);
        virtual void Enter(CompareAndSwap& object);
        virtual void Enter(Function& object);
        virtual void Enter(Program& object);
    };
    
} // namespace plankton

#endif //PLANKTON_PROGRAMS_VISITORS_HPP
