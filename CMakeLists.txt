cmake_minimum_required(VERSION 3.18)

function(post_build_copy target from dest)
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${from}
            ${dest}
            COMMENT "-- Post build copy of ${from} -> ${dest} --"
    )
endfunction(post_build_copy)

#set(CMAKE_CXX_FLAGS "--static")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")
project("Vortex")

#add_executable(Vcomp "src/compile.cpp")
#set_target_properties(Vcomp PROPERTIES
#    CXX_STANDARD_REQUIRED 17)

add_executable(Vtest "src/compiler.cpp")
set_target_properties(Vtest PROPERTIES
    CXX_STANDARD 17)

#set(CMAKE_FIND_DEBUG_MODE ON)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "LLVM includes: ${LLVM_INCLUDE_DIRS}")
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm_libs core)

add_executable(Vllvm "src/llvmcompiler.cpp")
set_target_properties(Vllvm PROPERTIES
    CXX_STANDARD 17
    LINK_FLAGS "`llvm-config --cxxflags --ldflags --libs`"
)
message(STATUS "Libs: ${llvm_libs}")
target_link_libraries(Vllvm PRIVATE ${llvm_libs})

post_build_copy(Vllvm "${CMAKE_SOURCE_DIR}/src/example.vtex" "${CMAKE_BINARY_DIR}/example.vtex")
    #LINK_FLAGS "`llvm-config -cxxflags --ldflags --system-libs --libs core`")