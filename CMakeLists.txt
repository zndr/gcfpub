cmake_minimum_required(VERSION 3.16)
project(mwcf_extractor VERSION 1.3.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    # Unicode support
    add_definitions(-DUNICODE -D_UNICODE)

    # Static linking for portability (no runtime DLL dependencies)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Minimum Windows version (Windows 10)
    add_definitions(-D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/cf_parser.cpp
    src/clipboard.cpp
    src/window_finder.cpp
    src/hotkey_manager.cpp
    src/tray_icon.cpp
    src/dialogs.cpp
    src/overlay.cpp
    src/config.cpp
    src/app_icon.cpp
    src/task_scheduler.cpp
    src/install_mode.cpp
    src/update_checker.cpp
)

# Header files
set(HEADERS
    src/resource.h
    src/cf_parser.h
    src/clipboard.h
    src/window_finder.h
    src/hotkey_manager.h
    src/tray_icon.h
    src/dialogs.h
    src/overlay.h
    src/config.h
    src/app_icon.h
    src/task_scheduler.h
    src/install_mode.h
    src/update_checker.h
)

# Resource file
set(RESOURCES
    res/resource.rc
)

# Create executable
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    user32
    gdi32
    shell32
    comctl32
    psapi
    dwmapi
    shcore
    shlwapi
    advapi32
    ole32
    oleaut32
    taskschd
    winhttp
)

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4          # Warning level 4
        /WX-         # Warnings not as errors (for development)
        /permissive- # Strict conformance
    )

    # Release optimizations
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
else()
    # MinGW/GCC options
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -static
        -static-libgcc
        -static-libstdc++
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        -static
        -mwindows
    )
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# Copy output to project root for easy access (Release only)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMENT "Copying executable to project root"
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
)

# CPack configuration for creating distributable package
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "MWCFExtractor")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}")
include(CPack)
