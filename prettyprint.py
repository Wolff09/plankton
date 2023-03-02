# -*- coding: utf8 -*-

import os
import signal
from subprocess import Popen, PIPE, TimeoutExpired
from statistics import mean
import re
import sys


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


def make_results(folder):
    files = os.listdir(folder)
    results = {}
    for file in files:
        log = os.path.join(folder, file)
        with open(log) as handle:
            name = handle.readline().strip()
            mode = handle.readline().strip()
            mode = 1 if mode == "new" else 0
            output = "".join(handle.readlines())
            result = extract_info(output)
            results.setdefault(name, ([], []))
            results[name][mode].append(result)
    return results


def print_results(results):
    print()
    print()
    width = max([len(x) for x in results]) + 3
    header = "{:<" + str(width) + "} | {:<8} | {:>5} | {:>5} | {:>5} | {:>5} | {:>5} | {:>5} | {:>5} | {:>5} | {:>15}"
    print(header.format("Program", "Analysis", "Iter", "Eff", "Cand", "Com", "Fut", "Hist", "Join", "Inter", "Linearizable"))
    print(("-" * width) + "-+----------+-------+-------+-------+-------+-------+-------+-------+-------+-----------------")
    for name in results:
        for index, array in enumerate(results.get(name)):
            pname = name if index == 0 else ""
            mode = "--old" if index == 0 else "--new"
            successes = [x for x in array if x.success]
            if len(successes) == 0:
                print(header.format(pname, mode, "--", "--", "--", "--", "--", "--", "--", "--", "failed ✗"))
                continue
            iters = average([x.iter for x in successes], 2)
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

            print(header.format(pname, mode, iters, eff, can, com, fut, hist, join, inter, total))


def main(folder):
    print()
    print()
    results = make_results(folder)
    print_results(results)
    print()


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Expected {} argument, got {}".format(1, len(sys.argv)-1))
        exit(-1)
    main(sys.argv[1])
