PKG_CONFIG	= pkg-config
INCS		= `$(PKG_CONFIG) --cflags sdl2 libpng opencv`
LIBS		= `$(PKG_CONFIG) --libs sdl2 libpng opencv`
LIBS		+= -lGL -lm

COMMON_SOURCES = \
	config.c \
	debug_utils.c \
	file_explorer.c \
	file_utils.c \
	font_data.c \
	font_utils.c \
	general_utils.c \
	input_utils.c \
	main.c \
	png_utils.c \
	sound_utils.c \
	text_data.c \
	window_handler.c \

GB_SOURCES = \
	gb_core/camera.c \
	gb_core/cpu.c \
	gb_core/daa_table.c \
	gb_core/debug.c \
	gb_core/debug_video.c \
	gb_core/dma.c \
	gb_core/gb_main.c \
	gb_core/general.c \
	gb_core/interrupts.c \
	gb_core/licensees.c \
	gb_core/mbc.c \
	gb_core/memory.c \
	gb_core/memory_dmg.c \
	gb_core/memory_gbc.c \
	gb_core/noise.c \
	gb_core/ppu.c \
	gb_core/ppu_dmg.c \
	gb_core/ppu_gbc.c \
	gb_core/rom.c \
	gb_core/serial.c \
	gb_core/sgb.c \
	gb_core/sound.c \
	gb_core/video.c \

GBA_SOURCES = \
	gba_core/arm.c \
	gba_core/bios.c \
	gba_core/cpu.c \
	gba_core/disassembler.c \
	gba_core/dma.c \
	gba_core/gba.c \
	gba_core/gba_debug_video.c \
	gba_core/interrupts.c \
	gba_core/memory.c \
	gba_core/rom.c \
	gba_core/save.c \
	gba_core/sound.c \
	gba_core/thumb.c \
	gba_core/timers.c \
	gba_core/video.c \

GUI_SOURCES = \
	gui/win_gba_disassembler.c \
	gui/win_gba_ioviewer.c \
	gui/win_gba_mapviewer.c \
	gui/win_gba_memviewer.c \
	gui/win_gba_palviewer.c \
	gui/win_gba_sprviewer.c \
	gui/win_gba_tileviewer.c \
	gui/win_gb_disassembler.c \
	gui/win_gb_gbcamviewer.c \
	gui/win_gb_ioviewer.c \
	gui/win_gb_mapviewer.c \
	gui/win_gb_memviewer.c \
	gui/win_gb_palviewer.c \
	gui/win_gb_sgbviewer.c \
	gui/win_gb_sprviewer.c \
	gui/win_gb_tileviewer.c \
	gui/win_main.c \
	gui/win_main_config.c \
	gui/win_main_config_input.c \
	gui/win_utils.c \
	gui/win_utils_draw.c \
	gui/win_utils_events.c \

SOURCES = $(COMMON_SOURCES) $(GB_SOURCES) $(GBA_SOURCES) $(GUI_SOURCES)

OBJS = $(SOURCES:.c=.o)

all: giibiiadvance

giibiiadvance: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c -o $*.o $<

clean:
	rm -f $(OBJS) giibiiadvance
