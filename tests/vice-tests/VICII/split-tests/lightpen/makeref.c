/**************************************************************************
 *
 * FILE  makeref.c
 * Copyright (c) 2010 Daniel Kahlin
 * Written by Daniel Kahlin <daniel@kahlin.net>
 *
 * DESCRIPTION
 *   Make reference data for lightpen.
 *
 ******/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

char *load_file(const char *name, uint16_t *sa, int *len)
{
    FILE *fp;
    uint16_t lsa;
    int llen;
    char *buf;
    int n;

    fp = fopen(name, "rb");
    if (!fp) {
	fprintf(stderr, "couldn't open file for reading");
	exit(-1);
    }
    fseek(fp, 0, SEEK_END);
    llen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    lsa = fgetc(fp);
    lsa |= fgetc(fp) << 8;
    llen -= 2;
    buf = malloc(llen);
    if (!buf) {
	fprintf(stderr, "couldn't malloc");
	exit(-1);
    }

    n = fread(buf, 1, llen, fp);
    if (n != llen) {
	fprintf(stderr, "premature end of file while reading");
	exit(-1);
    }

    fclose(fp);

    *sa=lsa;
    *len=llen;
    return buf;
}

void save_file(const char *name, char *buf, int len)
{
    FILE *fp;

    fp = fopen(name, "wb");
    if (!fp) {
	fprintf(stderr, "couldn't open file for writing");
	exit(-1);
    }
    fwrite(buf, 1, len, fp);

    fclose(fp);
}

int main(int argc, char *argv[])
{
    char *buf;
    uint16_t sa;
    int len;

    buf = load_file(argv[1], &sa, &len);
    if (buf[0x2fe] == buf[0x02fd]) {
	int diff;
	printf("pre r03 dump, correcting\n");
	diff = buf[0x2ff] - buf[0x2fe];
	buf[0x0fe] = buf[0x0ff];
	buf[0x1fe] = buf[0x1ff];
	buf[0x2fe] = buf[0x2ff];
	buf[0x2ff] = buf[0x2fe] + diff;
	buf[0x3fe] = buf[0x3ff];
	buf[0x4fe] = buf[0x4ff];
    }

    save_file(argv[2], buf, len);

    free(buf);
    exit(0);
}

/* eof */
