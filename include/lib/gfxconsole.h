#ifndef __LIB_GFXCONSOLE_H
#define __LIB_GFXCONSOLE_H

#include <lib/gfx.h>

void gfxconsole_start_on_display(void);
void gfxconsole_start(gfx_surface *surface);

void gfxconsole_clear(void);
void gfxconsole_setpos(unsigned x, unsigned y);
void gfxconsole_set_colors(uint32_t bg, uint32_t fg);
unsigned gfxconsole_getwidth();
unsigned gfxconsole_getheight();

void gfxconsole_settrans(unsigned val);

void gfxconsole_setbackground(gfx_surface *bgs);
#endif

