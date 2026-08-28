#ifndef SUIRABOX_DESKTOP_BOOTSTRAP_H
#define SUIRABOX_DESKTOP_BOOTSTRAP_H

/* Transitional graphical bootstrap renderer.
 * This is intentionally below the future userspace compositor boundary.
 * It proves that the display path can present a deterministic desktop-like
 * surface before the full GUI service/window system exists.
 */
int sb_desktop_bootstrap_render(void);

#endif
