// SPDX-License-Identifier: MIT
//
// Copyright (c) 2014, 2019, Antonio Niño Díaz

#include <stdio.h>
#include <stdlib.h>

void FileLoad(const char *path, void **buffer, unsigned int *size_)
{
	FILE *f = fopen(path, "rb");
	unsigned int size;
	*buffer = NULL;
	if (size_)
		*size_ = 0;

	if (f == NULL) {
		printf("%s couldn't be opened!", path);
		return;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if (size_)
		*size_ = size;
	if (size == 0) {
		printf("Size of %s is 0!", path);
		fclose(f);
		return;
	}
	rewind(f);
	*buffer = malloc(size);
	if (*buffer == NULL) {
		printf("Not enought memory to load %s!", path);
		fclose(f);
		return;
	}
	if (fread(*buffer, size, 1, f) != 1) {
		printf("Error while reading.");
		fclose(f);
		return;
	}

	fclose(f);
}

int main(int argc, char **argv)
{
	if (argc < 2)
		return 0;

	unsigned int *file, size;
	FileLoad(argv[1], (void **)&file, &size);

	FILE *f = fopen("out.c", "w");
	if (f == NULL) {
		free(file);
		return 1;
	}
	int i = 0;
	while (size > 0) {
		fprintf(f, "0x%08X,", file[i++]);
		if ((i % 5) == 0)
			fprintf(f, "\n");
		size -= 4;
	}
	fclose(f);

	return 0;
}
