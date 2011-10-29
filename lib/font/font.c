/*
 * Copyright (c) 2008-2010 Travis Geiselbrecht
 *
 * Copyright (c) 2011 James Sullins
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file
 * @brief  Font display
 *
 * This file contains functions to render fonts onto the graphics drawing
 * surface.
 *
 * @ingroup graphics
 */

#include <debug.h>
#include <lib/gfx.h>
#include <lib/font.h>

#include "fonts.h"

/**
 * @brief Draw one character from the built-in font
 *
 * @ingroup graphics
 */
void font_draw_char(gfx_surface *surface, unsigned char c, int x, int y, uint32_t fg, uint32_t bg)
{
	uint i,j;

	for (i = 0; i < FONT_Y; i++) {
		for (j = 0; j < FONT_X; j++) {
			if (FONT[(c - 32) * FONT_Y * FONT_COLS + i * FONT_COLS + j / 8] & ((1<<7) >> (j % 8))) {
				if (surface->rotation == 1) {
					gfx_putpixel(surface, (y + i), 
							surface->height - (x + j) - 1, fg);
				} else {
					gfx_putpixel(surface, x + j, y + i, fg);
				}
			} else {
				if (surface->rotation == 1) {
					gfx_putpixel(surface, (y + i), 
							surface->height - (x + j) - 1, bg);
				} else {
					gfx_putpixel(surface, x + j, y + i, bg);
				}
			}
		}
	}

	if (surface->rotation == 1) {
		gfx_flush_rows(surface, x, x + FONT_X);
	} else {
		gfx_flush_rows(surface, y, y + FONT_Y);
	}
}


void font_draw_char_trans(gfx_surface *surface, unsigned char c, int x, int y, uint32_t fg, gfx_surface *bg_surface)
{
	uint i,j;

	if (surface->rotation == 1) {
		gfx_surface_blend_rect(surface, bg_surface, y, 
				surface->height - x - FONT_X, y,
				surface->height - x - FONT_X, FONT_Y, FONT_X);
	} else {
		gfx_surface_blend_rect(surface, bg_surface, x, y, x, y, FONT_X, FONT_Y);
	}

	for (i = 0; i < FONT_Y; i++) {
		for (j = 0; j < FONT_X; j++) {
			if (FONT[(c - 32) * FONT_Y * FONT_COLS + i * FONT_COLS + j / 8] & ((1<<7) >> (j % 8))) {
				if (surface->rotation == 1) {
					gfx_putpixel(surface, (y + i), 
							surface->height - (x + j) - 1, fg);
				} else {
					gfx_putpixel(surface, x + j, y + i, fg);
				}
			}
		}
	}

	if (surface->rotation == 1) {
		gfx_flush_rows(surface, x, x + FONT_X);
	} else {
		gfx_flush_rows(surface, y, y + FONT_Y);
	}
}


