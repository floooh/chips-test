/*
    bombjack.c

    Bomb Jack arcade machine emulation (MAME used as reference).
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "bombjack-roms.h"

static void app_init(void);
static void app_frame(void);
static void app_input(const sapp_event*);
static void app_cleanup(void);

static void bombjack_init(void);
static void bombjack_exec(uint32_t micro_seconds);
static uint64_t bombjack_tick_main(int num, uint64_t pins, void* user_data);
static uint64_t bombjack_tick_sound(int num, uint64_t pins, void* user_data);
static void bombjack_ay_out(int port_id, uint8_t data, void* user_data);
static uint8_t bombjack_ay_in(int port_id, void* user_data);
static void bombjack_decode_video(void);

#define DISPLAY_WIDTH (256)
#define DISPLAY_HEIGHT (256)

#define VSYNC_PERIOD (4000000 / 60)

/* the Bomb Jack arcade machine is actually 2 computers, the main board and sound board */
typedef struct {
    z80_t cpu;
    clk_t clk;
    uint8_t p1;             /* joystick 1 state */
    uint8_t p2;             /* joystick 2 state */
    uint8_t sys;            /* coins and start buttons */
    uint8_t dsw1;           /* dip-switches 1 */
    uint8_t dsw2;           /* dip-switches 2 */
    uint8_t nmi_mask;
    uint32_t vsync_count;
    uint32_t palette[128];
    mem_t mem;
    uint8_t ram[0x2000];
} mainboard_t;

typedef struct {
    z80_t cpu;
    clk_t clk;
    ay38910_t ay[3];
    mem_t mem;
    uint8_t ram[0x0400];
} soundboard_t;

typedef struct {
    mainboard_t main;
    soundboard_t sound;
    uint8_t rom_chars[0x3000];
    uint8_t rom_tiles[0x6000];
    uint8_t rom_sprites[0x6000];
    uint8_t rom_maps[0x1000];
} bombjack_t;
bombjack_t bj;

sapp_desc sokol_main(int argc, char* argv[]) {
    args_init(argc, argv);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = DISPLAY_WIDTH * 2,
        .height = DISPLAY_HEIGHT * 2,
        .window_title = "Bomb Jack"
    };
}

/* one time app init */
void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        .fb_width = DISPLAY_WIDTH,
        .fb_height = DISPLAY_HEIGHT,
        .aspect_x = 1,
        .aspect_y = 1,
        .rot90 = true
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});
    bombjack_init();
}

/* per-frame stuff */
void app_frame(void) {
    bombjack_exec(clock_frame_time());
    gfx_draw();
}

/* input handling */
void app_input(const sapp_event* event) {
    switch (event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            switch (event->key_code) {
                /* player 1 joystick */
                case SAPP_KEYCODE_RIGHT: bj.main.p1 |= (1<<0); break;
                case SAPP_KEYCODE_LEFT:  bj.main.p1 |= (1<<1); break;
                case SAPP_KEYCODE_UP:    bj.main.p1 |= (1<<2); break;
                case SAPP_KEYCODE_DOWN:  bj.main.p1 |= (1<<3); break;
                case SAPP_KEYCODE_SPACE: bj.main.p1 |= (1<<4); break;
                /* player 1 coin */
                case SAPP_KEYCODE_1:     bj.main.sys |= (1<<0); break;
                /* player 1 start (any other key */
                default:                 bj.main.sys |= (1<<2); break;
            }
            break;

        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                /* player 1 joystick */
                case SAPP_KEYCODE_RIGHT: bj.main.p1 &= ~(1<<0); break;
                case SAPP_KEYCODE_LEFT:  bj.main.p1 &= ~(1<<1); break;
                case SAPP_KEYCODE_UP:    bj.main.p1 &= ~(1<<2); break;
                case SAPP_KEYCODE_DOWN:  bj.main.p1 &= ~(1<<3); break;
                case SAPP_KEYCODE_SPACE: bj.main.p1 &= ~(1<<4); break;
                /* player 1 coin */
                case SAPP_KEYCODE_1:     bj.main.sys &= ~(1<<0); break;
                /* player 1 start (any other key */
                default:                 bj.main.sys &= ~(1<<2); break;
            }
            break;
        default:
            break;
    }
}

