cmake_minimum_required(VERSION 3.16)
project(RTEgetData)

set(CMAKE_CXX_STANDARD 17)  # Updated for ImGui
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GUI build options
option(BUILD_GUI "Build GUI version with Dear ImGui" ON)
option(GUI_USE_SDL2 "Use SDL2 backend instead of GLFW for GUI" ON)
option(GUI_STATIC_LINK "Statically link GUI dependencies" ON)

# Common source files
set(CORE_SOURCES
    Code/bridge.cpp
    Code/cmd_line.cpp
    Code/com_lib.cpp
    Code/gdb_lib.cpp
    Code/logger.cpp
    Code/platform_compat.cpp
)

# Common header files
set(CORE_HEADERS
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

# CLI version (always built)
add_executable(RTEgetData ${CORE_SOURCES} ${CORE_HEADERS} Code/RTEgetData.cpp)
target_include_directories(RTEgetData PRIVATE Code)

# GUI version (conditional)
if(BUILD_GUI)
    include(FetchContent)
    
    # Download and set up ImGui
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.9
    )
    FetchContent_MakeAvailable(imgui)
    
    # Common ImGui sources
    set(IMGUI_CORE_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )
    
    set(GUI_BACKEND_FOUND FALSE)
    
    # Try SDL2 first if requested
    if(GUI_USE_SDL2)
        if(GUI_STATIC_LINK)
            # Fetch SDL2 for static linking
            FetchContent_Declare(
                SDL2
                GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
                GIT_TAG release-2.28.5
            )
            set(SDL_SHARED OFF CACHE BOOL "" FORCE)
            set(SDL_STATIC ON CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(SDL2)
            
            set(GUI_BACKEND_SOURCES
                ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
            )
            set(GUI_BACKEND_LIBRARIES SDL2::SDL2-static SDL2::SDL2main)
            set(GUI_BACKEND_INCLUDES ${imgui_SOURCE_DIR}/backends)
            set(GUI_BACKEND_DEFINITIONS GUI_USE_SDL2)
            set(GUI_BACKEND_FOUND TRUE)
            message(STATUS "GUI build enabled with SDL2 (static)")
        else()
            # Try to find system SDL2
            find_package(SDL2)
            if(SDL2_FOUND)
                set(GUI_BACKEND_SOURCES
                    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
                    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
                )
                set(GUI_BACKEND_LIBRARIES ${SDL2_LIBRARIES})
                set(GUI_BACKEND_INCLUDES ${imgui_SOURCE_DIR}/backends ${SDL2_INCLUDE_DIRS})
                set(GUI_BACKEND_DEFINITIONS GUI_USE_SDL2)
                set(GUI_BACKEND_FOUND TRUE)
                message(STATUS "GUI build enabled with SDL2 (system)")
            endif()
        endif()
    endif()
    
    # Fallback to GLFW if SDL2 not available or not requested
    if(NOT GUI_BACKEND_FOUND)
        if(GUI_STATIC_LINK)
            # Fetch GLFW for static linking
            FetchContent_Declare(
                glfw
                GIT_REPOSITORY https://github.com/glfw/glfw.git
                GIT_TAG 3.3.8
            )
            set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(glfw)
            
            set(GUI_BACKEND_SOURCES
                ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
            )
            set(GUI_BACKEND_LIBRARIES glfw)
            set(GUI_BACKEND_INCLUDES ${imgui_SOURCE_DIR}/backends)
            set(GUI_BACKEND_DEFINITIONS GUI_USE_GLFW)
            set(GUI_BACKEND_FOUND TRUE)
            message(STATUS "GUI build enabled with GLFW (static)")
        else()
            # Try to find system GLFW
            find_package(PkgConfig)
            if(PkgConfig_FOUND)
                pkg_check_modules(GLFW glfw3)
            endif()
            
            if(GLFW_FOUND)
                set(GUI_BACKEND_SOURCES
                    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
                    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
                )
                set(GUI_BACKEND_LIBRARIES ${GLFW_LIBRARIES})
                set(GUI_BACKEND_INCLUDES ${imgui_SOURCE_DIR}/backends ${GLFW_INCLUDE_DIRS})
                set(GUI_BACKEND_DEFINITIONS GUI_USE_GLFW)
                set(GUI_BACKEND_FOUND TRUE)
                message(STATUS "GUI build enabled with GLFW (system)")
            endif()
        endif()
    endif()
    
    if(GUI_BACKEND_FOUND)
        # Create GUI executable
        add_executable(RTEgetData-GUI 
            ${CORE_SOURCES} 
            ${CORE_HEADERS}
            ${IMGUI_CORE_SOURCES}
            ${GUI_BACKEND_SOURCES}
            Code/RTEgetData_GUI.cpp
            Code/RTEgetData_GUI.h
            Code/RTEgetData_GUI_main.cpp
        )
        
        # Include directories
        target_include_directories(RTEgetData-GUI PRIVATE 
            Code
            ${imgui_SOURCE_DIR}
            ${GUI_BACKEND_INCLUDES}
        )
        
        # Link libraries
        target_link_libraries(RTEgetData-GUI ${GUI_BACKEND_LIBRARIES})
        
        # Platform-specific OpenGL linking
        if(WIN32)
            target_link_libraries(RTEgetData-GUI opengl32)
        else()
            target_link_libraries(RTEgetData-GUI GL dl)
        endif()
        
        # Compiler definitions
        target_compile_definitions(RTEgetData-GUI PRIVATE 
            BUILD_GUI_VERSION 
            ${GUI_BACKEND_DEFINITIONS}
        )
        
        # Windows-specific settings for static linking
        if(WIN32 AND GUI_STATIC_LINK)
            if(MINGW)
                set_target_properties(RTEgetData-GUI PROPERTIES
                    LINK_FLAGS "-static -static-libgcc -static-libstdc++ -Wl,--subsystem,windows"
                )
                # Additional MinGW-specific libraries
                target_link_libraries(RTEgetData-GUI gdi32 winmm imm32 ole32 oleaut32 version uuid setupapi)
            else()
                set_target_properties(RTEgetData-GUI PROPERTIES
                    LINK_FLAGS "-static -static-libgcc -static-libstdc++"
                )
            endif()
        endif()
        
    else()
        message(WARNING "No GUI backend found. GUI build disabled. Try: -DGUI_USE_SDL2=ON -DGUI_STATIC_LINK=ON")
        set(BUILD_GUI OFF)
    endif()
endif()

# Platform-specific configuration function
function(configure_platform_specific target)
    if(WIN32)
        # Windows-specific libraries
        target_link_libraries(${target} ws2_32)
        target_compile_definitions(${target} PRIVATE 
            _CRT_NONSTDC_NO_WARNINGS
            _WIN32_WINNT=0x0601
        )
    else()
        # Linux/Unix-specific libraries
        find_package(Threads REQUIRED)
        target_link_libraries(${target} Threads::Threads)
        
        # Add compiler flags for Linux
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()

    # Compiler-specific settings
    if(MSVC)
        target_compile_options(${target} PRIVATE /W3)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    elseif(MINGW)
        target_compile_options(${target} PRIVATE -Wall -Wextra -O2)
        target_compile_definitions(${target} PRIVATE 
            _WIN32_WINNT=0x0601 
            WINVER=0x0601
            UNICODE 
            _UNICODE
        )
    else()
        target_compile_options(${target} PRIVATE -O2)
    endif()
endfunction()

# Apply platform configuration to CLI version
configure_platform_specific(RTEgetData)

# Apply platform configuration to GUI version if built
if(BUILD_GUI)
    configure_platform_specific(RTEgetData-GUI)
endif()