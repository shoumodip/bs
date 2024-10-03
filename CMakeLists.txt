cmake_minimum_required(VERSION 3.10)
project(bs)

# Set the C standard
set(CMAKE_C_STANDARD 11)

# Specify the PCRE2 source directory
set(PCRE2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pcre2")

# Include the ExternalProject module
include(ExternalProject)

# Build PCRE2 as an external project
ExternalProject_Add(
    PCRE2
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/pcre2
    SOURCE_DIR ${PCRE2_SOURCE_DIR}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/pcre2-install -DCMAKE_BUILD_TYPE=Release
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --config Release --target install
)

# Include directories for your project
include_directories(include)

# Find all source files for the project
file(GLOB BS_SRC_FILES src/bs/*.c)

# Main executable
add_executable(bs src/bs.c ${BS_SRC_FILES})

# Specify the target properties for the executable
set_target_properties(bs PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3"
)

# Link the executable against the PCRE2 library
add_dependencies(bs PCRE2) # Ensure PCRE2 is built before this target

# Include the PCRE2 headers and link against the static library
target_include_directories(bs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/include)

# Optionally, if you want to create the static library
add_library(libbs STATIC ${BS_SRC_FILES})
set_target_properties(libbs PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
    OUTPUT_NAME "bs"  # Set the output name to libbs.a
)

# Link the static library against the PCRE2 library
target_include_directories(libbs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/include)

# Create a shared library
add_library(libbs_shared SHARED ${BS_SRC_FILES})
set_target_properties(libbs_shared PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
    OUTPUT_NAME "bs"  # Set the output name to libbs.a
)

# Link the shared library against the PCRE2 library
target_include_directories(libbs_shared PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/include)

if(WIN32)
    target_link_libraries(bs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/lib/pcre2-8-static.lib)
    target_link_libraries(libbs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/lib/pcre2-8-static.lib)
    target_link_libraries(libbs_shared PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/lib/pcre2-8-static.lib)
else()
    target_link_libraries(bs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/lib/libpcre2-8.a m)
    target_link_libraries(libbs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/lib/libpcre2-8.a)
    target_link_libraries(libbs_shared PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pcre2-install/lib/libpcre2-8.a)
endif()

# Set the output directories for executables and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Apply the output directories for different build configurations (Release, Debug, etc.)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach()
