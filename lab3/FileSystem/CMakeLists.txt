cmake_minimum_required (VERSION 3.5)

project(Ext2FileSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_EXPORT_COMPILECOMMANDS ON)

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/Ext2.cpp
    src/Shell.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)