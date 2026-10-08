#ifndef SB_USERSPACE_SHELL_H
#define SB_USERSPACE_SHELL_H
#include "text.h"
#include <suirabox/syscall_abi.h>

#define SB_SHELL_ROWS 12u
#define SB_SHELL_PREVIEW_BYTES 512u
#define SB_SHELL_PREVIEW_COLUMNS 48u
enum { SB_SHELL_HOME, SB_SHELL_FILES, SB_SHELL_SETTINGS };
enum { SB_SHELL_NONE, SB_SHELL_REDRAW, SB_SHELL_RELOAD, SB_SHELL_OPEN, SB_SHELL_PARENT };
typedef struct {
    unsigned view, disk, overflow, selected, preview, path_length;
    int error;
    char path[SB_SYS_PATH_MAX+1u];
} sb_shell_state_t;
typedef struct { unsigned length, more; unsigned char bytes[SB_SHELL_PREVIEW_BYTES]; } sb_shell_preview_t;
typedef struct {
    unsigned count, truncated;
    int error;
    sb_directory_entry_t entries[SB_SHELL_ROWS];
    sb_shell_preview_t content;
} sb_shell_listing_t;
typedef int64_t (*sb_shell_call_t)(void *, uint64_t, uint64_t, uint64_t, uint64_t);
void sb_shell_init(sb_shell_state_t *state);
/* Navigation uses first make only; releases/repeats and Ctrl/Alt/Meta chords
 * never activate actions. Overflow redraws a persistent recovery notice. */
int sb_shell_event(sb_shell_state_t *state, const sb_key_event_t *event);
int sb_shell_browser_event(sb_shell_state_t *state, const sb_shell_listing_t *listing, const sb_key_event_t *event);
int sb_shell_browser_action(sb_shell_state_t *state, sb_shell_listing_t *listing, int action,
                            sb_shell_call_t call, void *context);
int sb_shell_load_path(sb_shell_listing_t *listing, const char *path, size_t length, sb_shell_call_t call, void *context);
unsigned sb_shell_preview_lines(const sb_shell_preview_t *preview,
                                char rows[SB_SHELL_ROWS][SB_SHELL_PREVIEW_COLUMNS+1u], unsigned *clipped);
/* Bounded, read-only directory snapshot. Every successfully opened handle is
 * closed, including malformed entries, read errors and truncation. */
int sb_shell_load(sb_shell_listing_t *listing, unsigned disk, sb_shell_call_t call, void *context);
size_t sb_shell_decimal(char *output, uint64_t value);
int sb_shell_render(const sb_text_renderer_t *renderer, const sb_shell_state_t *state,
                    const sb_shell_listing_t *listing, unsigned keyboard_available);
int sb_desktop_shell(void);
#endif
