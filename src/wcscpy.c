/* Copyright (c) 2014 Markus Uhlin <markus.uhlin@bredband.net>
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

#include <errno.h>
#include <stddef.h>
#include <wchar.h>

/* sw_wcscpy()  --  copy a wide-character string
   Returns 0 on success; and an error number on failure. */
int sw_wcscpy(wchar_t *dest, const wchar_t *src, size_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
	return (EINVAL);
    } else if (wcslen(src) >= size) {
	return (ERANGE);
    } else {
	while (*dest++ = *src++) {
	    ;
	}
    }
    return (0);
}
