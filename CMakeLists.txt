# MIT License
#
# Copyright (c) 2019 Ivan Kelarev
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.13)
project(allocator)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(THIRD_PARTY_DIR "${PROJECT_SOURCE_DIR}/libs")


# Curl
set(CMAKE_FOLDER "Curl")
set(CURL_DIR "${THIRD_PARTY_DIR}/curl")
set(CURL_INCLUDE_DIR "${CURL_DIR}/include")

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(BUILD_CURL_EXE OFF)
set(BUILD_SHARED_LIBS OFF)
set(HTTP_ONLY ON)
set(ENABLE_IPV6 OFF)
set(CURL_ZLIB OFF)
set(CURL_DISABLE_CRYPTO_AUTH ON)
if(WIN32)
  set(CMAKE_USE_WINSSL ON)
endif()
add_subdirectory(${CURL_DIR})

add_definitions(-DCURL_STATICLIB)

# Workaround
set_target_properties(show PROPERTIES FOLDER ${CMAKE_FOLDER})
set_target_properties(uninstall PROPERTIES FOLDER ${CMAKE_FOLDER})


# GLPK
set(CMAKE_FOLDER "GLPK")
file(GLOB GLPK_DIR "${THIRD_PARTY_DIR}/glpk-*")
set(GLPK_SRC_DIR "${GLPK_DIR}/src")

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

file(GLOB_RECURSE GLPK_HEADERS LIST_DIRECTORIES false "${GLPK_SRC_DIR}/*.h")
file(GLOB_RECURSE GLPK_SOURCES LIST_DIRECTORIES false "${GLPK_SRC_DIR}/*.c")
SUBDIRLIST(GLPK_INCLUDE_DIRS ${GLPK_SRC_DIR})

include_directories(${GLPK_INCLUDE_DIRS})
#add_definitions(-DHAVE_GMP) # TODO: use GMP/MPIR?
add_library(libglpk STATIC ${GLPK_HEADERS} ${GLPK_SOURCES})


# Other libs
set(INIH_INCLUDE_DIR ${THIRD_PARTY_DIR}/inih)
set(JSON_INCLUDE_DIR ${THIRD_PARTY_DIR}/json/single_include/nlohmann)
set(CATCH_INCLUDE_DIR ${THIRD_PARTY_DIR}/catch/single_include/catch2)


# Allocator
set(CMAKE_FOLDER "Allocator")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "--std=gnu++11 -Wall -g ${CMAKE_CXX_FLAGS}")
endif()

include_directories(
  ${CURL_INCLUDE_DIR}
  ${GLPK_SRC_DIR}
  ${INIH_INCLUDE_DIR}
  ${JSON_INCLUDE_DIR}
  ${CATCH_INCLUDE_DIR}
)

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(OPTIMIZER_HEADERS
  ${SRC_DIR}/allocation.h
  ${SRC_DIR}/mipsolver.h
  ${SRC_DIR}/optimizer.h
  ${INIH_INCLUDE_DIR}/ini.h
)

set(OPTIMIZER_SOURCES
  ${SRC_DIR}/allocation.cpp
  ${SRC_DIR}/mipsolver.cpp
  ${SRC_DIR}/optimizer.cpp
  ${INIH_INCLUDE_DIR}/ini.c
)

set(ALLOCATOR_HEADERS
  ${SRC_DIR}/curl.h
  ${SRC_DIR}/iextrading.h
  ${SRC_DIR}/internet_provider.h
  ${SRC_DIR}/market_info_provider.h
  ${SRC_DIR}/yahoofinance.h
  ${JSON_INCLUDE_DIR}/json.hpp
)

set(ALLOCATOR_SOURCES
  ${SRC_DIR}/curl.cpp
  ${SRC_DIR}/iextrading.cpp
  ${SRC_DIR}/yahoofinance.cpp
)

set(TESTS_HEADERS
  ${CATCH_INCLUDE_DIR}/catch.hpp
)

set(TESTS_SOURCES
  ${SRC_DIR}/tests.cpp
  ${SRC_DIR}/test_allocation.cpp
  ${SRC_DIR}/test_glpk.cpp
  ${SRC_DIR}/test_iextrading.cpp
  ${SRC_DIR}/test_mipsolver.cpp
  ${SRC_DIR}/test_optimizer.cpp
  ${SRC_DIR}/test_tableformatter.cpp
  ${SRC_DIR}/test_yahoofinance.cpp
)

add_library(liboptimizer STATIC ${OPTIMIZER_HEADERS} ${OPTIMIZER_SOURCES})
add_library(libtableformatter STATIC ${SRC_DIR}/tableformatter.h ${SRC_DIR}/tableformatter.cpp)
add_library(liballocator STATIC ${ALLOCATOR_HEADERS} ${ALLOCATOR_SOURCES})

add_executable(tests ${TESTS_HEADERS} ${TESTS_SOURCES})
add_executable(allocator ${SRC_DIR}/allocator.cpp)

set(LIBS libtableformatter liboptimizer liballocator libcurl libglpk)

target_link_libraries(tests ${LIBS})
target_link_libraries(allocator ${LIBS})
