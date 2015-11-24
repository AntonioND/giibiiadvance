# GNU Make 3.x doesn't support the "!=" shell syntax, so here's an alternative

PKG_CONFIG = pkg-config
INCS = $(shell $(PKG_CONFIG) --cflags sdl2 libpng opencv)
LIBS = $(shell $(PKG_CONFIG) --libs sdl2 libpng opencv)
SOURCES = $(wildcard *.c */*.c)

include Makefile
