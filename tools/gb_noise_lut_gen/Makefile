# SPDX-License-Identifier: MIT
#
# Copyright (c) 2019, Antonio Niño Díaz

NAME	:= gb_noise_lut_gen

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
