# -*- coding: utf8 -*-

import os
import signal
from subprocess import Popen, PIPE, TimeoutExpired
from statistics import mean
import re
import sys

#
# CONFIGURATION begin
#

TIMEOUT = 60 * 60 * 6  # in seconds
REPETITIONS = 1

EXECUTABLE = "./plankton"
BENCHMARKS = [
    "examples/FineSet.pl",
    "examples/LazySet.pl",
    "examples/VechevYahavDCas.pl",
    "examples/VechevYahavCas.pl",
    "examples/ORVYY.pl",
    "examples/Michael.pl",
    "examples/MichaelWaitFreeSearch.pl",
    "examples/Harris.pl",
    "examples/HarrisWaitFreeSearch.pl",
    "examples/FemrsTreeNoMaintenance.pl",
]

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

    def yes(self, total, iter, eff, can, com, fut, hist, join, inter):
        self.success = True
        self.info = human_readable(int(total))
        self.total, self.iter, self.eff, self.can, self.com, self.fut, self.hist, self.join, self.inter \
            = int(total), int(iter), int(eff), int(can), int(com), int(fut), int(hist), int(join), int(inter)


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
    iter = int(m_iter.group("count")) + 1
    eff = int(m_eff.group("count"))
    can = int(m_can1.group("count")) if m_can1 else 0
    com = int(m_com.group("time"))
    fut = int(m_fut1.group("time")) + int(m_fut2.group("time"))
    hist = int(m_hist1.group("time")) + int(m_hist2.group("time"))
    join = int(m_join.group("time"))
    inter = int(m_inter.group("time"))
    return Result(total, iter, eff, can, com, fut, hist, join, inter)


def run_with_timeout(path):
    all_args = [EXECUTABLE, "-g", path]

    # make sure to properly kill subprocesses after timeout
    # see: https://stackoverflow.com/questions/36952245/subprocess-timeout-failure
    with Popen(all_args, stderr=PIPE, stdout=PIPE, preexec_fn=os.setsid, universal_newlines=True) as process:
        try:
            output = process.communicate(timeout=TIMEOUT)[0]
            if process.returncode != 0:
                output = "__fail__"  # TODO: allow error message extraction
        except TimeoutExpired:
            os.killpg(process.pid, signal.SIGINT)
            output = "__to__"
    return output


def run_test(path, i):
    output = run_with_timeout(path)
    result = extract_info(output)
    print("[{:0>2}/{:0>2}] {:>12}  for  {:<40}".format(i+1, REPETITIONS, result.info, path), flush=True)
    return result


def finalize():
    print()
    print()
    header = "{:<40} | {:>5} | {:>5} | {:>5} | {:>5} | {:>5} | {:>5} | {:>5} | {:>5} | {:>15}"
    print(header.format("Program", "Iter", "Eff", "Cand", "Com", "Fut", "Hist", "Join", "Inter", "Linearizable"))
    print("-----------------------------------------+-------+-------+-------+-------+-------+-------+-------+-------+-----------------")
    for path in BENCHMARKS:
        successes = [x for x in RESULTS.get(path, []) if x.success]
        if len(successes) == 0:
            print(header.format(path, "--", "--", "--", "--", "--", "--", "--", "--", "failed ✗"))
            continue
        iter = average([x.iter for x in successes], 2)
        eff = average([x.eff for x in successes], 2)
        can = average([x.can for x in successes], 2)
        total = average([x.total for x in successes])
        com = average([x.com for x in successes])
        fut = average([x.fut for x in successes])
        hist = average([x.hist for x in successes])
        join = average([x.join for x in successes])
        inter = average([x.inter for x in successes])

        com = str(int(round((com / (float(total))) * 100.0, 0))) + "%"
        fut = str(int(round((fut / (float(total))) * 100.0, 0))) + "%"
        hist = str(int(round((hist / (float(total))) * 100.0, 0))) + "%"
        join = str(int(round((join / (float(total))) * 100.0, 0))) + "%"
        inter = str(int(round((inter / (float(total))) * 100.0, 0))) + "%"
        total = human_readable(total) + " ✓"

        print(header.format(path, iter, eff, can, com, fut, hist, join, inter, total))


def main():
    print("Settings: iterations={0}, timeout={1}".format(REPETITIONS, human_readable(TIMEOUT*1000)))
    print("Running benchmarks...")
    print()
    for i in range(REPETITIONS):
        for path in BENCHMARKS:
            result = run_test(path, i)
            RESULTS[path] = RESULTS.get(path, []) + [result]
    finalize()


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
