#pragma once
/**
 *  kitty.h - Kitty graphics and input protocol helpers
 */
#include <stdint.h>
#include <stddef.h>

void kitty_setup_termios(void);
void kitty_restore_termios(void);
void kitty_hide_cursor(void);
void kitty_show_cursor(void);
void kitty_set_pos(int x, int y);
bool kitty_send_image(chips_display_info_t display_info, uint32_t frame_id);
void kitty_poll_events(int ms);