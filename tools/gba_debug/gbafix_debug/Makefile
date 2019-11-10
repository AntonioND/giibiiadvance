# SPDX-License-Identifier: MIT
#
# Copyright (c) 2019, Antonio Niño Díaz

NAME	:= gbafix_debug

SOURCES	:= \
	gbafix_debug.c \

OBJS	:= $(SOURCES:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	rm -f $(OBJS) $(NAME)
