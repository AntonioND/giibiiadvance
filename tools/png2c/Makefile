# SPDX-License-Identifier: MIT
#
# Copyright (c) 2019, Antonio Niño Díaz

NAME		:= png2c

PKG_CONFIG	= pkg-config
INCS		= `$(PKG_CONFIG) --cflags libpng`
LIBS		= `$(PKG_CONFIG) --libs libpng`

SOURCES	:= \
	main.c \
	../../png/png_utils.c \

OBJS	:= $(SOURCES:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c -o $*.o $<

clean:
	rm -f $(OBJS) $(NAME)
