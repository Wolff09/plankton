#!/bin/bash

ITER=${1:-1}
LOGS="logs"
EXECUTABLE="./plankton"
PRINTER="python3 prettyprint.py"
BENCHMARKS=(
  "FineSet:--future"
  "LazySet:--future"
  "VechevYahavDCas:--future"
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
    cmd_old="${EXECUTABLE} --old ${flags} ${input}"
    cmd_new="${EXECUTABLE} --new ${flags} ${input}"
    # running original analysis
    touch "${output_old}"
    echo -n "    - Running --old analysis: ${name} "
    {
      echo "${name}"
      echo "old"
      echo "${cmd_old}"
      date
      echo "##################################"
      echo ""
      echo ""
      time ${cmd_old}
    } > "${output_old}" 2>&1
    if [ $? -eq 0 ]
    then echo "✓"
    else echo "✗"
    fi
    # running new analysis
    touch "${output_old}"
    echo -n "    - Running --new analysis: ${name} "
    {
      echo "${name}"
      echo "new"
      echo "${cmd_new}"
      date
      echo "##################################"
      echo ""
      echo ""
      time ${cmd_new}
    } > "${output_new}" 2>&1
    if [ $? -eq 0 ]
    then echo "✓"
    else echo "✗"
    fi
    # ${EXECUTABLE} ${input} >> "${output}"
    done
done
echo "[done]"
${PRINTER} "${LOGS}"
