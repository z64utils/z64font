/*
 * z64font <z64.me>
 *
 * converts truetype font files to oot/mm format
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdarg.h>

#define WOW_IMPLEMENTATION
#include <wow.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_TRUETYPE_IMPLEMENTATION

#include "z64font.h"

/*
 *
 * private functions
 *
 */

/* compose codepoint in i8 color space */
static void compose(
	int x
	, int y
	, int w
	, int h
	, uint8_t *src
	, uint8_t *dst
	, int yshift
)
{
	int i;
	int k;
	
	memset(dst, 0, FONT_W * FONT_H);
	
	for (k = 0; k < h; ++k)
	{
		for (i = 0; i < w; ++i)
		{
			int dstIdx = ((k + y) + yshift) * FONT_W + i + x;
			
			/* skip anything out of bounds */
			if (i + x < 0 || dstIdx < 0 || dstIdx >= FONT_W * FONT_H)
				continue;
			dst[dstIdx] = src[k * w + i];
			//fputc(" .:ioVM@"[src[k*w+i]>>5], stderr);
		}
		//fputc('\n', stderr);
	}
}

static void i8_to_i4(uint8_t *dst, uint8_t *src, int w, int h)
{
	/* color points to last color */
	int alt = 0;
	int i;
	
	for (i = 0; i < w * h; ++i, ++src)
	{
		uint8_t *b = dst;
		uint8_t  c;
		float f;
		
		c = *b;
		b = &c;
		
		/* convert pixel */
		f = 0.003921569f * *src;
		*b = roundf(f * 15);
		
		/* clear when we initially find it */
		if (!(alt&1))
			*dst = 0;
		
		/* how to transform result */
		if (!(alt&1))
			c <<= 4;
		else
			c &= 15;
		
		*dst |= c;
		
		/* pixel advancement logic */
		dst += ((alt++)&1);
	}
}

static void *quickWidth(float width)
{
	static uint8_t arr[4];
	unsigned int widthU32;
	widthU32 = *((unsigned int*)&width);
	arr[0] = widthU32 >> 24;
	arr[1] = widthU32 >> 16;
	arr[2] = widthU32 >> 8;
	arr[3] = widthU32;
	return arr;
}

/* read file from drive; returns 0 on failure */
static void *readFile(struct z64font *g, const char *fn, unsigned *sz)
{
	void *data;
	unsigned sz_;
	if (!sz)
		sz = &sz_;
	FILE *fp = fopen(fn, "rb");
	if (!fp)
	{
		g->error("failed to open '%s' for reading", fn);
		return 0;
	}
	
	/* get file size */
	fseek(fp, 0, SEEK_END);
	*sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	/* allocate memory to store file */
	data = malloc(*sz+1);
	if (!data)
	{
		fclose(fp);
		return 0;
	}
	((char*)data)[*sz] = '\0'; /* in case used as string */
	
	/* doing it in a single read failed... */
	if (fread(data, 1, *sz, fp) != *sz)
	{
		fclose(fp);
		free(data);
		g->error("failed to read file '%s'", fn);
		return 0;
	}
	
	fclose(fp);
	return data;
}

/* read file from drive as string; returns 0 on failure */
static char *readFileString(struct z64font *g, const char *fn)
{
	char *data;
	unsigned dataSz;
	data = readFile(g, fn, &dataSz);
	if (!data)
		return 0;
	data[dataSz] = '\0';
	return data;
}

/*
 *
 * public api
 *
 */


void z64font_exportDecomp(struct z64font *g, char **ofn)
{
	FILE *fp = 0;
	struct zchar *zchar;
	const char *delim = "\r\n";
	char *pngFn = 0;
	char *decompFileNames = strdup(g->decompFileNames);
	const float extraWidthEntries[] = {
		14.0f, // '[A]'
		14.0f, // '[B]'
		14.0f, // '[C]'
		14.0f, // '[L]'
		14.0f, // '[R]'
		14.0f, // '[Z]'
		14.0f, // '[C-Up]'
		14.0f, // '[C-Down]'
		14.0f, // '[C-Left]'
		14.0f, // '[C-Right]'
		14.0f, // '▼'
		14.0f, // '[Control-Pad]'
		14.0f, // '[D-Pad]'
		14.0f, // ?
		14.0f, // ?
		14.0f, // ?
		14.0f, // ?
	};
	
	if (!ofn || !*ofn || !decompFileNames)
		return;

	pngFn = strtok(decompFileNames, delim);

	for (zchar = g->zchar; zchar->bitmap; ++zchar)
	{
		stbi_write_png(pngFn, FONT_W, FONT_H, 1, zchar->bitmap, FONT_W * 1);
		pngFn = strtok(0, delim);
	}

	g->isI4 = 1; /* has converted to i4 */
	
	/* export 'comic-sans.font_width.h' */
	fp = fopen(*ofn, "w");
	if (!fp)
	{
		g->error("failed to open '%s' for writing\n", *ofn);
		goto L_cleanup;
	}
	for (zchar = g->zchar; zchar->bitmap; ++zchar)
	{
		if (fprintf(fp, "%ff,\n", zchar->width) < 0)
		{
			g->error("failed to write '%s'\n", *ofn);
			goto L_cleanup;
		}
	}
	for (int i = 0; i < sizeof(extraWidthEntries) / sizeof(extraWidthEntries[0]); ++i)
	{
		if (fprintf(fp, "%ff,\n", extraWidthEntries[i]) < 0)
		{
			g->error("failed to write '%s'\n", *ofn);
			goto L_cleanup;
		}
	}
	fclose(fp);
	fp = 0;
	g->info("Export successful!\n");
L_cleanup:
	free(decompFileNames);
	if (fp)
		fclose(fp);
}

