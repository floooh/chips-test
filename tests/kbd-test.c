//------------------------------------------------------------------------------
//  kbd-test.c
//  Test keyboard matrix helper functions.
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/keyboard_matrix.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

int main() {
    keyboard_matrix kbd;
    kbd_init(&kbd, &(keyboard_matrix_desc){0});
    kbd_register_shift(&kbd, 1, 0, 0);
    kbd_register_key(&kbd, 'a', 2, 3, 0);
    kbd_register_key(&kbd, 'A', 2, 3, 1);
    kbd_register_key(&kbd, 'b', 3, 3, 0);
    kbd_register_key(&kbd, 'B', 3, 3, 1);
    T(0 == kbd_get_bits(&kbd));
    /* unshifted, single key */
    kbd_key_down(&kbd, 'a');
    T(((1<<(2+16))|(1<<3)) == kbd_get_bits(&kbd));
    for (int i = 0; i < 10; i++) {
        kbd_update(&kbd);
        T(((1<<(2+16))|(1<<3)) == kbd_get_bits(&kbd));
    }
    kbd_key_up(&kbd, 'a');
    kbd_update(&kbd);
    T(0 == kbd_get_bits(&kbd));

    /* unshifted key, short pressed */
    kbd_key_down(&kbd, 'b');
    kbd_key_up(&kbd, 'b');
    T(((1<<(3+16))|(1<<3)) == kbd_get_bits(&kbd));
    for (int i = 0; i < kbd.sticky_count; i++) {
        kbd_update(&kbd);
        T(((1<<(3+16))|(1<<3)) == kbd_get_bits(&kbd));
    }
    kbd_update(&kbd);
    T(0 == kbd_get_bits(&kbd));

    /* shifted key */
    kbd_key_down(&kbd, 'A');
    T(((1<<(2+16))|(1<<16)|(1<<3)|(1<<0)) == kbd_get_bits(&kbd));
    kbd_key_up(&kbd, 'A');
    for (int i = 0; i < 5; i++) {
        kbd_update(&kbd);
    }
    T(0 == kbd_get_bits(&kbd));

    /* multiple keys */
    kbd_key_down(&kbd, 'a');
    kbd_key_down(&kbd, 'b');
    T(((1<<(2+16))|(1<<(3+16))|(1<<3)) == kbd_get_bits(&kbd));
    kbd_key_up(&kbd, 'a');
    for (int i = 0; i < 5; i++) {
        kbd_update(&kbd);
    }
    T(((1<<(3+16))|(1<<3)) == kbd_get_bits(&kbd));
    kbd_key_up(&kbd, 'b');
    for (int i = 0; i < 5; i++) {
        kbd_update(&kbd);
    }
    T(0 == kbd_get_bits(&kbd));

    return 0;
}
