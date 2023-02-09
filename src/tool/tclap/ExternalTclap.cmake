# -*- mode:cmake -*-

set(TCLAP_LOCAL_ROOT ${CMAKE_BINARY_DIR}/locals/tclap)

CMAKE_MINIMUM_REQUIRED(VERSION 3.7)
PROJECT(tclap_fetcher CXX)
INCLUDE(ExternalProject)

if(NOT EXISTS "${TCLAP_REPO_URL}")
  message(FATAL_ERROR "Unable to find TCLAP code base. TCLAP_REPO_URL: ${TCLAP_REPO_URL}")
endif()
if(NOT DEFINED TCLAP_REPO_HASH)
  message(FATAL_ERROR "Missing MD5 hash for TCLAP code base.")
endif()

# default path for source files
if (NOT TCLAP_GENERATED_SRC_DIR)
  set(TCLAP_GENERATED_SRC_DIR ${CMAKE_BINARY_DIR}/tclap_generated_src)
endif()

# build provided TCLAP version
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
ExternalProject_ADD(
  tclap
  PREFIX             ${TCLAP_LOCAL_ROOT}
  URL                ${TCLAP_REPO_URL}
  URL_HASH           MD5=${TCLAP_REPO_HASH}
  SOURCE_DIR         ${TCLAP_LOCAL_ROOT}/tclap
  BUILD_IN_SOURCE    TRUE
  CONFIGURE_COMMAND  ""
  BUILD_COMMAND      ""
  INSTALL_COMMAND    ""
  LOG_CONFIGURE ON
  LOG_BUILD ON
  CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=RELEASE
        -DWITH_STATIC_CRT:BOOL=${ANTLR4_WITH_STATIC_CRT}
        -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
)

ExternalProject_Get_Property(tclap SOURCE_DIR)
list(APPEND TCLAP_INCLUDE_DIRS ${SOURCE_DIR}/include/)

add_library(Tclap INTERFACE)
add_dependencies(Tclap tclap)
target_include_directories(Tclap INTERFACE ${TCLAP_INCLUDE_DIRS})
