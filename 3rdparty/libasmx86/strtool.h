/* 
 *	HT Editor
 *	snprintf.h
 *
 *	Copyright (C) 1999-2003 Sebastian Biallas (sb@biallas.net)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SNPRINTF_H__
#define __SNPRINTF_H__

#include <stdarg.h>
#include "types.h"

int ht_snprintf(char *str, size_t count, const char *fmt, ...);
int ht_vsnprintf(char *str, size_t count, const char *fmt, va_list args);

size_t ht_strlcpy(char *s1, const char *s2, size_t maxlen);
int ht_strnicmp(const char *s1, const char *s2, size_t max);
int ht_vsnprintf (char *str, size_t count, const char *fmt, va_list args);
int ht_snprintf(char *str, size_t count, const char *fmt,...);
int hexdigit(char a);
bool str2int(const char *str, uint64 &u64, int defaultbase = 10);
int ht_strncmp(const char *s1, const char *s2, size_t max);
void whitespaces(const char *&str);
void non_whitespaces(const char *&str);
bool waitforchar(const char *&str, char b);
bool is_whitespace(char c);

#endif
