/* <z64.me> z64font's gui code lives here */

#ifdef Z64FONT_GUI
#include <wow.h>
#define WOW_GUI_IMPLEMENTATION
#include <wow_gui.h>

#include "z64font.h"

#include <errno.h>

#define  WINW 440
#define  WINH 440
#define  PREVIEW_X (16)      /* window coordinates at which    */
#define  PREVIEW_Y (128+24)  /* to display text preview        */

static void i8_to_rgba32(
	unsigned char *dst
	, unsigned char *src
	, int w
	, int h
)
{
	unsigned char *dstOld = dst;
	dst += (w * h * 4) - 4;
	src += (w * h) - 1;
	
	while (dst > dstOld)
	{
		dst[0] = dst[1] = dst[2] = dst[3] = *src;
		src -= 1;
		dst -= 4;
	}
}

static void bake(
	struct z64font *g
	, unsigned char *dst
	, int dstW
	, int dstH
)
{
	char *next;
	char *w;
	utf8_int32_t codepoint;
	int x = 0;
	int y = 0;
	int yadv = FONT_H;
	memset(dst, 0, dstW * dstH);
	struct zchar *zchar = g->zchar;
	char *str = g->chars;
	
	for (next = 0, w = str; *w; w = next)
	{
		const struct zchar *z;
		unsigned char *bmp;
		int k;
		int i;
		
		next = utf8codepoint(w, &codepoint);
		
		/* end of string */
		if (codepoint == 0x0d || codepoint == 0x0a || !codepoint)
			break;
		
		/* explicit newline escape sequence */
		if (codepoint == '\\' && *next == 'n')
		{
			x = 0;
			if (g->rightToLeft && x <= 0)
				x = dstW;
			y += yadv;
			++next;
			continue;
		}
		
		/* find matching codepoint */
		if (!(z = zchar_findCodepoint(zchar, codepoint)))
			continue;
		
		if (g->rightToLeft && x <= 0)
			x = dstW;
		
		bmp = z->bitmap;
		
		if (g->rightToLeft)
		{
			x -= z->width;
			if (x <= 0)
			{
				y += yadv;
				x = dstW - z->width;
			}
		}
		
		/* blend character bitmap into preview */
		for (k = 0; k < FONT_H; ++k)
		{
			for (i = 0; i < FONT_W; ++i)
			{
				int dstIdx = (dstW * (y + k)) + x + i;
				if (dstIdx < 0 || dstIdx >= dstW * dstH)
					continue;
				dst[dstIdx] |= bmp[FONT_W * k + i];
			}
		}
		
		if (!g->rightToLeft)
		{
			x += z->width;
		
			if (x >= dstW )
			{
				y += yadv;
				x = 0;
			}
		}
	}
		
	i8_to_rgba32(dst, dst, dstW, dstH);
}

