// Written by Markus Uhlin
// Compile with: cc -O2 -Wall -lm -pipe colors.c

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef nitems
#define nitems(_a) (sizeof((_a)) / sizeof((_a)[0]))
#endif

typedef struct {
    short int r;
    short int g;
    short int b;
} rgb_t;

static struct {
    short int num;
    rgb_t val;
} colors[] = {
    /* 16-27 */
    { 52,  {0x47,0x00,0x00} },
    { 94,  {0x47,0x21,0x00} },
    { 100, {0x47,0x47,0x00} },
    { 58,  {0x32,0x47,0x00} },
    { 22,  {0x00,0x47,0x00} },
    { 29,  {0x00,0x47,0x2c} },
    { 23,  {0x00,0x47,0x47} },
    { 24,  {0x00,0x27,0x47} },
    { 17,  {0x00,0x00,0x47} },
    { 54,  {0x2e,0x00,0x47} },
    { 53,  {0x47,0x00,0x47} },
    { 89,  {0x47,0x00,0x2a} },

    /* 28-39 */
    { 88,  {0x74,0x00,0x00} },
    { 130, {0x74,0x3a,0x00} },
    { 142, {0x74,0x74,0x00} },
    { 64,  {0x51,0x74,0x00} },
    { 28,  {0x00,0x74,0x00} },
    { 35,  {0x00,0x74,0x49} },
    { 30,  {0x00,0x74,0x74} },
    { 25,  {0x00,0x40,0x74} },
    { 18,  {0x00,0x00,0x74} },
    { 91,  {0x4b,0x00,0x74} },
    { 90,  {0x74,0x00,0x74} },
    { 125, {0x74,0x00,0x45} },

    /* 40-51 */
    { 124, {0xb5,0x00,0x00} },
    { 166, {0xb5,0x63,0x00} },
    { 184, {0xb5,0xb5,0x00} },
    { 106, {0x7d,0xb5,0x00} },
    { 34,  {0x00,0xb5,0x00} },
    { 49,  {0x00,0xb5,0x71} },
    { 37,  {0x00,0xb5,0xb5} },
    { 33,  {0x00,0x63,0xb5} },
    { 19,  {0x00,0x00,0xb5} },
    { 129, {0x75,0x00,0xb5} },
    { 127, {0xb5,0x00,0xb5} },
    { 161, {0xb5,0x00,0x6b} },

    /* 52-63 */
    { 196, {0xff,0x00,0x00} },
    { 208, {0xff,0x8c,0x00} },
    { 226, {0xff,0xff,0x00} },
    { 154, {0xb2,0xff,0x00} },
    { 46,  {0x00,0xff,0x00} },
    { 86,  {0x00,0xff,0xa0} },
    { 51,  {0x00,0xff,0xff} },
    { 75,  {0x00,0x8c,0xff} },
    { 21,  {0x00,0x00,0xff} },
    { 171, {0xa5,0x00,0xff} },
    { 201, {0xff,0x00,0xff} },
    { 198, {0xff,0x00,0x98} },

    /* 64-75 */
    { 203, {0xff,0x59,0x59} },
    { 215, {0xff,0xb4,0x59} },
    { 227, {0xff,0xff,0x71} },
    { 191, {0xcf,0xff,0x60} },
    { 83,  {0x6f,0xff,0x6f} },
    { 122, {0x65,0xff,0xc9} },
    { 87,  {0x6d,0xff,0xff} },
    { 111, {0x59,0xb4,0xff} },
    { 63,  {0x59,0x59,0xff} },
    { 177, {0xc4,0x59,0xff} },
    { 207, {0xff,0x66,0xff} },
    { 205, {0xff,0x59,0xbc} },

    /* 76-87 */
    { 217, {0xff,0x9c,0x9c} },
    { 223, {0xff,0xd3,0x9c} },
    { 229, {0xff,0xff,0x9c} },
    { 193, {0xe2,0xff,0x9c} },
    { 157, {0x9c,0xff,0x9c} },
    { 158, {0x9c,0xff,0xdb} },
    { 159, {0x9c,0xff,0xff} },
    { 153, {0x9c,0xd3,0xff} },
    { 147, {0x9c,0x9c,0xff} },
    { 183, {0xdc,0x9c,0xff} },
    { 219, {0xff,0x9c,0xff} },
    { 212, {0xff,0x94,0xd3} },

    /* 88-98 */
    { 16,  {0x00,0x00,0x00} },
    { 233, {0x13,0x13,0x13} },
    { 235, {0x28,0x28,0x28} },
    { 237, {0x36,0x36,0x36} },
    { 239, {0x4d,0x4d,0x4d} },
    { 241, {0x65,0x65,0x65} },
    { 244, {0x81,0x81,0x81} },
    { 247, {0x9f,0x9f,0x9f} },
    { 250, {0xbc,0xbc,0xbc} },
    { 254, {0xe2,0xe2,0xe2} },
    { 231, {0xff,0xff,0xff} },
};

