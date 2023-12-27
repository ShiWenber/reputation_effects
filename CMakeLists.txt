cmake_minimum_required(VERSION 3.5.0)

project(reputation_effects VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_TOOLCHAIN_FILE "/home/shiwenbo/git/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_INSTALL_PATH "/home/shiwenbo/git/vcpkg/installed/x64-linux")

# include(${CMAKE_TOOLCHAIN_FILE})

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

find_package(muparser CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
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
# # target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

# test
set(TESTS PoolDequeTest BufferTest RewardMatrixTest NormTest)


foreach(TEST ${TESTS})
    add_executable(${TEST} tests/${TEST}.cpp)
    target_link_libraries(${TEST} PRIVATE mylib)
    # TODO: should I link all the libraries here?
    # It is not a good idea to link all the libraries for test code like the following line. This will increase the compile time
    # It is better to link the libraries that are only used in the test code, and write a separate cmake code for each test
    target_link_libraries(${TEST} PRIVATE ${VCPKG_LIBS})
    target_link_libraries(${TEST} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
    add_test(NAME ${TEST} COMMAND ${TEST})
endforeach()





set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