/* app shutdown */
void app_cleanup(void) {
    saudio_shutdown();
    gfx_shutdown();
}

/* initialize the Bombjack arcade hardware */
void bombjack_init(void) {
    memset(&bj, 0, sizeof(bj));

    /* set cached palette entries to black */
    for (int i = 0; i < 128; i++) {
        bj.main.palette[i] = 0xFF000000;
    }

    /* setup the main board (4 MHz Z80) */
    clk_init(&bj.main.clk, 4000000);
    z80_init(&bj.main.cpu, &(z80_desc_t){
        .tick_cb = bombjack_tick_main
    });

    /* setup the sound board (3 MHz Z80, 3x 1.5 MHz AY-38910) */
    clk_init(&bj.sound.clk, 3000000);
    z80_init(&bj.sound.cpu, &(z80_desc_t){
        .tick_cb = bombjack_tick_sound
    });
    for (int i = 0; i < 3; i++) {
        ay38910_init(&bj.sound.ay[i], &(ay38910_desc_t) {
            .type = AY38910_TYPE_8910,
            .in_cb = bombjack_ay_in,
            .out_cb = bombjack_ay_out,
            .tick_hz = 1500000,
            .sound_hz = saudio_sample_rate(),
            .magnitude = 0.3f,
        });
    }

    /* dip switches (FIXME: should be configurable by cmdline args) */
    bj.main.dsw1 = (1<<5)|(1<<6)|(1<<7);   /* 5LIVES|UPRIGHT|DEMO SOUND */
    bj.main.dsw2 = (1<<5);          /* easy difficulty (enemy number of speed) */
    
    /* main board memory map:
        0000..7FFF: ROM
        8000..8FFF: RAM
        9000..93FF: video ram
        9400..97FF: color ram
        9820..987F: sprite ram
        9C00..9CFF: palette
        9E00:       select background
        B000:       read: joystick 1, write: NMI mask
        B001:       read: joystick 2
        B002:       read: coins and start button
        B003:       ???
        B004:       read: dip-switches 1, write: flip screen
        B005:       read: dip-switches 2
        B800:       sound latch
        C000..DFFF: ROM

      palette RAM is 128 entries with 16-bit per entry (xxxxBBBBGGGGRRRR).
    */
    mem_init(&bj.main.mem);
    mem_map_rom(&bj.main.mem, 0, 0x0000, 0x2000, dump_09_j01b);
    mem_map_rom(&bj.main.mem, 0, 0x2000, 0x2000, dump_10_l01b);
    mem_map_rom(&bj.main.mem, 0, 0x4000, 0x2000, dump_11_m01b);
    mem_map_rom(&bj.main.mem, 0, 0x6000, 0x2000, dump_12_n01b);
    mem_map_ram(&bj.main.mem, 0, 0x8000, 0x2000, bj.main.ram);
    mem_map_rom(&bj.main.mem, 0, 0xC000, 0x2000, dump_13);

    /* sound board memory map */
    mem_init(&bj.sound.mem);
    mem_map_rom(&bj.sound.mem, 0, 0x0000, 0x2000, dump_01_h03t);
    mem_map_ram(&bj.sound.mem, 0, 0x4000, 0x0400, bj.sound.ram);

    /* copy ROM data that's not accessible by CPU, no need to put a
       memory mapper inbetween there
    */
    assert(sizeof(bj.rom_chars) == sizeof(dump_03_e08t)+sizeof(dump_04_h08t)+sizeof(dump_05_k08t));
    memcpy(&bj.rom_chars[0x0000], dump_03_e08t, 0x1000);
    memcpy(&bj.rom_chars[0x1000], dump_04_h08t, 0x1000);
    memcpy(&bj.rom_chars[0x2000], dump_05_k08t, 0x1000);

    assert(sizeof(bj.rom_tiles) == sizeof(dump_06_l08t)+sizeof(dump_07_n08t)+sizeof(dump_08_r08t));
    memcpy(&bj.rom_tiles[0x0000], dump_06_l08t, 0x2000);
    memcpy(&bj.rom_tiles[0x2000], dump_07_n08t, 0x2000);
    memcpy(&bj.rom_tiles[0x4000], dump_08_r08t, 0x2000);

    assert(sizeof(bj.rom_sprites) == sizeof(dump_16_m07b)+sizeof(dump_15_l07b)+sizeof(dump_14_j07b));
    memcpy(&bj.rom_sprites[0x0000], dump_16_m07b, 0x2000);
    memcpy(&bj.rom_sprites[0x2000], dump_15_l07b, 0x2000);
    memcpy(&bj.rom_sprites[0x4000], dump_14_j07b, 0x2000);

    assert(sizeof(bj.rom_maps) == sizeof(dump_02_p04t));
    memcpy(&bj.rom_maps[0x0000], dump_02_p04t, 0x1000);
}

