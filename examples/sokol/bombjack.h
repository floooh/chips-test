#pragma once
/*
    bombjack.h -- Bomb Jack emulator defines and structs
*/

#if defined(__cplusplus)
extern "C" {
#endif

#define DISPLAY_WIDTH (256)
#define DISPLAY_HEIGHT (256)

#define VSYNC_PERIOD_4MHZ (4000000 / 60)
#define VBLANK_DURATION_4MHZ (((4000000 / 60) / 525) * (525 - 483))
#define VSYNC_PERIOD_3MHZ (3000000 / 60)

#define NUM_AUDIO_SAMPLES (128)

void bombjack_init(void);

/* the Bomb Jack arcade machine is 2 separate computers, the main board and sound board */
typedef struct {
    z80_t cpu;
    clk_t clk;
    uint8_t p1;             /* joystick 1 state */
    uint8_t p2;             /* joystick 2 state */
    uint8_t sys;            /* coins and start buttons */
    uint8_t dsw1;           /* dip-switches 1 */
    uint8_t dsw2;           /* dip-switches 2 */
    uint8_t nmi_mask;       /* if 0, no NMIs are generated */
    uint8_t bg_image;       /* current background image */
    int vsync_count;
    int vblank_count;
    mem_t mem;
} mainboard_t;

typedef struct {
    z80_t cpu;
    clk_t clk;
    ay38910_t psg[3];
    uint32_t tick_count;
    int vsync_count;
    mem_t mem;
} soundboard_t;

typedef struct {
    mainboard_t main;
    soundboard_t sound;
    uint8_t sound_latch;            /* shared latch, written by main board, read by sound board */
    uint8_t main_ram[0x1C00];
    uint8_t sound_ram[0x0400];
    uint8_t rom_chars[0x3000];
    uint8_t rom_tiles[0x6000];
    uint8_t rom_sprites[0x6000];
    uint8_t rom_maps[0x1000];
    /* small intermediate buffer for generated audio samples */
    int sample_pos;
    float sample_buffer[NUM_AUDIO_SAMPLES];
    /* 32-bit RGBA color palette cache */
    uint32_t palette_cache[128];
} bombjack_t;

#if defined(__cplusplus)
} // extern "C"
#endif
