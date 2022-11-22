cmake_minimum_required (VERSION 3.5)

project(CTP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")

IF(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -pthread")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

add_subdirectory(Test)
add_subdirectory(Core)
