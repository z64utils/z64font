/* <z64.me> z64font api */

#ifndef Z64FONT_H_INCLUDED
#define Z64FONT_H_INCLUDED

/* TODO make customizable so users can compile higher resolution
 * fonts without having to recompile the program from source
 */
#define  FONT_W 16
#define  FONT_H 16

#define  PROGNAME    "z64font"
#define  PROGVER     "v1.1.0"
#define  PROGATTRIB  "<z64.me>"
#define  PROG_NAME_VER_ATTRIB    PROGNAME" "PROGVER" "PROGATTRIB
#define  ZCHAR_MAX 4096   /* 4096 character slots is plenty */

#include "zchar.h"
#include "stb_truetype.h"

struct z64font
{
	void *ttfBin;
	unsigned ttfBinSz;
	char *chars;
	char* decompFileNames;
	stbtt_fontinfo font;
	int fontSize;
	int yshift;
	int xPad;
	int rightToLeft;
	int widthAdvance;
	int isDecompMode;
	struct zchar *zchar;
	unsigned zcharNum;
	char isI4;
	void (*info)(const char *fmt, ...);
	void (*error)(const char *fmt, ...);
};
#define Z64FONT_DEFAULTS { \
  .fontSize = 16 \
  , .zchar = wow_calloc_die(ZCHAR_MAX, sizeof(struct zchar)) \
  , .info = wow_stderr \
  , .error = wow_stderr \
}

int z64font_convert(struct z64font *g);
void z64font_exportBinaries(struct z64font *g, char **ofn);
void z64font_exportDecomp(struct z64font *g, char **ofn);
int z64font_loadFont(struct z64font *g, const char *fn);
int z64font_loadCodepoints(struct z64font *g, const char *fn);
int z64font_loadDecompFileNames(struct z64font *g, const char *fn);

#endif

