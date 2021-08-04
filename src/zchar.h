/* <z64.me> zchar utf8 codepoint interpreter */

#ifndef Z64_ZCHAR_H_INCLUDED

#include "utf8.h"

struct zchar
{
	utf8_int32_t codepoint;
	void *bitmap; /* bitmap in i8 format */
	float width;
};

const struct zchar *zchar_findCodepoint(
	const struct zchar *array
	, utf8_int32_t codepoint
);

int zchar_parseCodepoints(
	const char *chars
	, struct zchar *arr
	, int arrMax
	, unsigned *num
);

#endif

