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

exec_benchmark() {
  name=$1
  mode=$2
  output=$3
  cmd=$4
  touch "${output}"
  {
    echo "${name}"
    echo "${mode}"
    echo "${cmd}"
    date
    echo "##################################"
    echo ""
    echo ""
    time ${cmd}
  } > "${output}" 2>&1
  return $?
}

run_benchmark() {
  name=$1
  echo -n "    - Running --old analysis: ${name} "
  exec_benchmark "$1" "$2" "$3" "$4"
  if [ $? -eq 0 ]
  then echo "✓"
  else
    exec_benchmark "$1" "$2" "$3" "$4"
    if [ $? -eq 0 ]
    then echo "✓"
    else echo "✗"
    fi
  fi
}

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
    run_benchmark "${name}" "old" "${output_old}" "${cmd_old}"
    run_benchmark "${name}" "new" "${output_new}" "${cmd_new}"
  done
done
echo "[done]"
${PRINTER} "${LOGS}"
