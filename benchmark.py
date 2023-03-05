# -*- coding: utf8 -*-

import os
import signal
from tempfile import NamedTemporaryFile
from subprocess import Popen, TimeoutExpired
from statistics import mean
import re
import sys

#
# CONFIGURATION begin
#

TIMEOUT = 60 * 60 * 6  # in seconds
REPETITIONS = 1

EXECUTABLE = "./plankton"
BENCHMARKS = {  # path: [flags]
    # "examples/check.pl": [],
    "examples/FineSet.pl": [],
    "examples/LazySet.pl": [],
    "examples/VechevYahavDCas.pl": ["--loopNoPostJoin"],
    "examples/VechevYahavCas.pl": [],
    "examples/ORVYY.pl": [],
    "examples/FemrsTreeNoMaintenance.pl": ["--loopNoPostJoin"],
    "examples/Michael.pl": ["--loopNoPostJoin"],
    "examples/MichaelWaitFreeSearch.pl": [],
    "examples/Harris.pl": ["--future"],
    "examples/HarrisWaitFreeSearch.pl": ["--future"],
    # "examples/LO_abstract.pl": [],
}
LOTREE = ("examples/LO_abstract.pl", [])
WIDTH = 40
STARTED = False

#
# CONFIGURATION end
#

REGEX_GIST = r"@gist\[(?P<path>.*?)\]=(?P<result>[01]),(?P<time>[0-9]*);(.*)"
REGEX_ITER = r"\[iter-(?P<count>[0-9]*)\] Fixed-point reached."
REGEX_EFF = r"Adding effects to solver \((?P<count>[0-9]*)\):"
REGEX_CAN1 = r"Using the following future suggestions \((?P<count>[0-9]*)\):"
REGEX_CAN2 = r"Using no future suggestions."
REGEX_COM = r"\[(?P<time>[0-9]*)ms\] Total time measured for 'TIME Post'"
REGEX_FUT1 = r"\[(?P<time>[0-9]*)ms\] Total time measured for 'TIME Future reduce'"
REGEX_FUT2 = r"\[(?P<time>[0-9]*)ms\] Total time measured for 'TIME Future improve'"
REGEX_HIST1 = r"\[(?P<time>[0-9]*)ms\] Total time measured for 'TIME Past reduce'"
REGEX_HIST2 = r"\[(?P<time>[0-9]*)ms\] Total time measured for 'TIME Past improve'"
REGEX_JOIN = r"\[(?P<time>[0-9]*)ms\] Total time measured for 'TIME Join'"
REGEX_INTER = r"\[(?P<time>[0-9]*)ms\] Total time measured for 'TIME Interference'"

RESULTS = {}


def default(string):
    return str(string)


def err(string):
    return '\033[31m' + str(string) + '\033[0m'
    # return '\033[91m' + str(string) + '\033[0m'


def good(string):
    return '\033[32m' + str(string) + '\033[0m'
    # return '\033[92m' + str(string) + '\033[0m'


def bold(string):
    return '\033[1m' + str(string) + '\033[0m'


def average(array, places=0):
    return round(mean(array), places)


def human_readable(ms):
    m, s = divmod(round(ms/1000, 0), 60)
    return "{}m{:0>2}s".format(int(m), int(s))


class Result:
    success, info, msg = False, "", ""
    total, verdict, iter, eff, can, com, fut, hist, join, inter = 0, False, 0, 0, 0, 0, 0, 0, 0, 0

    def __init__(self, *args):
        if len(args) == 1:
            self.no(args[0], "")
        elif len(args) == 2:
            self.no(args[0], args[1])
        else:
            self.yes(*args)

    def no(self, info, msg):
        self.success = False
        self.info = info
        self.msg = msg

    def yes(self, total, iters, eff, can, com, fut, hist, join, inter):
        self.success = True
        self.info = human_readable(int(total))
        self.total, self.iter, self.eff, self.can, self.com, self.fut, self.hist, self.join, self.inter \
            = int(total), int(iters), int(eff), int(can), int(com), int(fut), int(hist), int(join), int(inter)


def extract_info(output):
    if output == "__to__":
        return Result("timeout")
    if output == "__fail__":
        return Result("error")

    m_gist = re.search(REGEX_GIST, output)
    m_iter = re.search(REGEX_ITER, output)
    m_eff = re.search(REGEX_EFF, output)
    m_can1 = re.search(REGEX_CAN1, output)
    m_can2 = re.search(REGEX_CAN2, output)
    m_com = re.search(REGEX_COM, output)
    m_fut1 = re.search(REGEX_FUT1, output)
    m_fut2 = re.search(REGEX_FUT2, output)
    m_hist1 = re.search(REGEX_HIST1, output)
    m_hist2 = re.search(REGEX_HIST2, output)
    m_join = re.search(REGEX_JOIN, output)
    m_inter = re.search(REGEX_INTER, output)

    if not m_gist:
        return Result("error")
    if m_gist.group("result") != "1":
        return Result("failed")  # TODO: extract error message
    if not m_iter or not m_eff or (not m_can1 and not m_can2) or not m_com or not m_fut1 or not m_fut2 \
            or not m_hist1 or not m_hist2 or not m_join or not m_inter:
        return Result("error")

    total = m_gist.group("time")
    iters = int(m_iter.group("count")) + 1
    eff = int(m_eff.group("count"))
    can = int(m_can1.group("count")) if m_can1 else 0
    com = int(m_com.group("time"))
    fut = int(m_fut1.group("time")) + int(m_fut2.group("time"))
    hist = int(m_hist1.group("time")) + int(m_hist2.group("time"))
    join = int(m_join.group("time"))
    inter = int(m_inter.group("time"))
    return Result(total, iters, eff, can, com, fut, hist, join, inter)


