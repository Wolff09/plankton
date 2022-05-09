#include <chrono>
#include "tclap/CmdLine.h"
#include "cfg2string.hpp"
#include "engine/linearizability.hpp"
#include "parser/parse.hpp"
#include "util/log.hpp"

using namespace plankton;


//
// Command Line
//

struct CommandLineInput {
    std::string pathToInput;
    bool spuriousCasFail = false;
    bool printGist = false;
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
    TCLAP::CmdLine cmd("PLANKTON verification tool for lock-free data structures", ' ', "1.0");
    auto isFile = std::make_unique<IsRegularFileConstraint>("_to_input");
    
    TCLAP::SwitchArg casSwitch("", "no-spurious", "Deactivates Compare-and-Swap failing spuriously", cmd, false);
    TCLAP::SwitchArg gistSwitch("g", "gist", "Print machine readable gist at the very end", cmd, false);
    TCLAP::UnlabeledValueArg<std::string> programArg("input", "Input file with program code and flow definition", true, "", isFile.get(), cmd);
    
    cmd.parse(argc, argv);
    CommandLineInput input;
    input.pathToInput = programArg.getValue();
    input.spuriousCasFail = !casSwitch.getValue();
    input.printGist = gistSwitch.getValue();

     //input.pathToInput = "/Users/wolff/Tools/plankton/examples/test.txt";
    // input.pathToInput = "/Users/wolff/Tools/plankton/examples/VechevYahavDCas.pl";
    // input.pathToInput = "/Users/wolff/Tools/plankton/examples/VechevYahavCas.pl";
    // input.pathToInput = "/Users/wolff/Tools/plankton/examples/ORVYY.pl";
    // input.pathToInput = "/Users/wolff/Tools/plankton/examples/Michael.pl";
    // input.pathToInput = "/Users/wolff/Tools/plankton/examples/Harris.pl";

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

inline VerificationResult Verify(const ParsingResult& input) {
    VerificationResult result;
    auto begin = std::chrono::steady_clock::now();
    result.linearizable = plankton::IsLinearizable(*input.program, *input.config);
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
    //INFO("[input program]" << std::endl << *input.program << std::endl)
    //INFO("[input flow & invariants]" << std::endl << ConfigToString(*input.config, *input.program) << std::endl)
}

inline void PrintResult(const CommandLineInput& cmd, const ParsingResult& input, const VerificationResult& result) {
    INFO(std::endl << std::endl)
    INFO("#" << std::endl)
    INFO("# Verdict for " << input.program->name << ":" << std::endl)
    INFO("#   is correct/linearizable: " << (result.linearizable ? "YES" : "NO") << std::endl)
    INFO("#   time taken (ms): " << result.timeTaken.count() << std::endl)
    INFO("#" << std::endl << std::endl)
    
    if (!cmd.printGist) return;
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
        auto result = Verify(input);
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
