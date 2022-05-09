
################################
######## setting up z3 #########
################################

# TODO: check version
find_library(Z3_LIBRARY NAMES z3 REQUIRED)
find_path(Z3_INCLUDE NAMES "z3++.h" REQUIRED)
include_directories(${Z3_INCLUDE})


################################
####### setting up build #######
################################

set(SOURCES
        encoding/encoding.cpp
        encoding/encode.cpp
        encoding/graph.cpp
        encoding/solve.cpp
        encoding/spec.cpp

        flowgraph/flowgraph.cpp
        flowgraph/make.cpp

        solver/access.cpp
        solver/ful.cpp
        solver/future.cpp
        solver/implication.cpp
        solver/interference.cpp
        solver/join.cpp
        solver/locks.cpp
        solver/past.cpp
        solver/post_assign_var.cpp
        solver/post_assign_write.cpp
        solver/post_assume.cpp
        solver/post_malloc.cpp
        solver/scope.cpp
        solver/solver.cpp
        solver/stability.cpp
        solver/unsat.cpp
        solver/widen.cpp

        prover/cmd.cpp
        prover/default.cpp
        prover/dispatch.cpp
        prover/macro.cpp
        prover/prog.cpp
        prover/prover.cpp
        prover/stmt.cpp

        util/collect.cpp
        util/effects.cpp
        util/eval.cpp
        util/memory.cpp
        util/reachability.cpp
        util/spec.cpp
        util/stack.cpp
        util/symbolic.cpp

        static.cpp
)

find_package (Threads)
add_library(Engine ${SOURCES})
target_link_libraries(Engine Programs Logics ${Z3_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
