cmake_minimum_required(VERSION 3.0.0)

project(reputation_effects VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_TOOLCHAIN_FILE "/home/shiwenbo/git/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" FORCE)
set(VCPKG_INSTALL_PATH "/home/shiwenbo/git/vcpkg/installed/x64-linux" CACHE STRING "Vcpkg install path" FORCE)

include(${CMAKE_TOOLCHAIN_FILE})

option(ENABLE_ASSERTS "Enable asserts" OFF)

if(NOT ENABLE_ASSERTS)
    add_definitions(-DNDEBUG)
    # set O3 optimization if not enable asserts
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()

# fast-cpp-csv-parser
###############
# include(FetchContent)
# # download and import header-only library fast cpp csv parser and put it in ./include
# FetchContent_Declare(
#   fast_cpp_csv_parser
#   GIT_REPOSITORY git@github.com:ben-strasser/fast-cpp-csv-parser.git
#   GIT_TAG master
# )
# FetchContent_MakeAvailable(fast_cpp_csv_parser)
############### deprecated, for the library not in vcpkg, use git download to ./third_party manually

# check if third_party/fast-cpp-csv-parser exists, if not, download it
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fast-cpp-csv-parser")
    message(STATUS "Downloading fast-cpp-csv-parser")
    execute_process(
        COMMAND git submodule update --init ${CMAKE_SOURCE_DIR}/third_party/fast-cpp-csv-parser)
else()
    message(STATUS "fast-cpp-csv-parser exists")
endif()

####### fast-cpp-csv-parser doesn't have CMakeLists.txt, we can't use add_subdirectory
# add_subdirectory(third_party/fast-cpp-csv-parser)
# target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE third_party/fast-cpp-csv-parser/include)
#######
include_directories(third_party/fast-cpp-csv-parser)

find_package(muparser CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
# find_package(Boost REQUIRED [COMPONENTS <libs>...])
# 寻找 boost-json 库
find_package(Boost REQUIRED COMPONENTS json)
find_package(GTest CONFIG REQUIRED)

set(VCPKG_LIBS
    muparser::muparser
    fmt::fmt
    TBB::tbb TBB::tbbmalloc
    indicators::indicators
    gflags::gflags
    Boost::boost Boost::json
)

# vcpkg include and custom include
include_directories(include)
include_directories(${VCPKG_INSTALL_PATH}/include)

# custom library
aux_source_directory(src SRC_FILES)
add_library(mylib ${SRC_FILES})

# main
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE mylib)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${VCPKG_LIBS})

# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE muparser::muparser)
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fmt::fmt)
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE TBB::tbb TBB::tbbmalloc)
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE indicators::indicators)
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gflags::gflags)
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Boost::boost Boost::json)

# test
set(TESTS MyRandomTest NormTest)

foreach(TEST ${TESTS})
    message(STATUS "Adding test: ${TEST}")
    add_executable(${TEST} tests/${TEST}.cpp)
    target_link_libraries(${TEST} PRIVATE mylib)
    target_link_libraries(${TEST} PRIVATE ${VCPKG_LIBS})
    target_link_libraries(${TEST} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
    add_test(NAME ${TEST} COMMAND ${TEST})
endforeach()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
