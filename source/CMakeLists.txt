cmake_minimum_required(VERSION 3.10)

project(dsp56kemu VERSION 0.0.1)

include("base.cmake")

set(ASMJIT_STATIC TRUE)

add_subdirectory(dsp56kEmu)
add_subdirectory(asmjit)

# ----------------- command-line app
