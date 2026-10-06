#ifndef SUIRABOX_INPUT_ABI_H
#define SUIRABOX_INPUT_ABI_H
#define SB_INPUT_ABI_VERSION 1
#define SB_KEY_EVENT_SIZE 32
#define SB_INPUT_EVENT_KEY 1
#define SB_INPUT_EVENT_OVERFLOW 2
#define SB_KEY_DOWN 1
#define SB_KEY_REPEAT 2
#define SB_KEY_MOD_SHIFT 1
#define SB_KEY_MOD_CTRL 2
#define SB_KEY_MOD_ALT 4
#define SB_KEY_MOD_CAPS 8
#define SB_KEY_MOD_META 16
/* Physical US-labelled positions: ASCII uppercase letters, digits, punctuation
 * and space; special positions start at 256. No scancodes or text encoding. */
#define SB_KEY_ENTER 256
#define SB_KEY_ESCAPE 257
#define SB_KEY_BACKSPACE 258
#define SB_KEY_TAB 259
#define SB_KEY_LEFT_SHIFT 260
#define SB_KEY_RIGHT_SHIFT 261
#define SB_KEY_LEFT_CTRL 262
#define SB_KEY_RIGHT_CTRL 263
#define SB_KEY_LEFT_ALT 264
#define SB_KEY_RIGHT_ALT 265
#define SB_KEY_LEFT_META 266
#define SB_KEY_RIGHT_META 267
#define SB_KEY_CAPS_LOCK 268
#define SB_KEY_UP 272
#define SB_KEY_DOWN_ARROW 273
#define SB_KEY_LEFT 274
#define SB_KEY_RIGHT 275
#define SB_KEY_HOME 276
#define SB_KEY_END 277
#define SB_KEY_DELETE 278
#define SB_KEY_INSERT 279
#define SB_KEY_PAGE_UP 280
#define SB_KEY_PAGE_DOWN 281
#define SB_KEY_F1 288
#define SB_KEY_PRINT_SCREEN 300
#define SB_KEY_PAUSE 301
#ifndef __ASSEMBLER__
#include <stdint.h>
typedef struct {
    uint32_t size;
    uint16_t version, type;
    uint16_t keycode, flags;
    uint32_t modifiers;
    uint64_t sequence;
    uint32_t dropped, reserved;
} sb_key_event_t;
#endif
#endif
