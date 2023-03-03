#!/bin/bash

ITER=${1:-1}
LOGS="logs"
EXECUTABLE="build/bin/plankton"
PRINTER="python3 prettyprint.py"
BENCHMARKS=(
  "FineSet:--future"
  "LazySet:--future"
  "VechevYahavDCas:--future --loopNoPostJoin"
  "VechevYahavCas:--future"
  "ORVYY:--future"
  "FemrsTreeNoMaintenance:--future --loopNoPostJoin"
  "Michael:--future"
  "MichaelWaitFreeSearch:--future"
  "Harris:--future"
  "HarrisWaitFreeSearch:--future"
  # "examples/LO_abstract.pl:"
)


mkdir -p "${LOGS}"
rm -f -- "${LOGS}"/*


for (( iteration = 0; iteration < ITER; iteration++ )); do
  echo "# Iteration $((iteration + 1))/${ITER}"
  for (( index = 0; index < ${#BENCHMARKS[@]}; index++ )); do
    entry="${BENCHMARKS[$index]}"
    name=${entry%%:*}
    flags=${entry#*:}
    input="examples/${name}.pl"
    output_old="${LOGS}/${name}-old-${iteration}.txt"
    output_new="${LOGS}/${name}-new-${iteration}.txt"
    cmd_old="${EXECUTABLE} --old ${flags} -o ${output_old} ${input}"
    cmd_new="${EXECUTABLE} --new ${flags} -o ${output_new} ${input}"
    # old version
    echo -n "    - Running --old analysis: ${name} "
    ${cmd_old}
    if [ $? -eq 0 ]
    then echo "✓"
    else echo "✗"
    fi
    # new version
    echo -n "    - Running --new analysis: ${name} "
    ${cmd_new}
    if [ $? -eq 0 ]
    then echo "✓"
    else echo "✗"
    fi
  done
done
echo "[done]"
${PRINTER} "${LOGS}"