/* run the emulation for one frame */
void bombjack_exec(uint32_t micro_seconds) {
    /* tick the main board */
    uint32_t ticks_to_run = clk_ticks_to_run(&bj.main.clk, micro_seconds);
    uint32_t ticks_executed = 0;
    while (ticks_executed < ticks_to_run) {
        ticks_executed += z80_exec(&bj.main.cpu, ticks_to_run);
    }
    clk_ticks_executed(&bj.main.clk, ticks_executed);
    bombjack_decode_video();
}

/* main board tick callback */
uint64_t bombjack_tick_main(int num, uint64_t pins, void* user_data) {

    /* vsync and main board NMI */
    bj.main.vsync_count += num;
    if (bj.main.vsync_count >= VSYNC_PERIOD) {
        bj.main.vsync_count -= VSYNC_PERIOD;
        if (bj.main.nmi_mask != 0) {
            pins |= Z80_NMI;
        }
    }
    if (0 == bj.main.nmi_mask) {
        pins &= ~Z80_NMI;
    }

    const uint16_t addr = Z80_GET_ADDR(pins);
    if (pins & Z80_MREQ) {
        /* memory request */
        if ((addr >= 0x9C00) && (addr < 0x9D00)) {
            /* palette read/write */
            if (pins & Z80_RD) {
                Z80_SET_DATA(pins, mem_rd(&bj.main.mem, addr));
            }
            else if (pins & Z80_WR) {
                const uint8_t data = Z80_GET_DATA(pins);
                mem_wr(&bj.main.mem, addr, data);
                /* stretch palette colors to 32 bit (original layout is
                   xxxxBBBBGGGGRRRR)
                */
                uint16_t pal_index = (addr - 0x9C00) / 2;
                uint32_t c = bj.main.palette[pal_index];
                if (addr & 1) {
                    /* uneven addresses are the xxxxBBBB part */
                    uint8_t b = (data & 0x0F) | ((data<<4)&0xF0);
                    c = (c & 0xFF00FFFF) | (b<<16);
                }
                else {
                    /* even addresses are the GGGGRRRR part */
                    uint8_t g = (data & 0xF0) | ((data>>4)&0x0F);
                    uint8_t r = (data & 0x0F) | ((data<<4)&0xF0);
                    c = (c & 0xFFFF0000) | (g<<8) | r;
                }
                bj.main.palette[pal_index] = c;
            }
        }
        else if ((addr >= 0xB000) && (addr <= 0xB005)) {
            /* IO ports */
            if (addr == 0xB000) {
                /* read: joystick port 1:
                    0:  right
                    1:  left
                    2:  up
                    3:  down
                    5:  btn
                write: IRQ mask
                */
                if (pins & Z80_RD) {
                    Z80_SET_DATA(pins, bj.main.p1);
                }
                else if (pins & Z80_WR) {
                    bj.main.nmi_mask = Z80_GET_DATA(pins);
                }
            }
            else if (addr == 0xB001) {
                /* joystick port 2 */
                if (pins & Z80_RD) {
                    Z80_SET_DATA(pins, bj.main.p2);
                }
                else if (pins & Z80_WR) {
                    //printf("Trying to write joy2\n");
                }
            }
            else if (addr == 0xB002) {
                /* system:
                    0:  coin1
                    1:  coin2
                    2:  start1
                    3:  start2
                */
                if (pins & Z80_RD) {
                    Z80_SET_DATA(pins, bj.main.sys);
                }
                else if (pins & Z80_WR) {
                    //printf("Trying to write sys\n");
                }
            }
            else if (addr == 0xB003) {
                /* ??? */
                /*
                if (pins & Z80_RD) {
                    printf("read from 0xB003\n");
                }
                else if (pins & Z80_WR) {
                    printf("write to 0xB003\n");
                }
                */
            }
            else if (addr == 0xB004) {
                /* read: dip-switches 1
                write: flip screen
                */
                if (pins & Z80_RD) {
                    Z80_SET_DATA(pins, bj.main.dsw1);
                }
                else if (pins & Z80_WR) {
                    //printf("flip screen\n");
                }
            }
            else if (addr == 0xB005) {
                /* read: dip-switches 2 */
                if (pins & Z80_RD) {
                    Z80_SET_DATA(pins, bj.main.dsw2);
                }
                else if (pins & Z80_WR) {
                    //printf("write to 0xB005\n");
                }
            }
        }
        else if (addr == 0xB800) {
            /* sound latch */
            if (pins & Z80_RD) {
                //printf("read sound latch\n");
            }
            else {
                //printf("write sound latch\n");
            }
        }
        else {
            if (pins & Z80_RD) {
                Z80_SET_DATA(pins, mem_rd(&bj.main.mem, addr));
            }
            else if (pins & Z80_WR) {
                mem_wr(&bj.main.mem, addr, Z80_GET_DATA(pins));
            }
        }
    }
    /* the Z80 IORQ pin isn't connected, so no IO instructions need to be handled */
    return pins & Z80_PIN_MASK;
}

