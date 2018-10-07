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

#define DISPLAY_WIDTH (256)
#define DISPLAY_HEIGHT (256)

#define VSYNC_PERIOD_4MHZ (4000000 / 60)
#define VBLANK_DURATION_4MHZ (((4000000 / 60) / 525) * (525 - 483))
#define VSYNC_PERIOD_3MHZ (3000000 / 60)

#define NUM_AUDIO_SAMPLES (128)

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
    int num_samples;
    int sample_pos;
    float sample_buffer[NUM_AUDIO_SAMPLES];
    /* 32-bit RGBA color palette cache */
    uint32_t palette_cache[128];
} bombjack_t;
bombjack_t bj;

static uint64_t bombjack_tick_main(int num, uint64_t pins, void* user_data);
static uint64_t bombjack_tick_sound(int num, uint64_t pins, void* user_data);
static void bombjack_decode_video(void);

/* initialize the Bombjack arcade hardware */
static void bombjack_init(void) {
    memset(&bj, 0, sizeof(bj));

    /* The VSYNC/VBLANK mainly controls the interrupts (Bombjack generally
        uses NMIs for simplicity. The mainboard's NMI is connected to the
        VBLANK signal, and stays active for the VBLANK period, the
        soundboard's NMI is connected to a flip-flop which is switched
        active when VSYNC happens, and deactivated when the interrupt
        service routine reads from the shared sound-command latch
        (mapped into the mainboard CPU's address space at 0xB800, and
        the sound board's address space at 0x6000).

        NOTE: it's important that the machine doesn't start right off with a vsync
        when it is switched on, since this would cause an NMI before
        the stack pointer is setup!)
    */
    bj.main.vsync_count = VSYNC_PERIOD_4MHZ;
    bj.main.vblank_count = 0;
    bj.sound.vsync_count = VSYNC_PERIOD_3MHZ;

    /* setup the main board (4 MHz Z80) */
    clk_init(&bj.main.clk, 4000000);
    z80_init(&bj.main.cpu, &(z80_desc_t){
        .tick_cb = bombjack_tick_main
    });

    /* setup the sound board (3 MHz Z80 and 3x 1.5 MHz AY-38910) */
    clk_init(&bj.sound.clk, 3000000);
    z80_init(&bj.sound.cpu, &(z80_desc_t){
        .tick_cb = bombjack_tick_sound
    });
    for (int i = 0; i < 3; i++) {
        ay38910_init(&bj.sound.psg[i], &(ay38910_desc_t) {
            .type = AY38910_TYPE_8910,
            .tick_hz = 1500000,
            .sound_hz = saudio_sample_rate(),
            .magnitude = 0.3f,
        });
    }

    /* dip switches (FIXME: should be configurable by cmdline args) */
    bj.main.dsw1 = (1<<6)|(1<<7);   /* UPRIGHT|DEMO SOUND (demo sound doesn't work?) */
    bj.main.dsw2 = (1<<5);          /* easy difficulty (enemy number of speed) */
    
    /* main board memory map:
        0000..7FFF: ROM
        8000..8FFF: RAM
        9000..93FF: video ram
        9400..97FF: color ram
        9820..987F: sprite ram
        9C00..9CFF: palette ram (write-only?)
        9E00:       select background (write-only?)
        B000:       read: joystick 1, write: NMI mask
        B001:       read: joystick 2
        B002:       read: coins and start button
        B003:       read/write: watchdog reset (not emulated)
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
    mem_map_ram(&bj.main.mem, 0, 0x8000, 0x1C00, bj.main_ram);
    mem_map_rom(&bj.main.mem, 0, 0xC000, 0x2000, dump_13);

    /* sound board memory map */
    mem_init(&bj.sound.mem);
    mem_map_rom(&bj.sound.mem, 0, 0x0000, 0x2000, dump_01_h03t);
    mem_map_ram(&bj.sound.mem, 0, 0x4000, 0x0400, bj.sound_ram);

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
static void bombjack_exec(uint32_t micro_seconds) {

    /* Run the main board and sound board interleaved for half a frame.
       This simplifies the communication via the sound latch (the main CPU
       writes a command byte to the sound latch, the sound board reads
       the command latch in its interrupt service routine.

       The main board issues at most one command per 60Hz frame, but since the
       host machine is also running at roughly 60 Hz it may happen that the
       main board writes 2 sound commands per host frame. For this reason
       run the 2 boards interleaved for half a frame, so it is guaranteed
       that at most one sound command can be written by the main board
       before the sound board is ticked (that way we don't need to implement
       a complicated command queue.
    */
    const uint32_t slice_us = micro_seconds/2;
    for (int i = 0; i < 2; i++) {
        /* tick the main board */
        {
            uint32_t ticks_to_run = clk_ticks_to_run(&bj.main.clk, slice_us);
            uint32_t ticks_executed = 0;
            while (ticks_executed < ticks_to_run) {
                ticks_executed += z80_exec(&bj.main.cpu, ticks_to_run);
            }
            clk_ticks_executed(&bj.main.clk, ticks_executed);
        }
        /* tick the sound board */
        {
            uint32_t ticks_to_run = clk_ticks_to_run(&bj.sound.clk, slice_us);
            uint32_t ticks_executed = 0;
            while (ticks_executed < ticks_to_run) {
                ticks_executed += z80_exec(&bj.sound.cpu, ticks_to_run);
            }
            clk_ticks_executed(&bj.sound.clk, ticks_executed);
        }
    }
    /* decode the video image once per host frame */
    bombjack_decode_video();
}

/* Maintain a color palette cache with 32-bit colors, this is called for
    CPU writes to the palette RAM area. The hardware palette is 128
    entries of 16-bit colors (xxxxBBBBGGGGRRRR), the function keeps
    a 'palette cache' with 32-bit colors uptodate, so that the
    32-bit colors don't need to be computed for each pixel in the
    video decoding code.
*/
static inline void bombjack_update_palette_cache(uint16_t addr, uint8_t data) {
    assert((addr >= 0x9C00) && (addr < 0x9D00));
    int pal_index = (addr - 0x9C00) / 2;
    uint32_t c = bj.palette_cache[pal_index];
    if (addr & 1) {
        /* uneven addresses are the xxxxBBBB part */
        uint8_t b = (data & 0x0F) | ((data<<4)&0xF0);
        c = 0xFF000000 | (c & 0x0000FFFF) | (b<<16);
    }
    else {
        /* even addresses are the GGGGRRRR part */
        uint8_t g = (data & 0xF0) | ((data>>4)&0x0F);
        uint8_t r = (data & 0x0F) | ((data<<4)&0xF0);
        c = 0xFF000000 | (c & 0x00FF0000) | (g<<8) | r;
    }
    bj.palette_cache[pal_index] = c;
}

/* main board tick callback

    Bomb Jack uses memory mapped IO (the Z80's IORQ pin isn't connected).

    Special memory locations:

    9C00..9D00: the hardware color palette (128 entries @ 16-bit)
                I'm not sure if this area is normal memory and readable,
                the CPU doesn't appear to do any read accesses, write accesses
                are caught and used to update the color palette cache.
    B000:       read:  player 1 joystick state
                        bit 0: right
                            1: left
                            2: up
                            3: down
                            4: btn
                write: NMI mask (NMI on VSYNC disabled when 0 written to B000)
    B001:       read:  player 2 joystick state
                write: ???
    B002:       read:  coin detector and start button:
                        bit 0: player 1 coin
                            1: player 2 coin
                            2: player 1 start button
                            3: player 2 start button
                write:  ???
    B003:       read/write: connected to a WDCLR (watchdog clear?) line which together
                       with the VBLANK are connected to the RESET pin, the
                       game code seems to read B003 quite often, I guess
                       if those read or the VBLANK are not happening for
                       a little while because of a software or hardware
                       failure, the machine resets itself
    B004:       read:  dip-switches 1
                        bits [1,0]: 00: 1 COIN 1 PLAY (player 1)
                                    01: 1 COIN 2 PLAY
                                    10: 1 COIN 3 PLAY
                                    11: 1 COIN 5 PLAY
                             [3,2]: coin/play for player 2
                             [5,4]: 00: 3 Jacks
                                    01: 4 Jacks
                                    10: 5 Jacks
                                    11: 2 Jacks
                             6:     0: cocktail
                                    1: upright
                             7:     0: no demo sound
                                    1: demo sound
                write: flip-screen (not emulated)
    B005:       read: dip-switches 2
                        bits [4,3]: difficulty 1 (bird speed)
                                    00: moderate
                                    01: difficult
                                    10: more difficult
                                    11: top difficult
                        bits [6,5]: difficulty 2 (enemy number & speed)
                                    00: moderate
                                    01: easy
                                    10: difficult
                                    11: more difficult
                        7:          ratio of special coin appearance
                                    0:  easy
                                    1:  difficult
    B800:       sound command latch,

*/
static uint64_t bombjack_tick_main(int num_ticks, uint64_t pins, void* user_data) {

    /* activate NMI pin during VBLANK */
    bj.main.vsync_count -= num_ticks;
    if (bj.main.vsync_count < 0) {
        bj.main.vsync_count += VSYNC_PERIOD_4MHZ;
        bj.main.vblank_count = VBLANK_DURATION_4MHZ;
    }
    if (bj.main.vblank_count != 0) {
        bj.main.vblank_count -= num_ticks;
        if (bj.main.vblank_count < 0) {
            bj.main.vblank_count = 0;
        }
    }
    if (bj.main.nmi_mask && (bj.main.vblank_count > 0)) {
        pins |= Z80_NMI;
    }
    else {
        pins &= ~Z80_NMI;
    }

    /* handle memory requests

        In hardware, the address decoding is mostly implemented
        with cascaded 1-in-4 and 1-in-8 decoder chips. We'll take
        a little shortcut and just check for the expected address ranges.
    */
    uint16_t addr = Z80_GET_ADDR(pins);
    if (pins & Z80_MREQ) {
        if (pins & Z80_WR) {
            /* memory write access */
            uint8_t data = Z80_GET_DATA(pins);
            if ((addr >= 0x8000) && (addr < 0x9900)) {
                /* regular RAM, video/color RAM, sprite RAM */
                mem_wr(&bj.main.mem, addr, data);
            }
            else if ((addr >= 0x9C00) && (addr <= 0x9D00)) {
                /* color palette */
                bombjack_update_palette_cache(addr, data);
            }
            else if (addr == 0x9E00) {
                /* background image selection */
                bj.main.bg_image = data;
            }
            else if (addr == 0xB000) {
                /* NMI mask */
                bj.main.nmi_mask = data;
            }
            /* FIXME: 0xB004: flip screen */
            else if (addr == 0xB800) {
                /* shared sound latch */
                bj.sound_latch = Z80_GET_DATA(pins);
            }
        }
        else if (pins & Z80_RD) {
            /* memory read access */
            if ((addr >= 0xB000) && (addr <= 0xB005)) {
                /* IO ports */
                switch (addr) {
                    case 0xB000: Z80_SET_DATA(pins, bj.main.p1); break;
                    case 0xB001: Z80_SET_DATA(pins, bj.main.p2); break;
                    case 0xB002: Z80_SET_DATA(pins, bj.main.sys); break;
                    case 0xB004: Z80_SET_DATA(pins, bj.main.dsw1); break;
                    case 0xB005: Z80_SET_DATA(pins, bj.main.dsw2); break;
                }
            }
            else {
                /* regular memory */
                Z80_SET_DATA(pins, mem_rd(&bj.main.mem, addr));
            }
        }
    }
    /* the Z80 IORQ pin isn't connected, so no IO instructions need to be handled */
    return pins & Z80_PIN_MASK;
}

/* sound board tick callback

    The sound board receives commands from the main board via the shared
    sound command latch (mapped to address 0xB800 on the main board, and
    address 0x6000 on the sound board).

    The sound board reads the command latch at address in the interrupt
    handler (the NMI pin is activated by the VSYNC signal), and only
    copies the byte to RAM location 0x4391, and then sets the first bit
    in location 0x4390. The soundboard's "main loop" loops on the
    bit in 0x4390 which means a new sound command has been received.

    Reading the 8-bit latch at address 0x6000 automatically clears the
    latch and also clears the flip-flop connected to the soundboard's
    CPU NMI pin.

    Communication with the 3 sound chips is done through IO requests
    (not memory mapped IO like on the main board).

    The memory map of the sound board is as follows:

    0000 .. 1FFF    ROM
    4000 .. 43FF    RAM
    6000            shared sound latch

    The IO map:

    00 .. 01:       1st AY-3-8910
    10 .. 11:       2nd AY-3-8910
    80 .. 81:       3rd AY-3-8910
*/
static uint64_t bombjack_tick_sound(int num_ticks, uint64_t pins, void* user_data) {

    /* vsync triggers a flip-flop connected to the CPU's NMI, the flip-flop
       is reset on a read from address 0x6000 (this read happens in the
       interrupt service routine
    */
    bj.sound.vsync_count -= num_ticks;
    if (bj.sound.vsync_count < 0) {
        bj.sound.vsync_count += VSYNC_PERIOD_3MHZ;
        pins |= Z80_NMI;
    }

    /* tick the 3 sound chips at half frequency */
    for (int i = 0; i < num_ticks; i++) {
        if (bj.sound.tick_count++ & 1) {
            ay38910_tick(&bj.sound.psg[2]);
            ay38910_tick(&bj.sound.psg[1]);
            if (ay38910_tick(&bj.sound.psg[0])) {
                /* new sample ready */
                float s = bj.sound.psg[0].sample + bj.sound.psg[1].sample + bj.sound.psg[2].sample;
                bj.sample_buffer[bj.sample_pos++] = s;
                if (bj.sample_pos == NUM_AUDIO_SAMPLES) {
                    saudio_push(bj.sample_buffer, NUM_AUDIO_SAMPLES);
                    bj.sample_pos = 0;
                }
            }
        }
    }

    const uint16_t addr = Z80_GET_ADDR(pins);
    if (pins & Z80_MREQ) {
        /* memory requests */
        if (pins & Z80_RD) {
            /* special case: read and clear sound latch and NMI flip-flop */
            if (addr == 0x6000) {
                Z80_SET_DATA(pins, bj.sound_latch);
                bj.sound_latch = 0;
                pins &= ~Z80_NMI;
            }
            else {
                /* regular memory read */
                Z80_SET_DATA(pins, mem_rd(&bj.sound.mem, addr));
            }
        }
        else if (pins & Z80_WR) {
            /* regular memory write */
            mem_wr(&bj.sound.mem, addr, Z80_GET_DATA(pins));
        }
    }
    else if (pins & Z80_IORQ) {
        /* IO requests, schematics page 9 and 10

            PSG1, PSG2 and PSG3 are selected through a
            LS-138 1-of-4 decoder from address lines 4 and 7:

            A7 A4
             0  0   -> PSG 1
             0  1   -> PSG 2
             1  0   -> PSG 3
             1  1   -> not connected

            A0 is connected to BC1(!) (I guess that's an error in the
            schematics since these show BC2).
        */
        int psg_index = ((pins&Z80_A7)>>6) | ((pins&Z80_A4)>>4);
        if (psg_index < 4) {
            uint64_t psg_pins = (pins & Z80_PIN_MASK);
            if (pins & Z80_WR) {
                psg_pins |= AY38910_BDIR;
            }
            if (0 == (addr & 1)) {
                psg_pins |= AY38910_BC1;
            }
            pins = ay38910_iorq(&bj.sound.psg[psg_index], psg_pins);
        }
    }

    return pins & Z80_PIN_MASK;
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

static void bombjack_decode_background(uint32_t* dst) {
    /* only do a new decode when the image has changed */
    uint32_t* ptr = dst;
    int img_base_addr = (bj.main.bg_image & 7) * 0x0200;
    bool img_valid = (bj.main.bg_image & 0x10) != 0;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int addr = img_base_addr + (y * 16 + x);
            /* 256 tiles */
            uint8_t tile_code = img_valid ? bj.rom_maps[addr] : 0;
            uint8_t attr = bj.rom_maps[addr + 0x0100];
            uint8_t color_block = (attr & 0x0F)<<3;
            bool flip_y = (attr & 0x80) != 0;
            if (flip_y) {
                ptr += 15*256;
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
                    *ptr++ = bj.palette_cache[color_block | pen];
                }
                ptr += flip_y ? -272 : 240;
            }
            if (flip_y) {
                ptr += 256 + 16;
            }
            else {
                ptr -= (16 * 256) - 16;
            }
        }
        ptr += (15 * 256);
    }
    assert(ptr == dst+256*256);
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
static void bombjack_decode_foreground(uint32_t* dst) {
    /* 32x32 tiles, each 8x8 */
    uint32_t* ptr = dst;
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            int addr = y * 32 + x;
            /* char codes are at 0x9000, color codes at 0x9400, RAM starts at 0x8000 */
            uint8_t chr = bj.main_ram[(0x9000-0x8000) + addr];
            uint8_t clr = bj.main_ram[(0x9400-0x8000) + addr];
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
                    if (pen) {
                        *ptr = bj.palette_cache[color_block | pen];
                    }
                    ptr++;
                }
                ptr += 248;
            }
            ptr -= (8 * 256) - 8;
        }
        ptr += (7 * 256);
    }
    assert(ptr == dst+256*256);
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

