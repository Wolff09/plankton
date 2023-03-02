#include <chrono>
#include "tclap/CmdLine.h"
#include "cfg2string.hpp"
#include "engine/linearizability.hpp"
#include "engine/setup.hpp"
#include "parser/parse.hpp"
#include "util/log.hpp"

using namespace plankton;


//
// Command Line
//

struct CommandLineInput {
    std::string pathToInput;
    bool spuriousCasFail = false;
    // bool printGist = false;
    EngineSetup setup;
};

struct IsRegularFileConstraint : public TCLAP::Constraint<std::string> {
    std::string id = "path";
    [[nodiscard]] std::string description() const override { return "path to regular file"; }
    [[nodiscard]] std::string shortID() const override { return this->id; }
    [[nodiscard]] bool check(const std::string& path) const override {
        std::ifstream stream(path.c_str());
        return stream.good();
    }
    explicit IsRegularFileConstraint(const std::string& more_verbose="") {
        this->id += more_verbose;
    }
};

inline CommandLineInput Interact(int argc, char** argv) {
    CommandLineInput input;

    TCLAP::CmdLine cmd("PLANKTON verification tool for lock-free data structures", ' ', "1.0");
    auto isFile = std::make_unique<IsRegularFileConstraint>("_to_input");
    
    TCLAP::SwitchArg casSwitch("", "no-spurious", "Deactivates Compare-and-Swap failing spuriously", cmd, false);
    TCLAP::UnlabeledValueArg<std::string> programArg("input", "Input file with program code and flow definition", true, "", isFile.get(), cmd);
    // TCLAP::SwitchArg gistSwitch("g", "gist", "Print machine-readable gist at the very end", cmd, false);

    TCLAP::SwitchArg useFutures("", "future", "Uses futures to handle updates that affect unboundedly many nodes in the heap graph", cmd, false);
    TCLAP::SwitchArg flagNewMode("", "new", "Uses novel hindsight reasoning (temporal interpolation) from PLDI'23", false);
    TCLAP::SwitchArg flagOldMode("", "old", "Uses original hindsight reasoning from OOPSLA'22", false);
    TCLAP::EitherOf modeGroup;
    modeGroup.add(flagNewMode).add(flagOldMode);
    cmd.add(modeGroup);

    TCLAP::SwitchArg loopWidenSwitch("", "loopWiden", "Computes fixed points for loops using a widening, rather than a join", cmd, false);
    TCLAP::SwitchArg loopNoPostJoinSwitch("", "loopNoPostJoin", "Turns off joining loop post annotations", cmd, false);
    TCLAP::SwitchArg macroNoTabulationSwitch("", "macroNoTabulate", "Turns off tabulation of macro post annotations", cmd, false);
    TCLAP::ValueArg<std::size_t> loopMaxIterArg("", "loopMaxIter", "Maximal iterations for finding a loop invariant before aborting", false, 23, "integer", cmd);
    TCLAP::ValueArg<std::size_t> proofMaxIterArg("", "proofMaxIter", "Maximal iterations for finding an interference set before aborting", false, 7, "integer", cmd);

    cmd.parse(argc, argv);
    input.pathToInput = programArg.getValue();
    input.spuriousCasFail = !casSwitch.getValue();
    // input.printGist = gistSwitch.getValue();

    bool newMode = flagNewMode.getValue() || !flagOldMode.getValue();
    input.setup.loopJoinUntilFixpoint = !loopWidenSwitch.getValue();
    input.setup.loopJoinPost = !loopNoPostJoinSwitch.getValue();
    input.setup.macrosTabulateInvocations = !macroNoTabulationSwitch.getValue();
    input.setup.loopMaxIterations = loopMaxIterArg.getValue();
    input.setup.proofMaxIterations = proofMaxIterArg.getValue();
    input.setup.useFutures = useFutures.getValue();
    input.setup.improvePastIncreasedPrecisionForLinearizability = false;
    input.setup.improvePastIncreasedPrecisionForStability = newMode;
    input.setup.improvePastIncreasedPrecisionForAnnotations = newMode;

    return input;
}


//
// Parsing
//

inline ParsingResult Parse(const CommandLineInput& input) {
    return plankton::Parse(input.pathToInput, input.spuriousCasFail);
}


//
// Verification
//

using milliseconds_t = std::chrono::milliseconds;

struct VerificationResult {
    bool linearizable = false;
    milliseconds_t timeTaken = milliseconds_t(0);
};

inline VerificationResult Verify(const ParsingResult& input, const EngineSetup& setup) {
    VerificationResult result;
    auto begin = std::chrono::steady_clock::now();
    result.linearizable = plankton::IsLinearizable(*input.program, *input.config, setup);
    auto end = std::chrono::steady_clock::now();
    result.timeTaken = std::chrono::duration_cast<milliseconds_t>(end - begin);
    return result;
}


//
// Reporting
//

inline void PrintInput(const ParsingResult& input) {
    assert(input.program);
    assert(input.config);
    INFO("[input program]" << std::endl << *input.program << std::endl)
    INFO("[input flow & invariants]" << std::endl << ConfigToString(*input.config, *input.program) << std::endl)
}

inline void PrintResult(const CommandLineInput& cmd, const ParsingResult& input, const VerificationResult& result) {
    INFO(std::endl << std::endl)
    INFO("#" << std::endl)
    INFO("# Verdict for '" << input.program->name << "':" << std::endl)
    INFO("#   is linearizable: " << (result.linearizable ? "YES" : "NO") << std::endl)
    INFO("#   time taken (ms): " << result.timeTaken.count() << std::endl)
    INFO("#" << std::endl << std::endl)
    
    // if (!cmd.printGist) return;
    INFO("@gist[" << cmd.pathToInput << "]=")
    INFO("" << (result.linearizable ? "1" : "0"))
    INFO("," << result.timeTaken.count() << ";")
    INFO(std::endl << std::endl)
}


//
// Main
//

int main(int argc, char** argv) {
    try {
        auto cmd = Interact(argc, argv);
        auto input = Parse(cmd);
        PrintInput(input);
        auto result = Verify(input, cmd.setup);
        PrintResult(cmd, input, result);
        return 0;

    } catch (TCLAP::ArgException& err) {
        // command line misuse
        INFO("ERROR: " << err.error() << " for arg " << err.argId() << std::endl << std::endl)
        ERROR(err.error() << " for arg " << err.argId() << std::endl)
        return 1;

    } catch (std::logic_error& err) { // TODO: catch proper error class
        // plankton error
        INFO(std::endl << std::endl << "ERROR: " << err.what() << std::endl << std::endl)
        ERROR(err.what() << std::endl)
        return 2;
    }
}
