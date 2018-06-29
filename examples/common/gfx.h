#pragma once
/* 
    Common graphics functions for the chips-test example emulators.
*/
#define GFX_MAX_FB_WIDTH (1024)
#define GFX_MAX_FB_HEIGHT (1024)
extern void gfx_init(int w, int h);
extern void gfx_draw(void);
extern void gfx_shutdown(void);
extern uint32_t rgba8_buffer[GFX_MAX_FB_WIDTH * GFX_MAX_FB_HEIGHT];