static long int
getval(short int i)
{
    const double input_start = 0.0;
    const double input_end = 255.0;
    const double input = (double) i;
    const double output_start = 0.0;
    const double output_end = 1000.0;
    const double out =
	output_start +
	((output_end - output_start) / (input_end - input_start)) *
	(input - input_start);
    return lround(out);
}

static char *
rgb(short int r, short int g, short int b)
{
    static char out[128] = { '\0' };
    const int ret =
	snprintf(out, sizeof out, "%ld,%ld,%ld",
	    getval(r), getval(g), getval(b));
    if (ret == -1 || ret >= sizeof out) {
	fputs("rgb: snprintf error\n", stderr);
	exit(1);
    }
    return (&out[0]);
}

static const char *
get_spaces(const short int color)
{
    if (color < 10)
	return "   ";
    else if (color < 100)
	return "  ";
    return " ";
}

static void
dump_struct(FILE *fp)
{
    fputs("typedef struct {\n", fp);
    fputs("    short int r;\n", fp);
    fputs("    short int g;\n", fp);
    fputs("    short int b;\n", fp);
    fputs("} rgb_t;\n\n", fp);
    fputs("static struct {\n", fp);
    fputs("    short int color;\n", fp);
    fputs("    rgb_t val;\n", fp);
    fputs("} ext_colors_rgb[] = {\n", fp);

    for (size_t i = 0; i < nitems(colors); i++) {
	switch (colors[i].num) {
	case 52:
	    fprintf(fp, "    /* 16-27 */\n");
	    break;
	case 88:
	    fprintf(fp, "\n    /* 28-39 */\n");
	    break;
	case 124:
	    fprintf(fp, "\n    /* 40-51 */\n");
	    break;
	case 196:
	    fprintf(fp, "\n    /* 52-63 */\n");
	    break;
	case 203:
	    fprintf(fp, "\n    /* 64-75 */\n");
	    break;
	case 217:
	    fprintf(fp, "\n    /* 76-87 */\n");
	    break;
	case 16:
	    fprintf(fp, "\n    /* 88-98 */\n");
	    break;
	}

	rgb_t current = colors[i].val;
	fprintf(fp, "    { %hd,%s{%s} },\n", colors[i].num,
	    get_spaces(colors[i].num), rgb(current.r,current.g,current.b));
    }

    fputs("};\n", fp);
}

int
main()
{
    printf("0\tWhite\t\t(%s)\n",     rgb(255,255,255));
    printf("1\tBlack\t\t(%s)\n",     rgb(0,0,0));
    printf("2\tBlue\t\t(%s)\n",      rgb(0,0,127));
    printf("3\tGreen\t\t(%s)\n",     rgb(0,147,0));
    printf("4\tLight Red\t(%s)\n",   rgb(255,0,0));
    printf("5\tBrown\t\t(%s)\n",     rgb(127,0,0));
    printf("6\tPurple\t\t(%s)\n",    rgb(156,0,156));
    printf("7\tOrange\t\t(%s)\n",    rgb(252,127,0));
    printf("8\tYellow\t\t(%s)\n",    rgb(255,255,0));
    printf("9\tLight Green\t(%s)\n", rgb(0,252,0));
    printf("10\tCyan\t\t(%s)\n",     rgb(0,147,147));
    printf("11\tLight Cyan\t(%s)\n", rgb(0,255,255));
    printf("12\tLight Blue\t(%s)\n", rgb(0,0,252));
    printf("13\tPink\t\t(%s)\n",     rgb(255,0,255));
    printf("14\tGrey\t\t(%s)\n",     rgb(127,127,127));
    printf("15\tLight Grey\t(%s)\n", rgb(210,210,210));

    FILE *fp = NULL;
    const char path[] = "/tmp/struct.c";

    if ((fp = fopen(path, "w")) == NULL) {
	perror("main: fopen");
	return 1;
    } else {
	dump_struct(fp);
    }

    if (fp)
	fclose(fp);
    printf("struct placed in %s\n", path);
    return 0;
}
