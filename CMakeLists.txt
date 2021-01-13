# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2020-2021, Antonio Niño Díaz
#
# GiiBiiAdvance - GBA/GB emulator

cmake_minimum_required(VERSION 3.13)
project(GiiBiiAdvance)
enable_language(CXX C)

set(CXX_STANDARD 14)
set(CXX_EXTENSIONS OFF)
set(CXX_STANDARD_REQUIRED ON)

set(C_STANDARD 11)
set(C_EXTENSIONS OFF)
set(C_STANDARD_REQUIRED ON)

add_executable(giibiiadvance)

# Compiler-specific options
# -------------------------

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(giibiiadvance PRIVATE
        # Force all integers to be 2's complement to prevent the compiler from
        # doing optimizations because of undefined behaviour.
        -fwrapv

        # Force usage of extern for external variables
        -fno-common

        # Enable most common warnings
        -Wall -Wextra

        # Disable this warning, which is enabled by default
        -Wformat-truncation=0
    )
    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 9.3)
        target_compile_options(giibiiadvance PRIVATE
            # Enable a bunch of warnings that aren't enabled with Wall or Wextra
            -Wformat-overflow=2 -Wformat=2 -Wno-format-nonliteral
            -Wundef -Wunused -Wuninitialized -Wunknown-pragmas -Wshadow
            -Wlogical-op -Wduplicated-cond -Wswitch-enum -Wfloat-equal
            -Wcast-align -Walloc-zero -Winline
            -Wstrict-overflow=5 -Wstringop-overflow=4
            $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>
            $<$<COMPILE_LANGUAGE:C>:-Wold-style-definition>

            # Enable Wpedantic but disable warning about having strings that are
            # too long
            -Wpedantic -Wno-overlength-strings

            # Make sure we don't use too much stack. Windows doesn't like it
            # when the stack usage is too high, even when Linux doesn't complain
            # about it.
            -Wstack-usage=4096

            # TODO: Enable the following warnings?
            #-Wformat-truncation=1 -Wcast-qual -Wconversion
        )

        # Build option to enable Undefined Behaviour Sanitizer (UBSan)
        # --------------------------------------------------------
        #
        # This should only be enabled in debug builds. It makes the code far
        # slower, so it should only be used during development.
        option(ENABLE_UBSAN "Compile with UBSan support (GCC)" OFF)

        if(ENABLE_UBSAN)
            target_compile_options(giibiiadvance PRIVATE -fsanitize=undefined)
            target_link_options(giibiiadvance PRIVATE -fsanitize=undefined)
        endif()
    endif()
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(giibiiadvance PRIVATE
        # Silence warnings
        -D_USE_MATH_DEFINES
        -D_CRT_SECURE_NO_WARNINGS
    )
    target_compile_options(giibiiadvance PRIVATE
        # Enable parallel compilation
        /MP
    )
endif()

# Add source code files
# ---------------------

# Macro that searches all the source files in the specified directory in 'dir'
# and saves them in 'var'
macro(search_source_files dir var)
    file(GLOB ${var} CONFIGURE_DEPENDS ${dir}/*.c ${dir}/*.cpp ${dir}/*.h)
endmacro()

search_source_files(source FILES_SOURCE)
search_source_files(source/gb_core FILES_SOURCE_GB_CORE)
search_source_files(source/gba_core FILES_SOURCE_GBA_CORE)
search_source_files(source/gui FILES_SOURCE_GUI)

target_sources(giibiiadvance PRIVATE
    ${FILES_SOURCE}
    ${FILES_SOURCE_GB_CORE}
    ${FILES_SOURCE_GBA_CORE}
    ${FILES_SOURCE_GUI}
)

# Windows resources

if(WIN32)
    target_sources(giibiiadvance PRIVATE
        windows_resources/resource.rc
    )
endif()

# Link with libraries and check build options
# -------------------------------------------

# libpng and SLD2 are required

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    find_package(libpng REQUIRED 1.6)
    find_package(SDL2 REQUIRED 2.0.7)

    target_link_libraries(giibiiadvance PRIVATE
        png
        SDL2::SDL2 SDL2::SDL2main
    )
else()
    find_package(PNG REQUIRED 1.6)
    find_package(SDL2 REQUIRED 2.0.7)

    target_include_directories(giibiiadvance PRIVATE
        ${PNG_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
    )
    target_link_libraries(giibiiadvance PRIVATE
        ${PNG_LIBRARIES}
        ${SDL2_LIBRARIES}
    )
endif()

# Add Lua as a required library temporarily

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    include(FindLua)
    find_package(lua REQUIRED 5.2)
else()
    find_package(Lua REQUIRED 5.2)
endif()

if(Lua_FOUND)
    option(ENABLE_LUA "Enable Lua scripting support" ON)
else()
    set(ENABLE_LUA OFF)
endif()

if(ENABLE_LUA)
    target_link_libraries(giibiiadvance PRIVATE ${LUA_LIBRARIES})
    target_include_directories(giibiiadvance PRIVATE ${LUA_INCLUDE_DIR})
    target_compile_definitions(giibiiadvance PRIVATE ENABLE_LUA)
endif()

# OpenCV is optional. If found, let the user build with GB Camera emulation.

find_package(OpenCV 4)

if(OpenCV_FOUND)
    option(ENABLE_CAMERA "Enable Game Boy Camera emulation" ON)
else()
    set(ENABLE_CAMERA OFF)
endif()

if(ENABLE_CAMERA)
    if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_link_libraries(giibiiadvance PRIVATE opencv_videoio)
    else()
        target_include_directories(giibiiadvance PRIVATE ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(giibiiadvance PRIVATE ${OpenCV_LIBRARIES})
    endif()
else()
    target_compile_definitions(giibiiadvance PRIVATE -DNO_CAMERA_EMULATION)
endif()

# OpenGL is optional. It can be used as library to output graphics.

find_package(OpenGL)

if(OPENGL_FOUND)
    option(ENABLE_OPENGL "Compile with OpenGL" ON)
else()
    set(ENABLE_OPENGL OFF)
endif()

if(ENABLE_OPENGL)
    target_compile_definitions(giibiiadvance PRIVATE -DENABLE_OPENGL)
    target_include_directories(giibiiadvance PRIVATE ${OPENGL_INCLUDE_DIRS})
    target_link_libraries(giibiiadvance PRIVATE ${OPENGL_LIBRARIES})
endif()

# In x86 CPUs, replace part of the CPU interpreter by inline assembly.

if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # This isn't compatible with MSVC
    option(ENABLE_ASM_X86 "Compile with inline assembly" OFF)
else()
    set(ENABLE_ASM_X86 OFF)
endif()

if(ENABLE_ASM_X86)
    target_compile_definitions(giibiiadvance PRIVATE -DENABLE_ASM_X86)
endif()
