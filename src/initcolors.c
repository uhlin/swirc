/* initcolors.c
   Copyright (C) 2020 Markus Uhlin. All rights reserved.

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
#include "errHand.h"
#include "initcolors.h"

static struct {
    short int	num;
    rgb_t	val;
} ext_colors_rgb[] = {
    /* 16-27 */
    { 52,  {278,0,0} },
    { 94,  {278,129,0} },
    { 100, {278,278,0} },
    { 58,  {196,278,0} },
    { 22,  {0,278,0} },
    { 29,  {0,278,173} },
    { 23,  {0,278,278} },
    { 24,  {0,153,278} },
    { 17,  {0,0,278} },
    { 54,  {180,0,278} },
    { 53,  {278,0,278} },
    { 89,  {278,0,165} },

    /* 28-39 */
    { 88,  {455,0,0} },
    { 130, {455,227,0} },
    { 142, {455,455,0} },
    { 64,  {318,455,0} },
    { 28,  {0,455,0} },
    { 35,  {0,455,286} },
    { 30,  {0,455,455} },
    { 25,  {0,251,455} },
    { 18,  {0,0,455} },
    { 91,  {294,0,455} },
    { 90,  {455,0,455} },
    { 125, {455,0,271} },

    /* 40-51 */
    { 124, {710,0,0} },
    { 166, {710,388,0} },
    { 184, {710,710,0} },
    { 106, {490,710,0} },
    { 34,  {0,710,0} },
    { 49,  {0,710,443} },
    { 37,  {0,710,710} },
    { 33,  {0,388,710} },
    { 19,  {0,0,710} },
    { 129, {459,0,710} },
    { 127, {710,0,710} },
    { 161, {710,0,420} },

    /* 52-63 */
    { 196, {1000,0,0} },
    { 208, {1000,549,0} },
    { 226, {1000,1000,0} },
    { 154, {698,1000,0} },
    { 46,  {0,1000,0} },
    { 86,  {0,1000,627} },
    { 51,  {0,1000,1000} },
    { 75,  {0,549,1000} },
    { 21,  {0,0,1000} },
    { 171, {647,0,1000} },
    { 201, {1000,0,1000} },
    { 198, {1000,0,596} },

    /* 64-75 */
    { 203, {1000,349,349} },
    { 215, {1000,706,349} },
    { 227, {1000,1000,443} },
    { 191, {812,1000,376} },
    { 83,  {435,1000,435} },
    { 122, {396,1000,788} },
    { 87,  {427,1000,1000} },
    { 111, {349,706,1000} },
    { 63,  {349,349,1000} },
    { 177, {769,349,1000} },
    { 207, {1000,400,1000} },
    { 205, {1000,349,737} },

    /* 76-87 */
    { 217, {1000,612,612} },
    { 223, {1000,827,612} },
    { 229, {1000,1000,612} },
    { 193, {886,1000,612} },
    { 157, {612,1000,612} },
    { 158, {612,1000,859} },
    { 159, {612,1000,1000} },
    { 153, {612,827,1000} },
    { 147, {612,612,1000} },
    { 183, {863,612,1000} },
    { 219, {1000,612,1000} },
    { 212, {1000,580,827} },

    /* 88-98 */
    { 16,  {0,0,0} },
    { 233, {75,75,75} },
    { 235, {157,157,157} },
    { 237, {212,212,212} },
    { 239, {302,302,302} },
    { 241, {396,396,396} },
    { 244, {506,506,506} },
    { 247, {624,624,624} },
    { 250, {737,737,737} },
    { 254, {886,886,886} },
    { 231, {1000,1000,1000} },
};

void
initcolors(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(ext_colors_rgb); i++) {
	const rgb_t val = ext_colors_rgb[i].val;
	if (init_color(ext_colors_rgb[i].num, val.r,val.g,val.b) == ERR) {
	    debug("initcolors: init_color: cannot initialize color %hd",
		ext_colors_rgb[i].num);
	}
    }
}
