cmake_minimum_required(VERSION 3.20)
project(OpenBolo C)

# ENet ships cmake_minimum_required(VERSION 3.1) which CMake 3.27+ rejects.
# Setting this cache var lets CMake handle ENet's old policy floor gracefully.
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)

include(FetchContent)

# ---- ENet — cross-platform UDP networking (pure C, no extra deps) ---
FetchContent_Declare(enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.17
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(enet)
# Match ENet's CRT to our /MT /MTd so the linker doesn't warn about MSVCRTD
if(MSVC)
    set_property(TARGET enet PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Suppress MSVC warnings about deprecated POSIX names (_strdup, _stricmp, etc.)
if(MSVC)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        # Prevent windows.h from silently including the old winsock.h (Winsock 1).
        # All socket code must explicitly include winsock2.h before windows.h.
        WIN32_LEAN_AND_MEAN
        # Minimum target: Windows 7 (required for Winsock2 / modern APIs)
        _WIN32_WINNT=0x0601
        WINVER=0x0601
    )
    # Use multi-threaded runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Original WinBolo 1.15 source tree (embedded in this repo under src/)
set(ORIG_SRC "${CMAKE_SOURCE_DIR}/src")

# Verify the source tree exists
if(NOT EXISTS "${ORIG_SRC}/bolo/global.h")
    message(FATAL_ERROR "Cannot find original source at: ${ORIG_SRC}/bolo/global.h")
endif()

add_subdirectory(server)
add_subdirectory(client)
