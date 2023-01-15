cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_FLAGS "--static")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")
project("Vortex")

#add_executable(Vcomp "src/compile.cpp")
#set_target_properties(Vcomp PROPERTIES
#    CXX_STANDARD_REQUIRED 17)

add_executable(Vtest "src/compiler.cpp")
set_target_properties(Vtest PROPERTIES
    CXX_STANDARD 17)
