cmake_minimum_required(VERSION 3.20)
project(Safe LANGUAGES C CXX)

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
    core/src/core/renderer_impl.cpp
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
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/imgui
    ${CMAKE_SOURCE_DIR}/libs/imgui/backends
    ${CMAKE_SOURCE_DIR}/libs/sqlite3
)

# ── Windows / DirectX 11 libraries ───────────────────────────────────────────
target_link_libraries(Safe PRIVATE
    d3d11
    d3dcompiler
    dxgi
    dwmapi
)

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(Safe PRIVATE /W4 /MP)
    # Suppress warnings inside third-party ImGui headers
    target_compile_options(Safe PRIVATE /external:I${CMAKE_SOURCE_DIR}/libs /external:W0)
else()
    target_compile_options(Safe PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Unicode / Win32 subsystem defines
target_compile_definitions(Safe PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# ── Post-build: copy OpenSSL DLLs next to the executable ─────────────────────
# Uncomment when OpenSSL is integrated into the project
# set(OPENSSL_DLLS
#     ${CMAKE_SOURCE_DIR}/build/libcrypto-3-x64.dll
#     ${CMAKE_SOURCE_DIR}/build/libssl-3-x64.dll
# )
#
# foreach(DLL ${OPENSSL_DLLS})
#     add_custom_command(TARGET Safe POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${DLL}"
#             "$<TARGET_FILE_DIR:Safe>"
#         COMMENT "Copying ${DLL} to output directory"
#     )
# endforeach()
