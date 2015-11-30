PKG_CONFIG = pkg-config
INCS != $(PKG_CONFIG) --cflags sdl2 libpng opencv
LIBS != $(PKG_CONFIG) --libs sdl2 libpng opencv
LIBS += -lGL -lm
SOURCES != ls *.c */*.c
OBJS = $(SOURCES:.c=.o)

all: giibiiadvance

giibiiadvance: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c -o $*.o $<

clean:
	rm -f $(OBJS) giibiiadvance
