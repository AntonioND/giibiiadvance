// SPDX-License-Identifier: MIT
//
// Copyright (c) 2014, 2019, Antonio Niño Díaz

#include <stdio.h>

int asciitoint(char c)
{
	if ((c >= '0') && (c <= '9'))
		return c - '0';
	if ((c >= 'A') && (c <= 'F'))
		return c - 'A' + 10;
	if ((c >= 'a') && (c <= 'f'))
		return c - 'a' + 10;
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;

	char addr[8];
	FILE *faddr = fopen("addr.txt", "rb");
	fread(&(addr[0]), 1, 1, faddr);
	fread(&(addr[1]), 1, 1, faddr);
	fread(&(addr[2]), 1, 1, faddr);
	fread(&(addr[3]), 1, 1, faddr);
	fread(&(addr[4]), 1, 1, faddr);
	fread(&(addr[5]), 1, 1, faddr);
	fread(&(addr[6]), 1, 1, faddr);
	fread(&(addr[7]), 1, 1, faddr);
	int i;
	for (i = 0; i < 8; i++)
		addr[i] = asciitoint(addr[i]);
	unsigned int address = 0;
	for (i = 0; i < 8; i++)
		address = (16 * address) + addr[i];
	fclose(faddr);
	printf("\nRead address: 0x%08X\n", address);

	FILE *datafile = fopen(argv[1], "rb+");
	unsigned int size;

	if (datafile == NULL) {
		char msg[2048];
		printf(msg, "%s couldn't be opened!", argv[1]);
		return 1;
	}
	fseek(datafile, 0, SEEK_END);
	size = ftell(datafile);

	int add_size = (0x09FE2000 - 0x08000000) - size;

	char c = 0;
	while (add_size--)
		fwrite(&c, 1, 1, datafile);

	// ldr r0, =(FUNC_ADDR | 1)
	unsigned char op[4] = { 0x00, 0x00, 0x1F, 0xE5 };
	fwrite(&op, 1, 4, datafile);
	// bx r0
	unsigned char op2[4] = { 0x10, 0xFF, 0x2F, 0xE1 };
	fwrite(&op2, 1, 4, datafile);

	int UndefVector = address | 1;
	unsigned char op3[4] = {
		UndefVector & 0xFF,
		(UndefVector >> 8) & 0xFF,
		(UndefVector >> 16) & 0xFF,
		(UndefVector >> 24) & 0xFF
	}; // (FUNC_ADDR | 1) -- switch to thumb
	fwrite(&op3, 1, 4, datafile);

	fclose(datafile);

	return 0;
}
