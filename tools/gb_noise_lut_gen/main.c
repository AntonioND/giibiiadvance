
#include <stdio.h>

int seed;
bool exit;

int update_lfsr(bool _7bitmode)
{
	int out = ((seed & 1) ^ 1);
	int nextbit = ((seed >> 1) ^ seed) & 1;
	int shift = _7bitmode ? 6 : 14;
	seed = (  (seed >> 1) & (~(1<<shift))   )  |  (nextbit<<shift) ;
	return out;
}

int getbyte(bool _7bitmode)
{
	int i, value = 0;
	for(i = 0; i < 8; i++)
	{
		value = (value << 1) | update_lfsr(_7bitmode);
		if(seed == (_7bitmode ? 0x7F : 0x7FFF)) exit = true;
	}
	return value;
}

int main(int argc, char * argv[])
{
	int i;

	i = 0; seed = 0x7F; exit = false;
	FILE * out = fopen("noise7.txt","wb");
	while(!exit) fprintf(out,"0x%02x%s",getbyte(true),(((i++)&15) != 15) ? "," : ",\n");
	fclose(out);

	i = 0; seed = 0x7FFF; exit = false;
	out = fopen("noise15.txt","wb");
	while(!exit) fprintf(out,"0x%02x%s",getbyte(false),(((i++)&15) != 15) ? "," : ",\n");
	fclose(out);

	return 0;
}

