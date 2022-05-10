
set(SOURCES
        ast.cpp
        sugar.cpp
        visitors.cpp

        util/copy.cpp
        util/equal.cpp
        util/mover.cpp
        util/print.cpp
)

add_library(Programs ${SOURCES})
