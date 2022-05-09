//
// Created by Sebastian Wolff on 4/27/22.
//

#ifndef PLANKTON_LISTENER_HPP
#define PLANKTON_LISTENER_HPP

namespace plankton {


    struct InfoListener final : public ProverAwareProofListener {
        using ProverAwareProofListener::ProverAwareProofListener;
        StatusStack infoPrefix;

        inline std::string InfoSize() {
            return " (" + std::to_string(prover.current.size()) + ") ";
        }

        void BeforeConsolidateDiscoveredInterference() override {
            INFO(infoPrefix << "Checking for new effects. (" << prover.discoveredInterference.size() << ") " << std::endl)
        }
        void BeforeApplyInterference() override { INFO(infoPrefix << "Applying interference." << InfoSize() << std::endl) }
        void BeforeImprovePast() override { INFO(infoPrefix << "Improving past." << InfoSize() << std::endl) }
        void BeforeImproveFuture() override { INFO(infoPrefix << "Improving future." << InfoSize() << std::endl) }
        void BeforeReducePast() override { INFO(infoPrefix << "Minimizing past." << InfoSize() << std::endl) }
        void BeforeReduceFuture() override { INFO(infoPrefix << "Minimizing future." << InfoSize() << std::endl) }
        void BeforeJoin() override { INFO(infoPrefix << "Joining." << InfoSize() << std::endl) }

        void BeforeHandleCommand(const Command& object) {
            INFO(infoPrefix << "Post for '" << object << "'." << InfoSize() << std::endl);
        }
        void BeforeHandleSkip(const Skip& object) { BeforeHandleCommand(object); }
        void BeforeHandleBreak(const Break& object) {
            BeforeHandleCommand(object);
            DEBUG(" BREAKING with (" << current.size() << ") ")
            DEBUG_FOREACH(current, [](const auto& elem){ DEBUG(*elem) })
            DEBUG(std::endl)
        }
        void BeforeHandleAssume(const Assume& object) { BeforeHandleCommand(object); }
        void BeforeHandleFail(const Fail& object) { BeforeHandleCommand(object); }
        void BeforeHandleReturn(const Return& object) {
            BeforeHandleCommand(object);
            DEBUG(" RETURNING " << cmd << " with (" << current.size() << ") ")
            DEBUG_FOREACH(current, [](const auto& elem){ DEBUG(*elem) })
            DEBUG(std::endl)
        }
        void BeforeHandleMalloc(const Malloc& object) { BeforeHandleCommand(object); }
        void BeforeHandleMacro(const Macro& object) {
            // BeforeHandleCommand(object);
            infoPrefix.Push("macro-", cmd.function.get().name);
            INFO(infoPrefix << "Descending into macro '" << cmd.function.get().name << "'..." << std::endl)
            DEBUG(std::endl << "=== pre annotations for macro '" << cmd.Func().name << "':" << std::endl)
            DEBUG_FOREACH(current, [](const auto& elem){ DEBUG("  -- " << *elem << std::endl) })
        }
        void AfterHandleMacro(const Macro& object) {
            INFO(infoPrefix << "Returning from macro '" << cmd.function.get().name << "'..." << std::endl)
            infoPrefix.Pop();
            DEBUG(std::endl << "=== post annotations for macro '" << cmd.Func().name << "':" << std::endl)
            DEBUG_FOREACH(current, [](const auto& elem){ DEBUG("  -- " << *elem << std::endl) })
            DEBUG(std::endl << std::endl)
        }
        void BeforeHandleAcquireLock(const AcquireLock& object) { BeforeHandleCommand(object); }
        void BeforeHandleReleaseLock(const ReleaseLock& object) { BeforeHandleCommand(object); }
        void BeforeHandleVariableAssignment(const VariableAssignment& object) { BeforeHandleCommand(object); }
        void BeforeHandleMemoryWrite(const MemoryWrite& object) { BeforeHandleCommand(object); }
    };

    std::shared_ptr<ProofListener> plankton::MakeInfoListener(Prover& prover) {
        return std::make_shared<InfoListener>(prover);
    }


    struct MeasureListener final : public ProofListener {
        const std::string PREFIX = "PLDI ";
        Timer timerPost = Timer(PREFIX + "Post");
        Timer timerJoin = Timer(PREFIX + "Join");
        Timer timerInterference = Timer(PREFIX + "Interference"),;
        Timer timerPastImprove = Timer(PREFIX + "Past improve");
        Timer timerPastReduce = Timer(PREFIX + "Past reduce"),;
        Timer timerFutureImprove = Timer(PREFIX + "Future improve");
        Timer timerFutureReduce = Timer(PREFIX + "Future reduce");

        void BeforeApplyInterference() override { timerInterference.Start(); }
        void AfterApplyInterference() override { timerInterference.Stop(); }
        void BeforeImprovePast() override { timerPastImprove.Start(); }
        void AfterImprovePast() override { timerPastImprove.Stop(); }
        void BeforeImproveFuture() override { timerFutureImprove.Start(); }
        void AfterImproveFuture() override { timerFutureImprove.Stop(); }
        void BeforeReducePast() override { timerPastReduce.Start(); }
        void AfterReducePast() override { timerPastReduce.Stop(); }
        void BeforeReduceFuture() override { timerFutureReduce.Start(); }
        void AfterReduceFuture() override { timerFutureReduce.Stop(); }
        void BeforeJoin() override { timerJoin.Start(); }
        void AfterJoin() override { timerJoin.Stop(); }
    };

    std::shared_ptr<ProofListener> plankton::MakeMeasureListener(Prover& /*prover*/) {
        return std::make_shared<MeasureListener>();
    }

}

#endif //PLANKTON_LISTENER_HPP
