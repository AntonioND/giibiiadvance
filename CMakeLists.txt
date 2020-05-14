# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2020, Antonio Niño Díaz
#
# GiiBiiAdvance - GBA/GB emulator

cmake_minimum_required(VERSION 3.10)
project(GiiBiiAdvance)
enable_language(CXX C)

set(CXX_STANDARD 14)
set(CXX_EXTENSIONS ON)
set(CXX_STANDARD_REQUIRED ON)

set(C_STANDARD 11)
set(C_EXTENSIONS ON)
set(C_STANDARD_REQUIRED ON)

add_executable(giibiiadvance)

# This isn't meant to be generic, it's only here to help during development.
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(giibiiadvance
        PRIVATE
            -Wall
            -Wformat-truncation=0
            -Wextra
            -Wno-sign-compare
    )
endif()

target_sources(giibiiadvance
    PRIVATE
        source/config.c
        source/debug_utils.c
        source/file_explorer.c
        source/file_utils.c
        source/font_data.c
        source/font_utils.c
        source/general_utils.c
        source/input_utils.c
        source/main.c
        source/png_utils.c
        source/sound_utils.c
        source/text_data.c
        source/window_handler.c
        source/window_icon_data.c
        source/webcam_utils.cpp
        source/gb_core/camera.c
        source/gb_core/cpu.c
        source/gb_core/daa_table.c
        source/gb_core/debug.c
        source/gb_core/debug_video.c
        source/gb_core/dma.c
        source/gb_core/gb_main.c
        source/gb_core/general.c
        source/gb_core/interrupts.c
        source/gb_core/licensees.c
        source/gb_core/mbc.c
        source/gb_core/memory.c
        source/gb_core/memory_dmg.c
        source/gb_core/memory_gbc.c
        source/gb_core/noise.c
        source/gb_core/ppu.c
        source/gb_core/ppu_dmg.c
        source/gb_core/ppu_gbc.c
        source/gb_core/rom.c
        source/gb_core/serial.c
        source/gb_core/sgb.c
        source/gb_core/sound.c
        source/gb_core/video.c
        source/gba_core/arm.c
        source/gba_core/bios.c
        source/gba_core/cpu.c
        source/gba_core/disassembler.c
        source/gba_core/dma.c
        source/gba_core/gba.c
        source/gba_core/gba_debug_video.c
        source/gba_core/interrupts.c
        source/gba_core/memory.c
        source/gba_core/rom.c
        source/gba_core/save.c
        source/gba_core/sound.c
        source/gba_core/thumb.c
        source/gba_core/timers.c
        source/gba_core/video.c
        source/gui/win_gba_disassembler.c
        source/gui/win_gba_ioviewer.c
        source/gui/win_gba_mapviewer.c
        source/gui/win_gba_memviewer.c
        source/gui/win_gba_palviewer.c
        source/gui/win_gba_sprviewer.c
        source/gui/win_gba_tileviewer.c
        source/gui/win_gb_disassembler.c
        source/gui/win_gb_gbcamviewer.c
        source/gui/win_gb_ioviewer.c
        source/gui/win_gb_mapviewer.c
        source/gui/win_gb_memviewer.c
        source/gui/win_gb_palviewer.c
        source/gui/win_gb_sgbviewer.c
        source/gui/win_gb_sprviewer.c
        source/gui/win_gb_tileviewer.c
        source/gui/win_main.c
        source/gui/win_main_config.c
        source/gui/win_main_config_input.c
        source/gui/win_utils.c
        source/gui/win_utils_draw.c
        source/gui/win_utils_events.c
)

# libpng and SLD2 are required

find_package(PNG REQUIRED)
find_package(SDL2 REQUIRED)

target_include_directories(giibiiadvance
    PRIVATE
        ${PNG_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(giibiiadvance
    PRIVATE
        ${PNG_LIBRARIES}
        ${SDL2_LIBRARIES}
)

# OpenCV is optional. If found, let the user build with GB Camera emulation.

find_package(OpenCV 4)

if(OpenCV_FOUND)
    option(ENABLE_CAMERA "Enable Game Boy Camera emulation" ON)
else()
    set(ENABLE_CAMERA OFF)
endif()

if(ENABLE_CAMERA)
    target_include_directories(giibiiadvance
        PRIVATE
            ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(giibiiadvance
        PRIVATE
            ${OpenCV_LIBRARIES}
    )
else()
    target_compile_definitions(giibiiadvance
        PRIVATE
            -DNO_CAMERA_EMULATION
    )
endif()

# OpenGL is optional. It can be used as library to output graphics.

find_package(OpenGL)

if(OPENGL_FOUND)
    option(ENABLE_OPENGL "Compile with OpenGL" ON)
else()
    set(ENABLE_OPENGL OFF)
endif()

if(ENABLE_OPENGL)
    target_compile_definitions(giibiiadvance
        PRIVATE
            -DENABLE_OPENGL
    )
    target_include_directories(giibiiadvance
        PRIVATE
            ${OPENGL_INCLUDE_DIRS}
    )
    target_link_libraries(giibiiadvance
        PRIVATE
            ${OPENGL_LIBRARIES}
    )
endif()

# In x86 CPUs, replace part of the CPU interpreter by inline assembly.

option(ENABLE_ASM_X86 "Compile with inline assembly" ON)

if(ENABLE_ASM_X86)
    target_compile_definitions(giibiiadvance
        PRIVATE
            -DENABLE_ASM_X86
    )
endif()
