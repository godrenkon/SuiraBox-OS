#ifndef SB_LINE_EDIT_H
#define SB_LINE_EDIT_H
#include <suirabox/input_abi.h>
#define SB_LINE_EDIT_CAPACITY 32u
#define SB_LINE_CHANGED 1
#define SB_LINE_SUBMITTED 2
#define SB_LINE_OVERFLOW 3
typedef struct {
    char text[SB_LINE_EDIT_CAPACITY+1u], submitted[SB_LINE_EDIT_CAPACITY+1u];
    uint32_t length, submitted_length;
} sb_line_edit_t;
void sb_line_edit_init(sb_line_edit_t *editor);
/* US ASCII demo layout in userspace; no IME, selection or cursor movement. */
int sb_line_edit_event(sb_line_edit_t *editor, const sb_key_event_t *event);
#endif
