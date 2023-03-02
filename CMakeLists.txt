project(plankton CXX)
set(TOOL_NAME plankton)

# built requirements
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set default release type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# force debug mode
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_VERBOSE_MAKEFILE ON)

# be verbose in debug mode
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_DEBUG_PRINTING")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_TIMER")

# configure compiler
add_definitions(-fno-strict-aliasing)
add_definitions(-Wall -Wextra)
add_definitions(-fshow-column)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# installation folder
if (NOT INSTALL_FOLDER)
	set(INSTALL_FOLDER plankton)
endif()

# includes
include_directories(include)

# sources/executables
add_subdirectory(src)

# installation
install(PROGRAMS benchmark.sh DESTINATION ${INSTALL_FOLDER})
install(PROGRAMS reduced.sh DESTINATION ${INSTALL_FOLDER})
install(FILES prettyprint.py DESTINATION ${INSTALL_FOLDER})
install(DIRECTORY examples DESTINATION ${INSTALL_FOLDER})
