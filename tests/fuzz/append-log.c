/* Copyright (c) 2023-2026 Markus Uhlin <markus.uhlin@icloud.com>
   All rights reserved.

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
   AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
   PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   PERFORMANCE OF THIS SOFTWARE. */

#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//lint -sem(err, r_no)
//lint -sem(errx, r_no)

static const off_t one_megabyte = 1048576;

static void
usage(const char *exe)
{
	(void) fprintf(stderr, "usage: %s <filename> <megabytes>\n", exe);
	exit(1);
}

int
main(int argc, char *argv[])
{
	FILE		*fp = NULL;
	char		*ep = NULL;
	const int	 maxchars = 1024;
	int		 c, i;
	long int	 lval = 0;
	off_t		 maxfile = 0;
	struct stat	 sb = { 0 };

	if (argc != 3)
		usage(argv[0]);
	else if (geteuid() == 0)
		errx(1, "running the program as root is forbidden");

	errno = 0;
	lval = strtol(argv[2], &ep, 10);
	if (argv[2][0] == '\0' || *ep != '\0')
		errx(1, "argument 2 not a number");
	else if (errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN))
		errx(1, "megabytes out of range");
	else if (lval < 1 || lval > 300)
		errx(1, "megabytes out of range");

	maxfile = (lval * one_megabyte);

	if ((fp = fopen(argv[1], "a")) == NULL) {
		err(1, "fopen");
	} else if (stat(argv[1], &sb) == -1) {
		warn("stat");
		(void) fclose(fp);
		return 1;
	} else if (sb.st_size >= maxfile) {
		warnx("file too large");
		(void) fclose(fp);
		return 1;
	}

	i = 0;

	while ((c = fgetc(stdin)) != EOF && i++ < maxchars) {
		if (fputc(c, fp) == EOF)
			break;
	}

	return (fclose(fp) != 0 ? 1 : 0);
}
