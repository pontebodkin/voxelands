/************************************************************************
* crypto.c
* tausaga - environmental survival game
* Copyright (C) Lisa 'darkrose' Milne 2016 <lisa@ltmnet.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
************************************************************************/

#include "crypto.h"

#include <stdlib.h>
#include <string.h>

/* defined in base64.c */
int base64_lencode(char *source, size_t sourcelen, char *target, size_t targetlen);
size_t base64_ldecode(char *source, char *target, size_t targetlen);

uint32_t hash(char *str_param)
{
	uint32_t hval = 0;
	uint32_t g;
	const char *s = str_param;

	while (*s) {
		hval <<= 4;
		hval += (unsigned char) *s++;
		g = hval & ((uint32_t) 0xf << 28);
		if (g != 0) {
			hval ^= g >> 24;
			hval ^= g;
		}
	}

	return hval;
}

/* base64 encode a string */
char* base64_encode(char* str)
{
	int sl;
	int tl;
	char* ret;
	int el;
	if (!str)
		return NULL;

	sl = strlen(str);
	tl = ((sl+2)/3*4)+5;

	ret = malloc(tl);
	if (!ret)
		return NULL;

	el = base64_lencode(str, sl, ret, tl);
	if (!el) {
		free(ret);
		return NULL;
	}

	return ret;
}

/* decode a base64 string */
char* base64_decode(char* str)
{
	int sl;
	int tl;
	char* ret;
	int dl;
	if (!str)
		return NULL;

	sl = strlen(str);
	tl = (sl/4*3)+5;
	ret = malloc(tl);
	if (!ret)
		return NULL;

	dl = base64_ldecode(str, ret, tl);
	if (!dl) {
		free(ret);
		return NULL;
	}
	ret[dl] = 0;

	return ret;
}
