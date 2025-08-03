cmake_minimum_required(VERSION 3.16)
project(RTEgetData)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    Code/RTEgetData.cpp
    Code/bridge.cpp
    Code/cmd_line.cpp
    Code/com_lib.cpp
    Code/gdb_lib.cpp
    Code/logger.cpp
    Code/platform_compat.cpp
)

# Header files
set(HEADERS
    Code/RTEgetData.h
    Code/bridge.h
    Code/cmd_line.h
    Code/com_lib.h
    Code/gdb_defs.h
    Code/gdb_lib.h
    Code/logger.h
    Code/pch.h
    Code/rtedbg.h
    Code/rte_com.h
    Code/platform_compat.h
)

# Create executable
add_executable(RTEgetData ${SOURCES} ${HEADERS})

# Include directory
target_include_directories(RTEgetData PRIVATE Code)

# Platform-specific configuration
if(WIN32)
    # Windows-specific libraries
    target_link_libraries(RTEgetData ws2_32)
    target_compile_definitions(RTEgetData PRIVATE 
        _CRT_NONSTDC_NO_WARNINGS
        _WIN32_WINNT=0x0601
    )
else()
    # Linux/Unix-specific libraries
    find_package(Threads REQUIRED)
    target_link_libraries(RTEgetData Threads::Threads)
    
    # Add compiler flags for Linux
    target_compile_options(RTEgetData PRIVATE -Wall -Wextra)
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(RTEgetData PRIVATE /W3)
else()
    target_compile_options(RTEgetData PRIVATE -O2)
endif()