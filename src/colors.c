// Written by Markus Uhlin
// Compile with: cc -O2 -Wall -lm -pipe colors.c

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

long int
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
    return 0;
}
