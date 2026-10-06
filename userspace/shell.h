#ifndef SB_USERSPACE_SHELL_H
#define SB_USERSPACE_SHELL_H
#include "text.h"
#include <suirabox/syscall_abi.h>

#define SB_SHELL_ROWS 12u
enum { SB_SHELL_HOME, SB_SHELL_FILES, SB_SHELL_SETTINGS };
enum { SB_SHELL_NONE, SB_SHELL_REDRAW, SB_SHELL_RELOAD };
typedef struct { unsigned view, disk, overflow; } sb_shell_state_t;
typedef struct {
    unsigned count, truncated;
    int error;
    sb_directory_entry_t entries[SB_SHELL_ROWS];
} sb_shell_listing_t;
typedef int64_t (*sb_shell_call_t)(void *, uint64_t, uint64_t, uint64_t, uint64_t);
void sb_shell_init(sb_shell_state_t *state);
/* Navigation uses first make only; releases/repeats and Ctrl/Alt/Meta chords
 * never activate actions. Overflow redraws a persistent recovery notice. */
int sb_shell_event(sb_shell_state_t *state, const sb_key_event_t *event);
/* Bounded, read-only directory snapshot. Every successfully opened handle is
 * closed, including malformed entries, read errors and truncation. */
int sb_shell_load(sb_shell_listing_t *listing, unsigned disk, sb_shell_call_t call, void *context);
size_t sb_shell_decimal(char *output, uint64_t value);
int sb_shell_render(const sb_text_renderer_t *renderer, const sb_shell_state_t *state,
                    const sb_shell_listing_t *listing, unsigned keyboard_available);
int sb_desktop_shell(void);
#endif
