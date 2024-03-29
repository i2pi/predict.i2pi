/* csv - read write comma separated value format
 * Copyright (c) 2003 Michael B. Allen <mba2000 ioplex.com>
 *
 * The MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <wchar.h>
#include <wctype.h>

#include "mba_csv.h"

#define ST_START     1
#define ST_COLLECT   2
#define ST_TAILSPACE 3
#define ST_END_QUOTE 4

struct sinput {
	FILE *in;
	const unsigned char *src;
	size_t sn;
	size_t count;
};
struct winput {
	const wchar_t *src;
	size_t sn;
	size_t count;
};

static int
snextch(struct sinput *in)
{
	int ch;

	if (in->in) {
		if ((ch = fgetc(in->in)) == EOF) {
			if (ferror(in->in)) {
				return -1;
			}
			return 0;
		}
	} else {
		if (in->sn == 0) {
			return 0;
		}
		ch = *(in->src)++;
		in->sn--;
	}
	in->count++;

	if (ch == '\r') ch = ' ';

	return ch;
}
static int
wnextch(struct winput *in)
{
	int ch;

	if (in->sn == 0) {
		return 0;
	}
	ch = *(in->src)++;
	in->sn--;
	in->count++;

	return ch;
}

static int
csv_parse_str(struct sinput *in,
			unsigned char *buf,
			size_t bn,
			unsigned char *row[],
			int rn,
			int sep,
			int flags,
			int	*cols)
{
	int trim, quotes, ch, state, r, j, t, inquotes;

	trim = flags & CSV_TRIM;
	quotes = flags & CSV_QUOTES;
	state = ST_START;
	inquotes = 0;
	ch = r = j = t = 0;

	memset(row, 0, sizeof(unsigned char *) * rn);

	while (rn && bn && (ch = snextch(in)) > 0) {
		switch (state) {
			case ST_START:
				if (ch != '\n' && ch != sep && isspace(ch)) {
					if (!trim) {
						buf[j++] = ch; bn--;
						t = j;
					}
					break;
				} else if (quotes && ch == '"') {
					j = t = 0;
					state = ST_COLLECT;
					inquotes = 1;
					break;
				}
				state = ST_COLLECT;
			case ST_COLLECT:
				if (inquotes) {
					if (ch == '"') {
						state = ST_END_QUOTE;
						break;
					}
				} else if (ch == sep || ch == '\n') {
					row[r++] = buf; rn--;
					*cols = r;
					if (ch == '\n' && t && buf[t - 1] == '\r') {
						t--; bn++; /* crlf -> lf */
					}
					buf[t] = '\0'; bn--;
					buf += t + 1;
					j = t = 0;
					state = ST_START;
					inquotes = 0;
					if (ch == '\n') {
						rn = 0;
					}
					break;
				} else if (quotes && ch == '"') {
					fprintf (stderr, "CSV : unexpected quote in element %d\n", (r + 1));
					return -1;
				}
				buf[j++] = ch; bn--;
				if (!trim || isspace(ch) == 0) {
					t = j;
				}
				break;
			case ST_TAILSPACE:
			case ST_END_QUOTE:
				if (ch == sep || ch == '\n') {
					row[r++] = buf; rn--;
					*cols = r;
					buf[j] = '\0'; bn--;
					buf += j + 1;
					j = t =  0;
					state = ST_START;
					inquotes = 0;
					if (ch == '\n') {
						rn = 0;
					}
					break;
				} else if (quotes && ch == '"' && state != ST_TAILSPACE) {
					buf[j++] = '"';	bn--;		 /* nope, just an escaped quote */
					t = j;
					state = ST_COLLECT;
					break;
				} else if (isspace(ch)) {
					state = ST_TAILSPACE;
					break;
				}
				errno = EILSEQ;
				fprintf (stderr, "CSV: bad end quote in element %d\n", (r + 1));
				return -1;
		}
	}
	if (ch == -1) {
		return -1;
	}
	if (bn == 0) {
		return -1;
	}
	if (rn) {
		if (inquotes && state != ST_END_QUOTE) {
			return -1;
		}
		row[r] = buf;
		buf[t] = '\0';
	}

	return in->count;
}
static int
csv_parse_wcs(struct winput *in, wchar_t *buf, size_t bn, wchar_t *row[], int rn, wint_t sep, int flags, int *cols)
{
	int trim, quotes, state, r, j, t, inquotes;
	wint_t ch;

	trim = flags & CSV_TRIM;
	quotes = flags & CSV_QUOTES;
	state = ST_START;
	inquotes = 0;
	ch = r = j = t = 0;

	memset(row, 0, sizeof(wchar_t *) * rn);

	while (rn && bn && (ch = wnextch(in)) > 0) {
		switch (state) {
			case ST_START:
				if (ch != L'\n' && ch != sep && iswspace(ch)) {
					if (!trim) {
						buf[j++] = ch; bn--;
						t = j;
					}
					break;
				} else if (quotes && ch == L'"') {
					j = t = 0;
					state = ST_COLLECT;
					inquotes = 1;
					break;
				}
				state = ST_COLLECT;
			case ST_COLLECT:
				if (inquotes) {
					if (ch == L'"') {
						state = ST_END_QUOTE;
						break;
					}
				} else if (ch == sep || ch == L'\n') {
					row[r++] = buf; rn--;
					*cols = r;
					buf[t] = L'\0'; bn--;
					buf += t + 1;
					j = t = 0;
					state = ST_START;
					inquotes = 0;
					if (ch == L'\n') {
						rn = 0;
					}
					break;
				} else if (quotes && ch == L'"') {
					fprintf (stderr, "CSV : unexpected quote in element %d\n", (r + 1));
					return -1;
				}
				buf[j++] = ch; bn--;
				if (!trim || iswspace(ch) == 0) {
					t = j;
				}
				break;
			case ST_TAILSPACE:
			case ST_END_QUOTE:
				if (ch == sep || ch == L'\n') {
					row[r++] = buf; rn--;
					*cols = r;
					buf[j] = L'\0'; bn--;
					buf += j + 1;
					j = t =  0;
					state = ST_START;
					inquotes = 0;
					if (ch == L'\n') {
						rn = 0;
					}
					break;
				} else if (quotes && ch == L'"' && state != ST_TAILSPACE) {
					buf[j++] = L'"'; bn--;		 /* nope, just an escaped quote */
					t = j;
					state = ST_COLLECT;
					break;
				} else if (iswspace(ch)) {
					state = ST_TAILSPACE;
					break;
				}
				fprintf (stderr, "CSV : bad end quote in element %d\n", (r + 1));
				return -1;
		}
	}
	if (ch == (wint_t)-1) {
		return -1;
	}
	if (bn == 0) {
		return -1;
	}
	if (rn) {
		if (inquotes && state != ST_END_QUOTE) {
			return -1;
		}
		row[r] = buf;
		buf[t] = L'\0';
	}

	return in->count;
}
int
csv_row_parse_wcs(const wchar_t *src, size_t sn, wchar_t *buf, size_t bn, wchar_t *row[], int rn, int sep, int trim, int *cols)
{
	struct winput input;
	input.src = src;
	input.sn = sn;
	input.count = 0;
	return csv_parse_wcs(&input, buf, bn, row, rn, (wint_t)sep, trim, cols);
}
int
csv_row_parse_str(const unsigned char *src, size_t sn, unsigned char *buf, size_t bn, unsigned char *row[], int rn, int sep, int trim, int *cols)
{
	struct sinput input;
	input.in = NULL;
	input.src = src;
	input.sn = sn;
	input.count = 0;
	return csv_parse_str(&input, buf, bn, row, rn, sep, trim, cols);
}
int
csv_row_fread(FILE *in, unsigned char *buf, size_t bn, unsigned char *row[], int numcols, int sep, int trim, int *cols)
{
	struct sinput input;
	input.in = in;
	input.count = 0;
	return csv_parse_str(&input, buf, bn, row, numcols, sep, trim, cols);
}