/* sound board tick callback */
uint64_t bombjack_tick_sound(int num, uint64_t pins, void* user_data) {
    return pins;
}

/* AY port output callback */
void bombjack_ay_out(int port_id, uint8_t data, void* user_data) {

}

/* AY port input callback */
uint8_t bombjack_ay_in(int port_id, void* user_data) {
    return 0xFF;
}

/* render background tiles

    Background tiles are 16x16 pixels, and the screen is made of
    16x16 tiles. A background images consists of 16x16=256 tile
    'char codes', followed by 256 color code bytes. So each background
    image occupies 512 (0x200) bytes in the 'map rom'.

    The map-rom is 4 KByte, room for 8 background images (although I'm
    not sure yet whether all 8 are actually used). The background
    image number is written to address 0x9E00 (only the 3 LSB bits are
    considered). If bit 4 is cleared, no background image is shown
    (all tile codes are 0).

    A tile's image is created from 3 bitmaps, each bitmap stored in
    32 bytes with the following layout (the numbers are the byte index,
    each byte contains the bitmap pattern for 8 pixels):

    0: +--------+   8: +--------+
    1: +--------+   9: +--------+
    2: +--------+   10:+--------+
    3: +--------+   11:+--------+
    4: +--------+   12:+--------+
    5: +--------+   13:+--------+
    6: +--------+   14:+--------+
    7: +--------+   15:+--------+

    16:+--------+   24:+--------+
    17:+--------+   25:+--------+
    18:+--------+   26:+--------+
    19:+--------+   27:+--------+
    20:+--------+   28:+--------+
    21:+--------+   29:+--------+
    22:+--------+   30:+--------+
    23:+--------+   31:+--------+

    The 3 bitmaps for each tile are 8 KBytes apart (basically each
    of the 3 background-tile ROM chips contains one set of bitmaps
    for all 256 tiles).

    The 3 bitmaps are combined to get the lower 3 bits of the
    color palette index. The remaining 4 bits of the palette
    index are provided by the color attribute byte (for 7 bits
    = 128 color palette entries).

    This is how a color palette entry is constructed from the 4
    attribute bits, and 3 tile bitmap bits:

    |x|attr3|attr2|attr1|attr0|bm0|bm1|bm2|

    This basically means that each 16x16 background tile
    can select one of 16 color blocks from the palette, and
    each pixel of the tile can select one of 8 colors in the
    tile's color block.

    Bit 7 in the attribute byte defines whether the tile should
    be flipped around the Y axis.
*/
#define BOMBJACK_GATHER16(rom,base,off) \
    ((uint16_t)bj.rom[base+0+off]<<8)|((uint16_t)bj.rom[base+8+off])