int wow_main(argc, argv)
{
	wow_main_args(argc, argv);
	unsigned char *preview = 0;
	int previewW = WINW - (PREVIEW_X * 2);
	int previewH = WINH - (PREVIEW_Y + 16);
	struct z64font g = Z64FONT_DEFAULTS;
	g.info = wowGui_infof;
	g.error = wowGui_errorf;
	
	preview = wow_malloc_die(previewW * previewH * 4);
	
	wowGui_bind_init(PROGNAME, WINW, WINH);
	
	wow_windowicon(1);
	
	while (1)
	{
		int changed = 0;
		/* wowGui_frame() must be called before you do any input */
		wowGui_frame();
		
		/* events */
		wowGui_bind_events();
		
		if (!wowGui_bind_should_redraw())
			goto skipRedraw;
		
		/* draw */
		wowGui_viewport(0, 0, WINW, WINH);
		wowGui_padding(8, 8);
		
		static struct wowGui_window win = {
			.rect = {
				.x = 0
				, .y = 0
				, .w = WINW
				, .h = WINH
			}
			, .color = 0x301818FF
			, .not_scrollable = 1
			, .scroll_valuebox = 1
		};
		if (wowGui_window(&win))
		{
			wowGui_row_height(20);
			wowGui_columns(2);

			wowGui_column_width(WINW / 2);
			wowGui_italic(2);
			wowGui_label(PROG_NAME_VER_ATTRIB);
			wowGui_italic(0);

			wowGui_checkbox("decompMode", &g.isDecompMode);

			/* file droppers */
			wowGui_columns(3);
			static struct wowGui_fileDropper ttfFile = {
				.label = "TrueType Font  (.ttf)"
				, .labelWidth = 180
				, .filenameWidth = 200
				, .extension = "ttf"
			};
			if (wowGui_fileDropper(&ttfFile))
			{
				if (z64font_loadFont(&g, ttfFile.filename))
				{
					free(ttfFile.filename);
					ttfFile.filename = "";
				}
				changed = 1;
			}
			static struct wowGui_fileDropper txtFile = {
				.label = "Codepoint File (.txt)"
				, .labelWidth = 180
				, .filenameWidth = 200
				, .extension = "txt"
			};
			if (wowGui_fileDropper(&txtFile))
			{
				if (z64font_loadCodepoints(&g, txtFile.filename))
				{
					free(txtFile.filename);
					txtFile.filename = "";
				}
				changed = 1;
			}
			
			static struct wowGui_fileDropper decompFileNamesFile = {
				.label = "Decomp Names (.txt)"
				, .labelWidth = 180
				, .filenameWidth = 200
				, .extension = "txt"
			};
			if (g.isDecompMode)
			{
				if (wowGui_fileDropper(&decompFileNamesFile))
				{
					if (z64font_loadDecompFileNames(&g, decompFileNamesFile.filename))
					{
						free(decompFileNamesFile.filename);
						decompFileNamesFile.filename = "";
					}
					changed = 1;
				}
			}
			
			wowGui_column_width(64);
			wowGui_columns(6);
			{
				int ok = sizeof(g);
				static void *x = 0;
				if (!x)
				{
					x = wow_malloc_die(ok);
				}
				memcpy(x, &g, ok);
				wowGui_label("Y Shift");
				wowGui_int_range(&g.yshift, -16, 16, 1);
				wowGui_label("Size");
				wowGui_int_range(&g.fontSize, 1, 256, 1);
				wowGui_label("xPad");
				wowGui_int_range(&g.xPad, -16, 16, 1);
				
				wowGui_columns(3);
				wowGui_column_width(128+8);
				wowGui_checkbox("rightToLeft", &g.rightToLeft);
				wowGui_checkbox("widthAdvance", &g.widthAdvance);
				
				if (memcmp(x, &g, ok))
					changed = 1;
			}
			
			int previewOn = (
				!wowGui_fileDropper_filenameIsEmpty(&ttfFile)
				&& !wowGui_fileDropper_filenameIsEmpty(&txtFile)
			);
			
			wowGui_column_width(128 + 8);
			
			if (wowGui_button(g.isDecompMode ? "Export Decomp" : "Export Binaries"))
			{
				if (previewOn)
				{
					char *ofn = 0;
					ofn = wowGui_askFilename("font_static,h,png", 0, 1);
					if (ofn)
					{
						if (g.isDecompMode)
							z64font_exportDecomp(&g, &ofn);
						else
							z64font_exportBinaries(&g, &ofn);
						free(ofn);
					}
				}
			}
			
			/* display preview */
			if (previewOn)
			{
				/* update preview */
				if (changed)
				{
					/* (re)convert font */
					if (z64font_convert(&g))
						wowGui_dief("something went wrong");
					
					/* bake new preview */
					bake(&g, preview, previewW, previewH);
				}
				wowGui_label("preview:");
				wowGui_bind_blit_raw(
					preview
					, PREVIEW_X
					, PREVIEW_Y
					, previewW
					, previewH
					, 0 /* no blending */
				);
			}
			
			wowGui_column_width(100);
			
			wowGui_window_end();
		}
		
	skipRedraw:
		
		wowGui_frame_end(wowGui_bind_ms());
		
		/* display */
		wowGui_bind_result();
		
		/* loop exit condition */
		if (wowGui_bind_endmainloop())
			break;
	}
	
	free(g.zchar);
	
	wowGui_bind_quit();
	
	return 0;
	(void)argv;
}
#endif /* Z64FONT_GUI */

