#pragma once
/* 
    Common graphics functions for the chips-test example emulators.
*/
#define GFX_MAX_FB_WIDTH (1024)
#define GFX_MAX_FB_HEIGHT (1024)
extern void gfx_init(int fb_width, int fb_height, int aspect_scale_x, int aspect_scale_y);
extern uint32_t* gfx_framebuffer(void);
extern int gfx_framebuffer_size(void);
extern void gfx_draw(void);
extern void gfx_shutdown(void);