void z64font_exportBinaries(struct z64font *g, char **ofn)
{
	FILE *fp = 0;
	struct zchar *zchar;
	
	if (!ofn || !*ofn)
		return;
	
	/* export 'comic-sans.font_static' */
	if (wow_fnChangeExtension(ofn, "font_static"))
	{
		g->error("memory error");
		goto L_cleanup;
	}
	fp = fopen(*ofn, "wb");
	if (!fp)
	{
		g->error("failed to open '%s' for writing\n", *ofn);
		goto L_cleanup;
	}
	for (zchar = g->zchar; zchar->bitmap; ++zchar)
	{
		if (!g->isI4)
			i8_to_i4(zchar->bitmap, zchar->bitmap, FONT_W, FONT_H);
		if (fwrite(zchar->bitmap, 1, (FONT_W * FONT_H) / 2, fp)
			!= (FONT_W * FONT_H) / 2
		)
		{
			g->error("failed to write '%s'\n", *ofn);
			goto L_cleanup;
		}
	}
	fclose(fp);
	fp = 0;
	g->isI4 = 1; /* has converted to i4 */
	
	/* export 'comic-sans.width_table' */
	if (wow_fnChangeExtension(ofn, "width_table"))
	{
		g->error("memory error");
		goto L_cleanup;
	}
	fp = fopen(*ofn, "wb");
	if (!fp)
	{
		g->error("failed to open '%s' for writing\n", *ofn);
		goto L_cleanup;
	}
	for (zchar = g->zchar; zchar->bitmap; ++zchar)
	{
		if (fwrite(quickWidth(zchar->width), 1, 4, fp) != 4)
		{
			g->error("failed to write '%s'\n", *ofn);
			goto L_cleanup;
		}
	}
	fclose(fp);
	fp = 0;
	g->info("Export successful!\n");
L_cleanup:
	if (fp)
		fclose(fp);
}


int z64font_convert(struct z64font *g)
{
	float scale;
	int ascent;
	int baseline;
	uint8_t *bitmap;
	struct zchar *zchar;
	const char *chars = g->chars;
	stbtt_fontinfo *font = &g->font;
	int yshift = g->yshift;
	int fontSize = g->fontSize;
	int xPad = g->xPad;
	int widthAdvance = g->widthAdvance;
	struct zchar *arr = g->zchar;
	int arrMax = ZCHAR_MAX;
	
	g->isI4 = 0;
	
	scale = stbtt_ScaleForPixelHeight(font, fontSize);
	stbtt_GetFontVMetrics(font, &ascent, 0, 0);
	baseline = scale * ascent;
	
	/* get codepoints */
	if (zchar_parseCodepoints(chars, arr, arrMax, &g->zcharNum))
	{
		g->error("too many codepoints detected");
		return -1;
	}
	
	/* prepare each codepoint's graphic */
	for (zchar = arr; zchar < arr + g->zcharNum; ++zchar)
	{
		int width;
		int height;
		int xofs;
		int yofs;
		utf8_int32_t codepoint = zchar->codepoint;
		uint8_t conv[FONT_W * FONT_H];
		int advance;
		int lsb;
		
		bitmap = stbtt_GetCodepointBitmap(
			font
			, 0
			, scale
			, codepoint
			, &width
			, &height
			, &xofs
			, &yofs
		);
		
		//fprintf(stderr, "xofs yofs %d %d\n", xofs, yofs);
		//fprintf(stderr, "ascent = %d\n", ascent);
		//fprintf(stderr, "baseline = %d\n", baseline);
		
		compose(xofs, baseline + yofs, width, height, bitmap, conv, yshift);
		stbtt_GetCodepointHMetrics(font, codepoint, &advance, &lsb);
		
		if (widthAdvance)
			width = advance * scale;
		else
			width = fmax(width, advance * scale);
		
		width += xPad;
		
		zchar->codepoint = codepoint;
		zchar->width = width;
		if (!zchar->bitmap)
		{
			zchar->bitmap = wow_malloc_die(sizeof(conv));
		}
		memcpy(zchar->bitmap, conv, sizeof(conv));
		
		free(bitmap);
	}
	
	return 0;
}

int z64font_loadFont(struct z64font *g, const char *fn)
{
	/* ttf changed */
	if (g->ttfBin)
		free(g->ttfBin);
	
	if (!fn || !strlen(fn))
		return 1;
	
	if (!(g->ttfBin = readFile(g, fn, &g->ttfBinSz)))
		return 1;
	
	if (!stbtt_InitFont(&g->font, g->ttfBin, 0))
	{
		free(g->ttfBin);
		g->ttfBin = 0;
		g->error("unsupported font file '%s'", fn);
		return 1;
	}
	return 0;
}

int z64font_loadCodepoints(struct z64font *g, const char *fn)
{
	/* txt changed */
	if (g->chars)
		free(g->chars);
	
	if (!fn || !strlen(fn))
		return 1;
	
	/* read characters as string */
	if (!(g->chars = readFileString(g, fn)))
		return 1;
	
	if (utf8valid(g->chars))
	{
		g->error("'%s' contains invalid codepoint(s)", fn);
		return 1;
	}
	
	return 0;
}

int z64font_loadDecompFileNames(struct z64font *g, const char *fn)
{
	/* txt changed */
	if (g->decompFileNames)
		free(g->decompFileNames);
	
	if (!fn || !strlen(fn))
		return 1;
	
	/* read characters as string */
	if (!(g->decompFileNames = readFileString(g, fn)))
		return 1;

	return 0;
}
