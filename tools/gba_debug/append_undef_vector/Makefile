# SPDX-License-Identifier: MIT
#
# Copyright (c) 2019, Antonio Niño Díaz

NAME	:= append_undef_vector

SOURCES	:= \
	main.c \

OBJS	:= $(SOURCES:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	rm -f $(OBJS) $(NAME)
