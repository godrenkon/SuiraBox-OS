#ifndef SB_PROFILE_H
#define SB_PROFILE_H

/*
 * Select one profile at build time with -DSB_PROFILE_<NAME>.
 * The default profile is desktop so a generic build remains useful.
 */
#if defined(SB_PROFILE_MINECRAFT)
#define SB_PROFILE_NAME "minecraft"
#elif defined(SB_PROFILE_GAMING)
#define SB_PROFILE_NAME "gaming"
#elif defined(SB_PROFILE_WORKSTATION)
#define SB_PROFILE_NAME "workstation"
#elif defined(SB_PROFILE_SERVER)
#define SB_PROFILE_NAME "server"
#elif defined(SB_PROFILE_DATACENTER)
#define SB_PROFILE_NAME "datacenter"
#else
#define SB_PROFILE_NAME "desktop"
#endif

#endif
