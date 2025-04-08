cmake_minimum_required(VERSION 3.20)

# Define the project: we name it "pfs" and specify C as the language.
project(pfs C)

# Set the output directory for the build and the final executable.
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})

# Explicitly set the C compiler (adjust if needed)
set(CMAKE_C_COMPILER gcc)

# Set the build type to Release (if not already set) to enable compiler optimizations.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Set the C standard to C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure universal binary for macOS: build both arm64 and x86_64 slices.
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

# Specify the source files. Here we assume main.c is located in the root of the project.
set(SOURCE_FILES ${CMAKE_SOURCE_DIR}/main.c)

# Optionally, you can add extra compiler flags for performance in Release mode.
# For example, to force high optimization:
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fopenmp -O3")

# Create the executable target using the project name.
add_executable(${EXECUTABLE} ${SOURCE_FILES})

# Optionally, add a custom target to run the executable immediately after the build.
add_custom_target(run
        COMMAND $<TARGET_FILE:${EXECUTABLE}>
        DEPENDS ${EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
