/* <z64.me> zchar utf8 codepoint interpreter */

#include <stdio.h>
#include <string.h>

#include "zchar.h"

const struct zchar *zchar_findCodepoint(
	const struct zchar *array
	, utf8_int32_t codepoint
)
{
	const struct zchar *z = array;
	
	/* find matching codepoint */
	for (z = array; z->bitmap; ++z)
	{
		if (z->codepoint == codepoint)
			return z;
	}
	return 0;
}

int zchar_parseCodepoints(
	const char *chars
	, struct zchar *arr
	, int arrMax
	, unsigned *num
)
{
	const char *next;
	const char *w;
	struct zchar *zchar;
	
	*num = 0;
	
	/* skip first line containing sample string */
	chars = utf8chr(chars, '\n') + 1;
	
	for (zchar = arr, next = 0, w = chars; *w; w = next)
	{
		utf8_int32_t codepoint;
		char c[16];
		int sz = utf8codepointcalcsize(w);
		
		/* skip newlines */
		while (*w == 0x0d || *w == 0x0a)
			++w;
		
		if (!*w)
			break;
		
		if (zchar - arr >= arrMax)
			return -1;
		
		/* isolate multibyte character to its own string */
		memcpy(c, w, sz);
		c[sz] = '\0';
		fprintf(stdout, "'%s' (%d)\n", c, sz);
		next = utf8codepoint(w, &codepoint);
		
		zchar->codepoint = codepoint;
		++zchar;
		++*num;
	}
	
	return 0;
}

