#define ADD_EXPORTS

#include "main.h"
//#include "stdio.h"
#include "stdlib.h"
#include "string.h"

typedef unsigned char byte;
typedef unsigned short ushort;

#define min_noreps (2)
#define max_noreps (0x80)
#define minlen ((ver == '1') ? 3 : 4)
#define maxlen ((ver == '1') ? 0x12 : 0x83)
#define maxfrom ((ver == '1') ? 0x800 : 0x10000)
#define maxfrom_mask (maxfrom - 1)

byte read_byte(byte *input, int *readoff)
{
	return (input[(*readoff)++]);
}

void write_byte(byte *output, int *writeoff, byte b)
{
	output[(*writeoff)++] = b;
}

ushort read_word(byte *input, int *readoff)
{
	ushort retn = read_byte(input, readoff) << 8;
	retn |= read_byte(input, readoff);
	return retn;
}

ushort read_word_inv(byte *input, int *readoff)
{
	ushort retn = read_byte(input, readoff);
	retn |= (read_byte(input, readoff) << 8);
	return retn;
}

void write_word(byte *output, int *writeoff, ushort w)
{
	write_byte(output, writeoff, w >> 8);
	write_byte(output, writeoff, w & 0xFF);
}

void write_word_inv(byte *output, int *writeoff, ushort w)
{
	write_byte(output, writeoff, w & 0xFF);
	write_byte(output, writeoff, w >> 8);
}

int ADDCALL decompress(byte *input, byte *output)
{
	int readoff = 0, writeoff = 0, from = 0;
	byte reps = 0;
	byte *copy_from = 0;

	byte b = 0, ver = 0;
	ushort out_size = 0;

	ver = read_word(input, &readoff) & 0xFF;
	out_size = read_word_inv(input, &readoff);

	if (ver != '1' && ver != '2')
	{
		for (ushort i = 0; i < out_size; ++i)
		{
			b = read_byte(input, &readoff);
			write_byte(output, &writeoff, b);
		}

		return out_size;
	}

	while (writeoff < out_size)
	{
		b = read_byte(input, &readoff);

		if (b & 0x80)
		{
			switch (ver)
			{
			case '1':
			{
				// 0rrr rfff ffff ffff
				reps = ((b >> 3) & 0xF) + minlen;
				from = ((b & 7) << 8);
				from |= read_byte(input, &readoff);

				from = (short)(writeoff - from - 1);
			} break;
			case '2':
			{
				// 0rrr rrrr ffff ffff
				// ffff ffff
				reps = (b & 0x7F) + minlen;
				from = (read_byte(input, &readoff) << 8);
				from |= read_byte(input, &readoff);

				from = (int)(writeoff - from - 1);
			} break;
			}
			copy_from = output;
		}
		else
		{
			// 0rrr rrrr
			reps = (b & 0x7F) + 1;
			from = readoff;
			copy_from = input;
			readoff += reps;
		}

		for (ushort i = 0; i < reps; ++i)
		{
			b = read_byte(copy_from, &from);
			write_byte(output, &writeoff, b);
		}
	}

	return writeoff;
}

int ADDCALL compressed_size(byte *input)
{
	int readoff = 0, writeoff = 0;
	byte b = 0, reps = 0, ver = 0;
	ushort out_size = 0;

	ver = read_word(input, &readoff) & 0xFF;
	out_size = read_word_inv(input, &readoff);

	if (ver != '1' && ver != '2')
	{
		return out_size;
	}

	while (writeoff < out_size)
	{
		b = read_byte(input, &readoff);

		if (b & 0x80)
		{
			switch (ver)
			{
			case '1':
			{
				// 0rrr rfff ffff ffff
				reps = ((b >> 3) & 0xF) + minlen;
				readoff++;
			} break;
			case '2':
			{
				// 0rrr rrrr ffff ffff
				// ffff ffff
				reps = (b & 0x7F) + minlen;
				readoff += 2;
			} break;
			}
		}
		else
		{
			// 0rrr rrrr
			reps = (b & 0x7F) + 1;
			readoff += reps;
		}

		writeoff += reps;
	}

	return readoff;
}

