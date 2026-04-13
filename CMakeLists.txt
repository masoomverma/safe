cmake_minimum_required(VERSION 3.20)
project(Safe VERSION 1.0.0 LANGUAGES C CXX)

# ── Language standard ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json so clangd can find include paths
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Source files ─────────────────────────────────────────────────────────────
set(IMGUI_SOURCES
    core/libs/imgui/imgui.cpp
    core/libs/imgui/imgui_demo.cpp
    core/libs/imgui/imgui_draw.cpp
    core/libs/imgui/imgui_tables.cpp
    core/libs/imgui/imgui_widgets.cpp
    core/libs/imgui/backends/imgui_impl_dx11.cpp
    core/libs/imgui/backends/imgui_impl_win32.cpp
)

set(PROJECT_SOURCES
    core/src/main.cpp
    core/src/core/cli.cpp
    core/src/core/filesystem.cpp
    core/src/core/folder.cpp
    core/src/core/renderer.cpp
    core/src/core/types.cpp
    core/src/ui/ui.cpp
)

set(SQLITE_SOURCES
    core/libs/sqlite3/sqlite3.c
)

# ── Target ───────────────────────────────────────────────────────────────────
add_executable(Safe WIN32
    ${PROJECT_SOURCES}
    ${IMGUI_SOURCES}
    ${SQLITE_SOURCES}

    app.rc
)

# Mark SQLite as C source (not C++)
set_source_files_properties(${SQLITE_SOURCES} PROPERTIES LANGUAGE C)

# ── Include paths ────────────────────────────────────────────────────────────
# CMAKE_SOURCE_DIR (workspace root) is added so that
# #include "core/..." and #include "ui/..." style paths resolve correctly.
target_include_directories(Safe PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/libs/imgui
    ${CMAKE_SOURCE_DIR}/core/libs/imgui/backends
    ${CMAKE_SOURCE_DIR}/core/libs/sqlite3
)

# ── Windows / DirectX 11 libraries ───────────────────────────────────────────
target_link_libraries(Safe PRIVATE
    d3d11
    d3dcompiler
    dxgi
    dwmapi
    shell32
    bcrypt
)

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(Safe PRIVATE /W4 /MP)
    # Suppress warnings inside third-party ImGui headers
    target_compile_options(Safe PRIVATE /external:I${CMAKE_SOURCE_DIR}/libs /external:W0)
else()
    target_compile_options(Safe PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Ensure runtime font path exists next to Safe.exe.
add_custom_command(TARGET Safe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Safe>/assets/fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/assets/fonts/Inter_24pt-Regular.ttf"
        "$<TARGET_FILE_DIR:Safe>/assets/fonts/Inter-Regular.ttf"
    COMMENT "Copying Inter font to output assets/fonts"
)

# Unicode / Win32 subsystem defines
target_compile_definitions(Safe PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

find_program(ISCC_EXECUTABLE NAMES ISCC.exe iscc)
if(ISCC_EXECUTABLE)
    add_custom_target(installer
        COMMAND "${ISCC_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/installer/inno/SafeInstaller.iss"
            "/DAppVersion=${PROJECT_VERSION}"
            "/DSourceDir=$<TARGET_FILE_DIR:Safe>"
            "/DOutputDir=${CMAKE_BINARY_DIR}/installer"
        DEPENDS Safe
        COMMENT "Building per-user safe.exe installer with Inno Setup"
        VERBATIM
    )
endif()

# OpenSSL DLLs
set(OPENSSL_DLL_DIR "${CMAKE_SOURCE_DIR}/build" CACHE PATH "Directory containing OpenSSL runtime DLLs")
if(EXISTS "${OPENSSL_DLL_DIR}")
    file(GLOB OPENSSL_DLLS
        "${OPENSSL_DLL_DIR}/libcrypto*.dll"
        "${OPENSSL_DLL_DIR}/libssl*.dll"
    )

    foreach(DLL ${OPENSSL_DLLS})
        add_custom_command(TARGET Safe POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "$<TARGET_FILE_DIR:Safe>"
            COMMENT "Copying ${DLL} to output directory"
        )
    endforeach()
endif()
