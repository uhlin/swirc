/* colormap.cpp  --  output colors
   Copyright (C) 2020-2024 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <string>
#include <vector>

#include "../cursesInit.h"
#include "../printtext.h"
#include "../strHand.h"

#include "colormap.h"
#include "i18n.h"

#if defined(__cplusplus) && __cplusplus >= 201103L
#define OUT_PB(str) out.emplace_back(str)
#else
#define OUT_PB(str) out.push_back(str)
#endif

void	pb_linux_colors(std::vector<std::string> &);
void	pb_other_colors(std::vector<std::string> &);

void
pb_linux_colors(std::vector<std::string> &out)
{
	OUT_PB("\0030,16 16 \0030,17 17 \0030,18 18 \0030,19 19 \0030,20 20 "
	       "\0030,21 21 \0030,22 22 \0030,23 23 \0030,24 24 \0030,25 25 "
	       "\0030,26 26 \0030,27 27 ");
	OUT_PB("\0030,28 28 \0030,29 29 \0030,30 30 \0030,31 31 \0030,32 32 "
	       "\0030,33 33 \0030,34 34 \0030,35 35 \0030,36 36 \0030,37 37 "
	       "\0030,38 38 \0030,39 39 ");
	OUT_PB("\0030,40 40 \0030,41 41 \0030,42 42 \0030,43 43 \0030,44 44 "
	       "\0030,45 45 \0030,46 46 \0030,47 47 \0030,48 48 \0030,49 49 "
	       "\0030,50 50 \0030,51 51 ");
	OUT_PB("\0030,52 52 \0030,53 53 \0031,54 54 \0031,55 55 \0031,56 56 "
	       "\0031,57 57 \0031,58 58 \0030,59 59 \0030,60 60 \0030,61 61 "
	       "\0030,62 62 \0030,63 63 ");
	OUT_PB("\0030,64 64 \0031,65 65 \0031,66 66 \0031,67 67 \0031,68 68 "
	       "\0031,69 69 \0031,70 70 \0031,71 71 \0030,72 72 \0030,73 73 "
	       "\0030,74 74 \0030,75 75 ");
	OUT_PB("\0031,76 76 \0031,77 77 \0031,78 78 \0031,79 79 \0031,80 80 "
	       "\0031,81 81 \0031,82 82 \0031,83 83 \0031,84 84 \0031,85 85 "
	       "\0031,86 86 \0031,87 87 ");
	OUT_PB("\0030,88 88 \0030,89 89 \0030,90 90 \0030,91 91 \0030,92 92 "
	       "\0030,93 93 \0030,94 94 \0030,95 95 \0031,96 96 \0031,97 97 "
	       "\0031,98 98 \00399,99 99 ");
}

void
pb_other_colors(std::vector<std::string> &out)
{
	OUT_PB("\00316 16 \00317 17 \00318 18 \00319 19 \00320 20 "
	       "\00321 21 \00322 22 \00323 23 \00324 24 \00325 25 "
	       "\00326 26 \00327 27 ");
	OUT_PB("\00328 28 \00329 29 \00330 30 \00331 31 \00332 32 "
	       "\00333 33 \00334 34 \00335 35 \00336 36 \00337 37 "
	       "\00338 38 \00339 39 ");
	OUT_PB("\00340 40 \00341 41 \00342 42 \00343 43 \00344 44 "
	       "\00345 45 \00346 46 \00347 47 \00348 48 \00349 49 "
	       "\00350 50 \00351 51 ");
	OUT_PB("\00352 52 \00353 53 \00354 54 \00355 55 \00356 56 "
	       "\00357 57 \00358 58 \00359 59 \00360 60 \00361 61 "
	       "\00362 62 \00363 63 ");
	OUT_PB("\00364 64 \00365 65 \00366 66 \00367 67 \00368 68 "
	       "\00369 69 \00370 70 \00371 71 \00372 72 \00373 73 "
	       "\00374 74 \00375 75 ");
	OUT_PB("\00376 76 \00377 77 \00378 78 \00379 79 \00380 80 "
	       "\00381 81 \00382 82 \00383 83 \00384 84 \00385 85 "
	       "\00386 86 \00387 87 ");
	OUT_PB("\00388 88 \00389 89 \00390 90 \00391 91 \00392 92 "
	       "\00393 93 \00394 94 \00395 95 \00396 96 \00397 97 "
	       "\00398 98 \00399,99 99 ");
}

static void
print_function_key(PPRINTTEXT_CONTEXT ctx, const char *key, const char *desc)
{
	printtext(ctx, "  %c%-3s%c = %s", BOLD, key, NORMAL, desc);
}

/* usage: /colormap */
void
cmd_colormap(const char *data)
{
	PRINTTEXT_CONTEXT		ctx;
	std::vector<std::string>	out;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

	if (!strings_match(data, "")) {
		printtext(&ctx, "/colormap: implicit trailing data");
		return;
	}

	OUT_PB("");
	OUT_PB(TXT_BOLD "COLORMAP" TXT_BOLD);
	OUT_PB("");
	OUT_PB("\0031,0 00 \0030,1 01 \0030,2 02 \0030,3 03 "
	       "\0031,4 04 \0030,5 05 \0030,6 06 \0031,7 07 ");
	OUT_PB("\0031,8 08 \0031,9 09 \0030,10 10 \0031,11 11 "
	       "\0030,12 12 \0031,13 13 \0031,14 14 \0031,15 15 ");
	OUT_PB("");
#if BSD || LINUX
	pb_linux_colors(out);
#else
	pb_other_colors(out);
#endif
	OUT_PB("");

	ctx.spec_type = TYPE_SPEC1;

	for (const std::string &str : out)
		printtext(&ctx, "%s", str.c_str());

	print_function_key(&ctx, "F5", "Blink");
	print_function_key(&ctx, "F6", "Bold");
	print_function_key(&ctx, "F7", "Color");
	print_function_key(&ctx, "F8", "Normal");
	print_function_key(&ctx, "F9", "Reverse");
	print_function_key(&ctx, "F10", "Underline");

	printtext(&ctx, " ");
	printtext(&ctx, "COLORS:      %d", COLORS);
	printtext(&ctx, "COLOR_PAIRS: %d", COLOR_PAIRS);
	printtext(&ctx, "can_change_color: %s", (can_change_color() ? _("Yes")
	    : _("No")));
	printtext(&ctx, "g_initialized_pairs: %hd", g_initialized_pairs);
}
