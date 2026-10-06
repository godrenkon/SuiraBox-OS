#ifndef SB_DISPLAY_CLIENT_H
#define SB_DISPLAY_CLIENT_H
#include "text.h"
/* Initialize a text renderer over DISPLAY_PRESENT; 1 means no mapped display. */
int sb_text_display_setup(sb_text_renderer_t *renderer);
#endif
