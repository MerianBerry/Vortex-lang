cmake_minimum_required(VERSION 3.18)

#set(CMAKE_CXX_FLAGS "--static")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")
project("Vortex")

#add_executable(Vcomp "src/compile.cpp")
#set_target_properties(Vcomp PROPERTIES
#    CXX_STANDARD_REQUIRED 17)

add_executable(Vtest "src/compiler.cpp")
set_target_properties(Vtest PROPERTIES
    CXX_STANDARD 17)


find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm_libs core)

add_executable(Vllvm "src/llvmcompiler.cpp")
set_target_properties(Vllvm PROPERTIES
    CXX_STANDARD 17
)
message(STATUS "Libs: ${llvm_libs}")
target_link_libraries(Vllvm ${llvm_libs})
    #LINK_FLAGS "`llvm-config -cxxflags --ldflags --system-libs --libs core`")