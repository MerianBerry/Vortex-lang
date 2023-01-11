cmake_minimum_required(VERSION 3.18)

project("Vortex")

add_executable(Vcomp "src/compile.cpp")
set_target_properties(Vcomp PROPERTIES
    CXX_STANDARD_REQUIRED 17)

