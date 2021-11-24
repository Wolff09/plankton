# -*- coding: utf8 -*-

import os
import signal
from subprocess import Popen, PIPE, TimeoutExpired
import re

TIMEOUT = 60 * 60 * 6  # in seconds
REPETITIONS = 5

EXECUTABLE = "./build/bin/plankton"
BENCHMARKS = [
    "examples/VechevYahavDCas.pl",
    "examples/VechevYahavCas.pl",
    "examples/ORVYY.pl",
    "examples/Michael.pl",
    "examples/Harris.pl",
]

REGEX = r"@gist\[(?P<path>.*?)\]=(?P<result>[01]),(?P<time>[0-9]*);(.*)"

RESULT = {}


def extract_info(output):
    if output == "__to__":
        return False, "timeout"
    if output == "__fail__":
        return False, "error"

    m = re.search(REGEX, output)
    if not m:
        return False, "error"
    if m.group("result") != "1":
        return False, "failed"
    return True, m.group("time")


def run_with_timeout(path):
    all_args = [EXECUTABLE, "-g", path]

    # make sure to properly kill subprocesses after timeout
    # see: https://stackoverflow.com/questions/36952245/subprocess-timeout-failure
    with Popen(all_args, stderr=PIPE, stdout=PIPE, preexec_fn=os.setsid, universal_newlines=True) as process:
        try:
            output = process.communicate(timeout=TIMEOUT)[0]
            if process.returncode != 0:
                output = "__fail__"
        except TimeoutExpired:
            os.killpg(process.pid, signal.SIGINT)
            output = "__to__"
    return output


def run_test(path, i):
    output = run_with_timeout(path)
    success, info = extract_info(output)
    time = None
    if success:
        time = int(info)
        info += "ms"
    print("[{:0>2}/{:0>2}] {:>15}  for  {:<20}".format(i+1, REPETITIONS, info, path), flush=True)
    return time


def finalize():
    print()
    for path in BENCHMARKS:
        times = [x for x in RESULT.get(path, []) if x]
        avg = int(sum(times) / len(times)) if len(times) > 0 else -1
        avg = str(avg) + "ms" if avg >= 0 else "--"
        print("==avg== {:>15}  for  {:<20}".format(avg, path), flush=True)


def main():
    print("Settings: iterations={0}, timeout={1}ms".format(REPETITIONS, TIMEOUT*1000))
    print("Running benchmarks...")
    print()
    for i in range(REPETITIONS):
        for path in BENCHMARKS:
            time = run_test(path, i)
            RESULT[path] = RESULT.get(path, []) + [time]
    finalize()


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("", flush=True)
        print("", flush=True)
        print("[interrupted]", flush=True)
        finalize()
