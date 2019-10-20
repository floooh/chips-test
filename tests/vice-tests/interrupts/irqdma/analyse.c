
// naive tool to compare dumps made by irqdma tests

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLEN  (64*1024)

FILE *f1,*f2;
char *name1, *name2;

unsigned char b1[MAXLEN];
unsigned char b2[MAXLEN];

// normal mode
//
// d015  $00...$7f ($80)   $80
//  timer $10...$8f ($90)  $80
unsigned int getoffsA(int d015, int timer)
{
    return ((d015 * 0x80) + timer) * 2;
}

unsigned int getdiffA1(int d015, int timer)
{
    int offs = getoffsA(d015, timer);
    return b1[offs] - b2[offs];
}

unsigned int getdiffA2(int d015, int timer)
{
    int offs = getoffsA(d015, timer) + 1;
    return b1[offs] - b2[offs];
}

// b-mode
//
// d015  $04...$13 ($14)   $10
//  timer $30...$af ($b0)  $80
//   delay $40...$39 ($38) $08
unsigned int getoffsB(int d015, int timer, int delay)
{
    return ((d015 * (0x08 * 0x80)) + (timer * 0x08) + delay) * 2;
}

unsigned int getdiffB1(int d015, int timer, int delay)
{
    int offs = getoffsB(d015, timer, delay);
    return b1[offs] - b2[offs];
}

unsigned int getdiffB2(int d015, int timer, int delay)
{
    int offs = getoffsB(d015, timer, delay) + 1;
    return b1[offs] - b2[offs];
}

void usage(void)
{
    printf("usage: analyse [options] <dump1> <dump2>\n\n"
           "options:\n"
           "-b    b-mode (for -b tests)\n"
          );
    exit(-1);
}

int main(int argc,char *argv[])
{
    int c1,c2,cnt,total,i,timer,d015,bmode = 0,delay;
    float percent;
    unsigned char *p1, *p2;

    i = 1;
    if (argv[i][0] == '-') {
        if (!strcmp(argv[i], "-b")) {
            bmode = 1;
            i++;
        } else {
            usage();
        }
    }
    if ((argc - i) < 2) {
        usage();
    }
    name1 = argv[i];
    name2 = argv[i + 1];

    f1 = fopen(name1, "rb");
    if (!f1) {
        fprintf(stderr, "could not open '%s'\n", name1);
    }
    f2 = fopen(name2, "rb");
    if (!f2) {
        fprintf(stderr, "could not open '%s'\n", name2);
    }
    cnt = 0;total=0;

    fgetc(f1);fgetc(f1);
    fgetc(f2);fgetc(f2);

    p1 = &b1[0];
    p2 = &b2[0];
    while (!feof(f1)) {
        c1 = fgetc(f1);
        c2 = fgetc(f2);
        if (c1 != c2) {
            cnt++;
        }
        *p1++ = c1;
        *p2++ = c2;
        total++;
    }
    total -= 1;
    fclose(f1);
    fclose(f2);

    printf("comparing '%s' and '%s':\n\n", name1, name2);

    percent = (float)cnt * 100.0f / (float)total;
    printf("differences: %d of %d = %f%%\n\n", cnt, total, percent);

    if (bmode == 0) {
        // normal mode
        //
        // d015  $00...$7f ($80)   $80
        //  timer $10...$8f ($90)  $80
        printf("normal mode:\n\n");
        printf("- d015  $00...$7f\n");
        printf("-- timer $10...$8f\n\n");

        printf("            --> timer value -->\n");
        printf("offs d015  | ");
        for (timer = 0; timer < 0x80; timer++) {
            printf("%d", ((timer) / 10) % 10);
        }
        printf("\n");
        printf("           | ");
        for (timer = 0; timer < 0x80; timer++) {
            printf("%d", (timer) % 10);
        }
        printf("\n");

        for (d015 = 0; d015 < 0x80; d015++) {
            printf("%04x   %02x  | ", 0x2000 + getoffsA(d015, 0), d015);
            for (timer = 0; timer < 0x80; timer++) {
                if ((getdiffA1(d015, timer) != 0) && (getdiffA2(d015, timer) != 0)) {
                    printf("*");
                } else if (getdiffA1(d015, timer) != 0) {
                    printf("X");
                } else if (getdiffA2(d015, timer) != 0) {
                    printf("Y");
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }
        printf("\n\n");

        printf("            --> $d015 value -->\n");
        printf("offs timer | ");
        for (d015 = 0; d015 < (0x80); d015++) {
            printf("%x", d015 >> 4);
        }
        printf("\n");
        printf("           | ");
        for (d015 = 0; d015 < (0x80); d015++) {
            printf("%x", d015 & 0x0f);
        }
        printf("\n");

        for (timer = 0; timer < 0x80; timer ++) {
            printf("%04x  %4d | ", 0x2000 + getoffsA(0, timer), timer);
            for (d015 = 0; d015 < 0x80; d015++) {
                if ((getdiffA1(d015, timer) != 0) && (getdiffA2(d015, timer) != 0)) {
                    printf("*");
                } else if (getdiffA1(d015, timer) != 0) {
                    printf("X");
                } else if (getdiffA2(d015, timer) != 0) {
                    printf("Y");
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }

    } else {
        // b-mode
        //
        // d015  $04...$13 ($14)   $10
        //  timer $30...$af ($b0)  $80
        //   delay $40...$39 ($38) $08
        printf("b-mode:\n\n");
        printf("- d015  $04...$13\n");
        printf("-- timer $30...$af\n");
        printf("--- delay $40...$39\n\n");

        printf("                  --> timer value -->\n");
        printf("offs d015 delay | ");
        for (timer = 0; timer < 0x80; timer++) {
            printf("%d", ((timer) / 10) % 10);
        }
        printf("\n");
        printf("                | ");
        for (timer = 0; timer < 0x80; timer++) {
            printf("%d", (timer) % 10);
        }
        printf("\n");

        for (d015 = 0; d015 < 0x10; d015++) {
            for (delay = 0; delay < 0x08; delay++) {
                printf("%04x   %02x    %02x | ", 0x2000 + getoffsB(d015, 0, delay), d015 + 4, 0x40 - delay);
                for (timer = 0; timer < 0x80; timer++) {
                    if ((getdiffB1(d015, timer, delay) != 0) && (getdiffB2(d015, timer, delay) != 0)) {
                        printf("*");
                    } else if (getdiffB1(d015, timer, delay) != 0) {
                        printf("X");
                    } else if (getdiffB2(d015, timer, delay) != 0) {
                        printf("Y");
                    } else {
                        printf(".");
                    }
                }
                printf("\n");
            }
            printf("\n");
        }
        printf("\n\n");

        for (delay = 0; delay < 0x08; delay++) {
            for (d015 = 0; d015 < 0x10; d015++) {
                printf("%04x   %02x    %02x | ", 0x2000 + getoffsB(d015, 0, delay), d015 + 4, 0x40 - delay);
                for (timer = 0; timer < 0x80; timer++) {
                    if ((getdiffB1(d015, timer, delay) != 0) && (getdiffB2(d015, timer, delay) != 0)) {
                        printf("*");
                    } else if (getdiffB1(d015, timer, delay) != 0) {
                        printf("X");
                    } else if (getdiffB2(d015, timer, delay) != 0) {
                        printf("Y");
                    } else {
                        printf(".");
                    }
                }
                printf("\n");
            }
            printf("\n");
        }
    }

    exit(EXIT_SUCCESS);
}
