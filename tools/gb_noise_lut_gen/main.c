// SPDX-License-Identifier: MIT
//
// Copyright (c) 2014, 2019, Antonio Niño Díaz

#include <stdio.h>

int seed;
int done;

int update_lfsr(int _7bitmode)
{
	int out = ((seed & 1) ^ 1);
	int nextbit = ((seed >> 1) ^ seed) & 1;
	int shift = _7bitmode ? 6 : 14;
	seed = ((seed >> 1) & (~(1 << shift))) | (nextbit << shift);
	return out;
}

int getbyte(int _7bitmode)
{
	int i, value = 0;
	for (i = 0; i < 8; i++) {
		value = (value << 1) | update_lfsr(_7bitmode);
		if (seed == (_7bitmode ? 0x7F : 0x7FFF))
			done = 1;
	}
	return value;
}

int main(int argc, char *argv[])
{
	int i;
	FILE *f;

	i = 0;
	seed = 0x7F;
	done = 0;
	f = fopen("noise7.txt", "wb");
	while (!done) {
		fprintf(f, "0x%02x%s", getbyte(1),
			(((i++) & 15) != 15) ? "," : ",\n");
	}
	fclose(f);

	i = 0;
	seed = 0x7FFF;
	done = 0;
	f = fopen("noise15.txt", "wb");
	while (!done) {
		fprintf(f, "0x%02x%s", getbyte(0),
			(((i++) & 15) != 15) ? "," : ",\n");
	}
	fclose(f);

	return 0;
}