void bombjack_decode_background(uint32_t* dst) {
    /* background image code is stored at address 0x9E00 */
    uint8_t bg_image = bj.main.ram[0x9E00 - 0x8000];
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int addr = ((bg_image & 7) * 0x200) + (y * 16 + x);
            /* 256 tiles */
            uint8_t tile_code = (bg_image & 0x10) ? bj.rom_maps[addr] : 0;
            uint8_t attr = bj.rom_maps[addr + 0x0100];
            uint8_t color_block = (attr & 0x0F)<<3;
            bool flip_y = (attr & 0x80) != 0;
            if (flip_y) {
                dst += 15*256;
            }
            /* every tile is 32 bytes */
            int off = tile_code * 32;
            for (int yy = 0; yy < 16; yy++) {
                uint16_t bm0 = BOMBJACK_GATHER16(rom_tiles, 0x0000, off);
                uint16_t bm1 = BOMBJACK_GATHER16(rom_tiles, 0x2000, off);
                uint16_t bm2 = BOMBJACK_GATHER16(rom_tiles, 0x4000, off);
                off++;
                if (yy == 7) {
                    off += 8;
                }
                for (int xx = 15; xx >= 0; xx--) {
                    uint8_t pen = ((bm2>>xx)&1) | (((bm1>>xx)&1)<<1) | (((bm0>>xx)&1)<<2);
                    *dst++ = bj.main.palette[color_block | pen];
                }
                dst += flip_y ? -272 : 240;
            }
            if (flip_y) {
                dst += 256 + 16;
            }
            else {
                dst -= (16 * 256) - 16;
            }
        }
        dst += (15 * 256);
    }
    assert(dst == gfx_framebuffer()+256*256);
}

/* render foreground tiles

    Similar to the background tiles, but each tile is 8x8 pixels,
    for 32x32 tiles on the screen.

    Tile char- and color-bytes are not stored in ROM, but in RAM
    at address 0x9000 (1 KB char codes) and 0x9400 (1 KB color codes).

    There are actually 512 char-codes, bit 4 of the color byte
    is used as the missing bit 8 of the char-code.

    The color decoding is the same as the background tiles, the lower
    3 bits are provided by the 3 tile bitmaps, and the remaining
    4 upper bits by the color byte.

    Only 7 foreground colors are possible, since 0 defines a transparent
    pixel.
*/
void bombjack_decode_foreground(uint32_t* dst) {
    /* 32x32 tiles, each 8x8 */
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            int addr = y * 32 + x;
            /* char codes are at 0x9000, color codes at 0x9400, RAM starts at 0x8000 */
            uint8_t chr = bj.main.ram[(0x9000-0x8000) + addr];
            uint8_t clr = bj.main.ram[(0x9400-0x8000) + addr];
            /* 512 foreground tiles, take 9th bit from color code */
            int tile_code = chr | ((clr & 0x10)<<4);
            /* 16 color blocks a 8 colors */
            int color_block = (clr & 0x0F)<<3;
            /* 8 bytes per char bitmap */
            int off = tile_code * 8;
            for (int yy = 0; yy < 8; yy++) {
                /* 3 bit planes per char (8 colors per pixel within
                   the palette color block of the char
                */
                uint8_t bm0 = bj.rom_chars[0x0000 + off];
                uint8_t bm1 = bj.rom_chars[0x1000 + off];
                uint8_t bm2 = bj.rom_chars[0x2000 + off];
                off++;
                for (int xx = 7; xx >= 0; xx--) {
                    uint8_t pen = ((bm2>>xx)&1) | (((bm1>>xx)&1)<<1) | (((bm0>>xx)&1)<<2);
                    if (pen != 0) {
                        *dst = bj.main.palette[color_block | pen];
                    }
                    dst++;
                }
                dst += 248;
            }
            dst -= (8 * 256) - 8;
        }
        dst += (7 * 256);
    }
    assert(dst == gfx_framebuffer()+256*256);
}

