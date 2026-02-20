/* Copyright (c) 2023 Markus Uhlin <markus.uhlin@bredband.net>
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

int
main(int argc, char *argv[])
{
	FILE		*fp;
	const int	 maxchars = 1024;
	const off_t	 maxfile = 18874368; // 18 MB
	int		 c, i;
	struct stat	 sb = { 0 };

	if (argc != 2)
		errx(1, "bogus number of args");
	else if (geteuid() == 0)
		errx(1, "running the program as root is forbidden");
	else if ((fp = fopen(argv[1], "a")) == NULL)
		err(1, "fopen");
	else if (stat(argv[1], &sb) == -1) {
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