int find_matches(byte *input, int readoff, int size, ushort *reps, int *from, byte ver)
{
	ushort len = 0;
	int pos = 0;

	*from = 0;
	*reps = 1;

	while (
		pos < readoff &&
		readoff < size
		)
	{
		len = 0;

		while (
			pos < readoff &&
			(readoff - pos) < maxfrom &&
			input[readoff] != input[pos]
			)
		{
			pos++;
		}

		while (
			pos < readoff &&
			readoff + len < size &&
			len < maxlen &&
			(readoff - pos) < maxfrom &&
			input[readoff + len] == input[pos + len]
			)
		{
			len++;
		}

		if (len >= *reps && len >= minlen)
		{
			*reps = len;
			*from = pos;
		}

		pos++;
	}

	return (*reps >= minlen);
}

int do_compress(byte *input, byte *output, int size, byte ver)
{
	int readoff = 0, writeoff = 0, from = 0;
	ushort no_reps = 0, reps = 0;
	byte b = 0;

	write_word(output, &writeoff, ver);
	write_word_inv(output, &writeoff, (ushort)size);

	if (ver != '1' && ver != '2')
	{
		for (int i = 0; i < size; ++i)
		{
			b = read_byte(input, &readoff);
			write_byte(output, &writeoff, b);
		}

		return writeoff;
	}

	while (readoff < size)
	{
		no_reps = 0;
		while (
			readoff + no_reps < size &&
			no_reps < max_noreps &&
			!find_matches(input, readoff + no_reps, size, &reps, &from, ver)
			)
		{
			no_reps++;
		}

		if (no_reps >= min_noreps)
		{
			b = (byte)(no_reps - 1);
			write_byte(output, &writeoff, b);

			for (byte i = 0; i < no_reps; ++i)
			{
				b = read_byte(input, &readoff);
				write_byte(output, &writeoff, b);
			}
		}
		else
		{
			find_matches(input, readoff, size, &reps, &from, ver);

			if (reps >= minlen)
			{
				b = 0x80;

				switch (ver)
				{
				case '1':
				{
					// 0rrr rfff ffff ffff
					from = (readoff - from - 1) & maxfrom_mask;
					b |= ((reps - minlen) & 0xF) << 3;
					b |= (from >> 8) & 7;
					write_byte(output, &writeoff, b);
					write_byte(output, &writeoff, (byte)(from & 0xFF));
				} break;
				case '2':
				{
					// 0rrr rrrr ffff ffff
					// ffff ffff
					from = (readoff - from - 1) & maxfrom_mask;
					b |= (reps - minlen) & 0x7F;
					write_byte(output, &writeoff, b);
					write_byte(output, &writeoff, (byte)(from >> 8));
					write_byte(output, &writeoff, (byte)(from & 0xFF));
				} break;
				}

				readoff += reps;
			}
			else
			{
				write_byte(output, &writeoff, 0);

				b = read_byte(input, &readoff);
				write_byte(output, &writeoff, b);
			}
		}
	}

	return writeoff;
}

int ADDCALL compress(byte *input, byte *output, int size)
{
	int min_writeoff = 0, writeoff = 0;
	byte ver = 0;

	ver = '1';
	writeoff = do_compress(input, output, size, ver);
	min_writeoff = writeoff;

	writeoff = do_compress(input, output, size, '2');
	if (min_writeoff > writeoff)
	{
		min_writeoff = writeoff;
		ver = '2';
	}

	writeoff = do_compress(input, output, size, '3');
	if (min_writeoff > writeoff)
	{
		min_writeoff = writeoff;
		ver = '3';
	}

	writeoff = do_compress(input, output, size, ver);

	return (writeoff & 1) ? writeoff + 1 : writeoff;
}

/*
int main(int argc, char *argv[])
{
	byte *input, *output;

	FILE *inf = fopen(argv[1], "rb");

	input = (byte *)malloc(0x10000);
	output = (byte *)malloc(0x10000);

	char mode = (argv[3][0]);

	if (mode == 'd')
	{
		long offset = strtol(argv[4], NULL, 16);
		fseek(inf, offset, SEEK_SET);
	}

	fread(&input[0], 1, 0x10000, inf);

	int dest_size;
	if (mode == 'd')
	{
		dest_size = decompress(input, output);
	}
	else
	{
		fseek(inf, 0, SEEK_END);
		int dec_size = ftell(inf);

		dest_size = compress(input, output, dec_size);
	}

	if (dest_size != 0)
	{
		FILE *outf = fopen(argv[2], "wb");
		fwrite(&output[0], 1, dest_size, outf);
		fclose(outf);
	}

	fclose(inf);

	free(input);
	free(output);

	return 0;
}
*/
