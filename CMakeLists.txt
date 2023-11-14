cmake_minimum_required(VERSION 3.5.0)

project(reputation_effects VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_TOOLCHAIN_FILE "/home/shiwenbo/git/vcpkg/scripts/buildsystems/vcpkg.cmake")

include(${CMAKE_TOOLCHAIN_FILE})
include(CTest)
enable_testing()

find_package(muparser CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

include_directories(include)

aux_source_directory(src SRC_FILES)

add_executable(${CMAKE_PROJECT_NAME} main.cpp ${SRC_FILES})
add_executable(PoolDequeTest tests/PoolDequeTest.cpp ${SRC_FILES})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE muparser::muparser)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fmt::fmt)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE TBB::tbb TBB::tbbmalloc)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE indicators::indicators)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

# add_test(AllTestsInMain ${CMAKE_PROJECT_NAME})
add_test(NAME PoolDequeTest COMMAND PoolDequeTest)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
