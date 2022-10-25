cmake_minimum_required (VERSION 3.5)

project(MemoryAlloc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug") 

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/VarSizeAllocMngr.cpp
    src/Allocator.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
