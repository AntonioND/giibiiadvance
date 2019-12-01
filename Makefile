# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2019, Antonio Niño Díaz
#
# GiiBiiAdvance - GBA/GB emulator

# User config
# ------------

# `make V=` builds the binary in verbose build mode
V		:= @
NAME		:= giibiiadvance

# Tools
# -----

CC		:= gcc
OBJDUMP		:= objdump
MKDIR		:= mkdir
RM		:= rm -rf
PKG_CONFIG	:= pkg-config

# Directories
# -----------

SOURCEDIR	:= source
BUILDDIR	:= build

# Source files
# ------------

COMMON_SOURCES := \
	source/config.c \
	source/debug_utils.c \
	source/file_explorer.c \
	source/file_utils.c \
	source/font_data.c \
	source/font_utils.c \
	source/general_utils.c \
	source/input_utils.c \
	source/main.c \
	source/png_utils.c \
	source/sound_utils.c \
	source/text_data.c \
	source/window_handler.c \
	source/window_icon_data.c \

GB_SOURCES := \
	source/gb_core/camera.c \
	source/gb_core/cpu.c \
	source/gb_core/daa_table.c \
	source/gb_core/debug.c \
	source/gb_core/debug_video.c \
	source/gb_core/dma.c \
	source/gb_core/gb_main.c \
	source/gb_core/general.c \
	source/gb_core/interrupts.c \
	source/gb_core/licensees.c \
	source/gb_core/mbc.c \
	source/gb_core/memory.c \
	source/gb_core/memory_dmg.c \
	source/gb_core/memory_gbc.c \
	source/gb_core/noise.c \
	source/gb_core/ppu.c \
	source/gb_core/ppu_dmg.c \
	source/gb_core/ppu_gbc.c \
	source/gb_core/rom.c \
	source/gb_core/serial.c \
	source/gb_core/sgb.c \
	source/gb_core/sound.c \
	source/gb_core/video.c \

GBA_SOURCES := \
	source/gba_core/arm.c \
	source/gba_core/bios.c \
	source/gba_core/cpu.c \
	source/gba_core/disassembler.c \
	source/gba_core/dma.c \
	source/gba_core/gba.c \
	source/gba_core/gba_debug_video.c \
	source/gba_core/interrupts.c \
	source/gba_core/memory.c \
	source/gba_core/rom.c \
	source/gba_core/save.c \
	source/gba_core/sound.c \
	source/gba_core/thumb.c \
	source/gba_core/timers.c \
	source/gba_core/video.c \

GUI_SOURCES := \
	source/gui/win_gba_disassembler.c \
	source/gui/win_gba_ioviewer.c \
	source/gui/win_gba_mapviewer.c \
	source/gui/win_gba_memviewer.c \
	source/gui/win_gba_palviewer.c \
	source/gui/win_gba_sprviewer.c \
	source/gui/win_gba_tileviewer.c \
	source/gui/win_gb_disassembler.c \
	source/gui/win_gb_gbcamviewer.c \
	source/gui/win_gb_ioviewer.c \
	source/gui/win_gb_mapviewer.c \
	source/gui/win_gb_memviewer.c \
	source/gui/win_gb_palviewer.c \
	source/gui/win_gb_sgbviewer.c \
	source/gui/win_gb_sprviewer.c \
	source/gui/win_gb_tileviewer.c \
	source/gui/win_main.c \
	source/gui/win_main_config.c \
	source/gui/win_main_config_input.c \
	source/gui/win_utils.c \
	source/gui/win_utils_draw.c \
	source/gui/win_utils_events.c \

SOURCES := $(COMMON_SOURCES) $(GB_SOURCES) $(GBA_SOURCES) $(GUI_SOURCES)

# Defines passed to all files
# ---------------------------

DEFINES		:= \

# Libraries
# ---------

LIBS		:= \

# Include paths
# -------------

INCLUDES	:= \

# Build options
# -------------

# `make ENABLE_OPENCV=1` builds the emulator with OpenCV support
ifeq ($(ENABLE_OPENCV),1)
PKG_CONFIG_LIBS	:= sdl2 libpng opencv
else
PKG_CONFIG_LIBS	:= sdl2 libpng
DEFINES		+= -DNO_CAMERA_EMULATION
endif

# `make DISABLE_ASM_X86=1` builds the emulator without inline assembly
ifneq ($(DISABLE_ASM_X86),1)
DEFINES		+= -DENABLE_ASM_X86
endif

# `make DISABLE_OPENGL=1` builds the emulator without OpenGL support
ifneq ($(DISABLE_OPENGL),1)
DEFINES		+= -DENABLE_OPENGL
endif

INCLUDES	+= `$(PKG_CONFIG) --cflags $(PKG_CONFIG_LIBS)`
LIBS		+= `$(PKG_CONFIG) --libs $(PKG_CONFIG_LIBS)`
LIBS		+= -lGL -lm

# Compiler and linker flags
# -------------------------

WARNFLAGS	:= \
		-Wall \
		-Wformat-truncation=0 \
		-Wextra \
		-Wno-sign-compare \

CFLAGS		+= -std=gnu11 $(WARNFLAGS) $(INCLUDES) $(DEFINES) -O3

LDFLAGS		:= $(LIBS)

# Intermediate build files
# ------------------------

OBJS		:= $(patsubst $(SOURCEDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))
DEPS		:= $(OBJS:.o=.d)

# Rules
# -----

$(BUILDDIR)/%.o : $(SOURCEDIR)/%.c
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D) # Build target's directory if it doesn't exist
	$(V)$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

# Targets
# -------

.PHONY: all clean bin dump

ELF	:= $(NAME)
DUMP	:= $(NAME).dump

all: $(ELF)

$(ELF): $(OBJS)
	@echo "  LD      $@"
	$(V)$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(DUMP): $(ELF)
	@echo "  OBJDUMP $@"
	$(V)$(OBJDUMP) -h -C -S $< > $@

dump: $(DUMP)

clean:
	@echo "  CLEAN"
	$(V)$(RM) $(ELF) $(DUMP) $(BUILDDIR)

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)