def run_with_timeout_to_file(path, mode, file):
    if path not in BENCHMARKS:
        raise NameError("Internal error: could not find benchmark file '" + path + "'")
    flags = BENCHMARKS.get(path)
    all_args = [EXECUTABLE, path] + flags + [mode] + ["-o " + file.name]

    # make sure to properly kill subprocesses after timeout
    # see: https://stackoverflow.com/questions/36952245/subprocess-timeout-failure
    with Popen(all_args, preexec_fn=os.setsid) as process:
        try:
            process.communicate(timeout=TIMEOUT)
            if process.returncode == 0:
                output = "".join(file.readlines())
            else:
                output = "__fail__"  # TODO: allow error message extraction
        except TimeoutExpired:
            os.killpg(process.pid, signal.SIGINT)
            output = "__to__"
    return output


def run_with_timeout(path, mode):
    with NamedTemporaryFile(mode='r', delete=True) as tmp:
        return run_with_timeout_to_file(path, mode, tmp)


def run_test(path, i, mode):
    print("[{:0>2}/{:0>2}] Running {} {}  ".format(i+1, REPETITIONS, mode, path), flush=True, end="")
    output = run_with_timeout(path, mode)
    result = extract_info(output)
    if result.success:
        print(result.info, flush=True)
    else:
        print(err(result.info), flush=True)
    return result


def run_test_old(path, i):
    return run_test(path, i, "--old")


def run_test_new(path, i):
    return run_test(path, i, "--new")


def get_values(path, precise_past):
    key = (path, "new") if precise_past else (path, "old")
    successes = [x for x in RESULTS.get(key, []) if x.success]
    if len(successes) == 0:
        return False, -1
    else:
        return True, average([x.total for x in successes])


def fmt(string, width, formatter=default):
    result = ("{:>" + str(width) + "}").format(string)
    return formatter(result)


def finalize():
    if not STARTED:
        return
    print()
    print()
    width = max([len(x) for x in BENCHMARKS]) + 3
    header = " {:<" + str(width) + "} | {:>8} | {:>15} | {:>6} "
    print(header.format("Program", "Analysis", "Linearizable", "Factor"))
    print(("-" * width) + "--+----------+-----------------+--------")
    some_failed = False
    for path in BENCHMARKS:
        good_old, total_old = get_values(path, False)
        good_new, total_new = get_values(path, True)
        if not good_old or not good_new:
            some_failed = True
            factor = "--"
            factor_fmt = err if good_old and not good_new else default
        else:
            factor = total_new/total_old
            factor_fmt = good if factor <= 4 else err
            factor = "×{:.2f}".format(factor)
        total_old = human_readable(total_old) + " ✓" if good_old else "failed ✗"
        total_new = human_readable(total_new) + " ✓" if good_new else "failed ✗"
        if good_old and not good_new:
            total_new = fmt(total_new, 15, err)
        factor = bold(fmt(factor, 6, factor_fmt))
        print(header.format(path, "--old", total_old, ""))
        print(header.format("", "--new", total_new, factor))

    if some_failed:
        print()
        print()
        print("Note: due to issues with Z3 and Python's subprocess library, benchmarks might fail spuriously. ")
        print("      Try invoking plankton directly (without pipes), e.g.: {0} path/to/benchmark".format(EXECUTABLE))


def claim1():
    print("CLAIM 1: verification of the LO-tree")
    print("====================================")
    print()
    print()
    log_old, log_new = "<unknown>", "<unknown>"
    with open("LO_old.txt", 'r') as file:
        log_old = file.name
        print("Running --old {}  ".format(LOTREE), flush=True, end="")
        output_old = run_with_timeout_to_file(LOTREE, "--old", file)
        result_old = extract_info(output_old)
        if result_old.success:
            print(result_old.info, flush=True)
        else:
            print(result_old.info, flush=True)
    with open("LO_new.txt", 'r') as file:
        log_new = file.name
        print("Running --new {}  ".format(LOTREE), flush=True, end="")
        output_new = run_with_timeout_to_file(LOTREE, "--new", file)
        result_new = extract_info(output_old)
        if result_new.success:
            print(result_new.info, flush=True)
        else:
            print(err(result_new.info), flush=True)
    print()
    old_expected = "failure"
    old_actual = good("failure ✓") if not result_old.success else err("success ✗")
    new_expected = "success"
    new_actual = good("success ✓") if not result_old.success else err("failure ✗")
    print("Log files have been written to {} and {}.".format(log_old, log_new))
    print("Expected result for '--old' analysis: {}. Actual result: {}".format(old_expected, old_actual))
    print("Expected result for '--new' analysis: {}. Actual result: {}".format(new_expected, new_actual))
    


def claim2():
    print("CLAIM 2: comparison of '--old' and '--new'")
    print("==========================================")
    print()
    print("Settings: iterations={0}, timeout={1}".format(REPETITIONS, human_readable(TIMEOUT*1000)))
    print("Running benchmarks...")
    print()
    STARTED = True
    for i in range(REPETITIONS):
        for path in BENCHMARKS:
            result_old = run_test_old(path, i)
            RESULTS[(path, "old")] = RESULTS.get((path, "old"), []) + [result_old]
            result_new = run_test_new(path, i)
            RESULTS[(path, "new")] = RESULTS.get((path, "new"), []) + [result_new]
    finalize()


def main():
    STARTED = False
    claim1()
    print()
    print()
    claim2()


if __name__ == '__main__':
    try:
        if len(sys.argv) > 1:
            REPETITIONS = int(sys.argv[1])
            if len(sys.argv) > 2:
                TIMEOUT = int(sys.argv[2])
        main()
    except KeyboardInterrupt:
        print("", flush=True)
        print("", flush=True)
        print("[interrupted]", flush=True)
        finalize()