static void bombjack_decode_sprites(uint32_t* dst) {
    /* 24 hardware sprites, sprite 0 has highest priority */
    for (int sprite_nr = 23; sprite_nr >= 0; sprite_nr--) {
        /* sprite RAM starts at 0x9820, RAM starts at 0x8000 */
        int addr = (0x9820 - 0x8000) + sprite_nr*4;
        uint8_t b0 = bj.main_ram[addr + 0];
        uint8_t b1 = bj.main_ram[addr + 1];
        uint8_t b2 = bj.main_ram[addr + 2];
        uint8_t b3 = bj.main_ram[addr + 3];
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
                        *ptr = bj.palette_cache[color_block | pen];
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
                            *ptr = bj.palette_cache[color_block | pen];
                        }
                        ptr++;
                    }
                }
                else {
                    for (int x=15; x>=0; x--) {
                        uint8_t pen = ((bm2>>x)&1) | (((bm1>>x)&1)<<1) | (((bm0>>x)&1)<<2);
                        if (0 != pen) {
                            *ptr = bj.palette_cache[color_block | pen];
                        }
                        ptr++;
                    }
                }
                ptr += flip_x ? -272 : 240;
            }
        }
    }
}

static void bombjack_decode_video() {
    uint32_t* dst = gfx_framebuffer();
    bombjack_decode_background(dst);
    bombjack_decode_foreground(dst);
    bombjack_decode_sprites(dst);
}

/* one time app init */
static void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        .fb_width = DISPLAY_WIDTH,
        .fb_height = DISPLAY_HEIGHT,
        .aspect_x = 4,
        .aspect_y = 5,
        .rot90 = true
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});
    bombjack_init();
}

/* per-frame stuff */
static void app_frame(void) {
    bombjack_exec(clock_frame_time());
    gfx_draw();
}

/* input handling */
static void app_input(const sapp_event* event) {
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
static void app_cleanup(void) {
    saudio_shutdown();
    gfx_shutdown();
}

/* sokol-app entry */
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

