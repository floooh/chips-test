//------------------------------------------------------------------------------
//  kbd-test.c
//  Test keyboard matrix helper functions.
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/kbd.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

UTEST(kbd, kbd) {
    kbd_t kbd;
    kbd_init(&kbd, 0);
    kbd_register_modifier(&kbd, 0, 0, 0);
    kbd_register_key(&kbd, 'a', 2, 3, 0);
    kbd_register_key(&kbd, 'A', 2, 3, 1);
    kbd_register_key(&kbd, 'b', 3, 3, 0);
    kbd_register_key(&kbd, 'B', 3, 3, 1);
    T(0 == kbd_test_lines(&kbd, 0xFFFF));

    /* unshifted, single key */
    kbd_key_down(&kbd, 'a');
    T((1<<3) == kbd_test_lines(&kbd, 0xFFFF));
    T(0 == kbd_test_lines(&kbd, 0));
    T((1<<3) == kbd_test_lines(&kbd, (1<<2)));
    for (int i = 0; i < 10; i++) {
        kbd_update(&kbd, 16667);
        T((1<<3) == kbd_test_lines(&kbd, 0xFFFF));
    }
    kbd_key_up(&kbd, 'a');
    kbd_update(&kbd, 16667);
    T(0 == kbd_test_lines(&kbd, 0xFFFF));

    /* unshifted key, short pressed */
    kbd_key_down(&kbd, 'b');
    kbd_key_up(&kbd, 'b');
    T((1<<3) == kbd_test_lines(&kbd, 0xFFFF));
    kbd_update(&kbd, 16667);
    kbd_update(&kbd, 16667);
    T(0 == kbd_test_lines(&kbd, 0xFFFF));

    /* shifted key */
    kbd_key_down(&kbd, 'A');
    T(((1<<3)|(1<<0)) == kbd_test_lines(&kbd, 0xFFFF));
    kbd_key_up(&kbd, 'A');
    for (int i = 0; i < 5; i++) {
        kbd_update(&kbd, 16667);
    }
    T(0 == kbd_test_lines(&kbd, 0xFFFF));

    /* multiple keys */
    kbd_key_down(&kbd, 'a');
    kbd_key_down(&kbd, 'b');
    T((1<<3) == kbd_test_lines(&kbd, 0xFFFF));
    kbd_key_up(&kbd, 'a');
    for (int i = 0; i < 5; i++) {
        kbd_update(&kbd, 16667);
    }
    T((1<<3) == kbd_test_lines(&kbd, 0xFFFF));
    kbd_key_up(&kbd, 'b');
    for (int i = 0; i < 5; i++) {
        kbd_update(&kbd, 16667);
    }
    T(0 == kbd_test_lines(&kbd, 0xFFFF));
}