/*  render sprites

    Each sprite is described by 4 bytes in the 'sprite RAM'
    (0x9820..0x987F => 96 bytes => 24 sprites):

    ABBBBBBB CDEFGGGG XXXXXXXX YYYYYYYY

    A:  sprite size (16x16 or 32x32)
    B:  sprite index
    C:  X flip
    D:  Y flip
    E:  ?
    F:  ?
    G:  color
    X:  x pos
    Y:  y pos
*/
#define BOMBJACK_GATHER32(rom,base,off) \
    ((uint32_t)bj.rom[base+0+off]<<24)|\
    ((uint32_t)bj.rom[base+8+off]<<16)|\
    ((uint32_t)bj.rom[base+32+off]<<8)|\
    ((uint32_t)bj.rom[base+40+off])

void bombjack_decode_sprites(uint32_t* dst) {
    /* 24 hardware sprites, sprite 0 has highest priority */
    for (int sprite_nr = 23; sprite_nr >= 0; sprite_nr--) {
        /* sprite RAM starts at 0x9820, RAM starts at 0x8000 */
        int addr = (0x9820 - 0x8000) + sprite_nr*4;
        uint8_t b0 = bj.main.ram[addr + 0];
        uint8_t b1 = bj.main.ram[addr + 1];
        uint8_t b2 = bj.main.ram[addr + 2];
        uint8_t b3 = bj.main.ram[addr + 3];
        uint8_t color_block = (b1 & 0x0F)<<3;

        /* screen is 90 degree rotated, so x and y are switched */
        int px = b3;
        int sprite_code = b0 & 0x7F;
        if (b0 & 0x80) {
            /* 32x32 'large' sprites (no flip-x/y needed) */
            int py = 225 - b2;
            uint32_t* ptr = dst + py*256 + px;
            /* offset into sprite ROM to gather sprite bitmap pixels */
            int off = sprite_code * 128;
            for (int y = 0; y < 32; y++) {
                uint32_t bm0 = BOMBJACK_GATHER32(rom_sprites, 0x0000, off);
                uint32_t bm1 = BOMBJACK_GATHER32(rom_sprites, 0x2000, off);
                uint32_t bm2 = BOMBJACK_GATHER32(rom_sprites, 0x4000, off);
                off++;
                if ((y & 7) == 7) {
                    off += 8;
                }
                if ((y & 15) == 15) {
                    off += 32;
                }
                for (int x = 31; x >= 0; x--) {
                    uint8_t pen = ((bm2>>x)&1) | (((bm1>>x)&1)<<1) | (((bm0>>x)&1)<<2);
                    if (0 != pen) {
                        *ptr = bj.main.palette[color_block | pen];
                    }
                    ptr++;
                }
                ptr += 224;
            }
        }
        else {
            /* 16*16 sprites are decoded like 16x16 background tiles */
            int py = 241 - b2;
            uint32_t* ptr = dst + py*256 + px;
            bool flip_x = (b1 & 0x80) != 0;
            bool flip_y = (b1 & 0x40) != 0;
            if (flip_x) {
                ptr += 16*256;
            }
            /* offset into sprite ROM to gather sprite bitmap pixels */
            int off = sprite_code * 32;
            for (int y = 0; y < 16; y++) {
                uint16_t bm0 = BOMBJACK_GATHER16(rom_sprites, 0x0000, off);
                uint16_t bm1 = BOMBJACK_GATHER16(rom_sprites, 0x2000, off);
                uint16_t bm2 = BOMBJACK_GATHER16(rom_sprites, 0x4000, off);
                off++;
                if (y == 7) {
                    off += 8;
                }
                if (flip_y) {
                    for (int x=0; x<=15; x++) {
                        uint8_t pen = ((bm2>>x)&1) | (((bm1>>x)&1)<<1) | (((bm0>>x)&1)<<2);
                        if (0 != pen) {
                            *ptr = bj.main.palette[color_block | pen];
                        }
                        ptr++;
                    }
                }
                else {
                    for (int x=15; x>=0; x--) {
                        uint8_t pen = ((bm2>>x)&1) | (((bm1>>x)&1)<<1) | (((bm0>>x)&1)<<2);
                        if (0 != pen) {
                            *ptr = bj.main.palette[color_block | pen];
                        }
                        ptr++;
                    }
                }
                ptr += flip_x ? -272 : 240;
            }
        }
    }
}

void bombjack_decode_video() {
    uint32_t* dst = gfx_framebuffer();
    bombjack_decode_background(dst);
    bombjack_decode_foreground(dst);
    bombjack_decode_sprites(dst);
}
