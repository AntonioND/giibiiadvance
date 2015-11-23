PKG_CONFIG = pkg-config
INCS != $(PKG_CONFIG) --cflags sdl2 libpng opencv
LIBS != $(PKG_CONFIG) --libs sdl2 libpng opencv
LIBS += -lGL -lm

OBJS = \
	config.o \
	debug_utils.o \
	file_explorer.o \
	file_utils.o \
	font_data.o \
	font_utils.o \
	gb_core/camera.o \
	gb_core/cpu.o \
	gb_core/daa_table.o \
	gb_core/debug.o \
	gb_core/debug_video.o \
	gb_core/dma.o \
	gb_core/gb_main.o \
	gb_core/general.o \
	gb_core/interrupts.o \
	gb_core/licensees.o \
	gb_core/mbc.o \
	gb_core/memory.o \
	gb_core/memory_dmg.o \
	gb_core/memory_gbc.o \
	gb_core/noise.o \
	gb_core/ppu.o \
	gb_core/ppu_dmg.o \
	gb_core/ppu_gbc.o \
	gb_core/rom.o \
	gb_core/serial.o \
	gb_core/sgb.o \
	gb_core/sound.o \
	gb_core/video.o \
	gba_core/arm.o \
	gba_core/bios.o \
	gba_core/cpu.o \
	gba_core/disassembler.o \
	gba_core/dma.o \
	gba_core/gba.o \
	gba_core/gba_debug_video.o \
	gba_core/interrupts.o \
	gba_core/memory.o \
	gba_core/rom.o \
	gba_core/save.o \
	gba_core/sound.o \
	gba_core/thumb.o \
	gba_core/timers.o \
	gba_core/video.o \
	general_utils.o \
	gui/win_gb_disassembler.o \
	gui/win_gb_gbcamviewer.o \
	gui/win_gb_ioviewer.o \
	gui/win_gb_mapviewer.o \
	gui/win_gb_memviewer.o \
	gui/win_gb_palviewer.o \
	gui/win_gb_sgbviewer.o \
	gui/win_gb_sprviewer.o \
	gui/win_gb_tileviewer.o \
	gui/win_gba_disassembler.o \
	gui/win_gba_ioviewer.o \
	gui/win_gba_mapviewer.o \
	gui/win_gba_memviewer.o \
	gui/win_gba_palviewer.o \
	gui/win_gba_sprviewer.o \
	gui/win_gba_tileviewer.o \
	gui/win_main.o \
	gui/win_main_config.o \
	gui/win_main_config_input.o \
	gui/win_utils.o \
	gui/win_utils_draw.o \
	gui/win_utils_events.o \
	input_utils.o \
	main.o \
	png/png_utils.o \
	sound_utils.o \
	text_data.o \
	window_handler.o

all: giibiiadvance

giibiiadvance: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c -o $*.o $<

clean:
	rm -f $(OBJS) giibiiadvance
