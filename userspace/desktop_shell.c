#include "shell.h"
#include "display_client.h"
#include "syscall_client.h"

static int64_t syscall(void *context,uint64_t n,uint64_t a,uint64_t b,uint64_t c) {
    (void)context; return sb_user_call3(n,a,b,c);
}
static int message(const char *text,size_t n) {
    return sb_user_call3(SB_SYS_LOG_WRITE,(uintptr_t)text,n,0u)==(int64_t)n?0:-1;
}
static int frame(unsigned number,const sb_shell_state_t *s) {
    char log[256]; size_t n=23u;
    static const char prefix[]="Userspace: shell frame ";
    for(unsigned i=0u;i<23u;++i) log[i]=prefix[i];
    n+=sb_shell_decimal(log+n,number); log[n++]=' ';
    const char *name=s->view==SB_SHELL_HOME?"Home":s->view==SB_SHELL_SETTINGS?"Settings":"Files ";
    while(*name) log[n++]=*name++;
    if(s->view==SB_SHELL_FILES) {
        /* Bounded diagnostics even when a valid VFS path fills all 255 bytes. */
        unsigned limit=s->path_length<180u?s->path_length:180u;
        for(unsigned i=0u;i<limit;++i) {
            unsigned char c=(unsigned char)s->path[i]; log[n++]=c>=32u && c<=126u?(char)c:'?';
        }
        if(s->preview) { static const char suffix[]=" preview"; for(unsigned i=0u;i<sizeof(suffix)-1u;++i) log[n++]=suffix[i]; }
    }
    log[n++]='\r'; log[n++]='\n'; return message(log,n);
}
int sb_desktop_shell(void) {
    sb_text_renderer_t r; const int setup=sb_text_display_setup(&r);
    if(setup==1) return 0;
    if(setup!=0) return -1;
    const int64_t probe=sb_user_call3(SB_SYS_KEY_EVENT_READ,1u,SB_KEY_EVENT_SIZE,0u);
    if(probe!=SB_SYS_ERROR_FAULT && probe!=SB_SYS_ERROR_NOT_FOUND) return -1;
    const unsigned keyboard=probe==SB_SYS_ERROR_FAULT;
    sb_shell_state_t state; sb_shell_init(&state);
    /* A bounded static snapshot avoids consuming the small initial user stack. */
    static sb_shell_listing_t listing;
    unsigned number=1u;
    if(sb_shell_render(&r,&state,&listing,keyboard)!=0 || frame(number,&state)!=0) return -1;
    static const char ready[]="Userspace: shell ready\r\n";
    if(message(ready,sizeof(ready)-1u)!=0) return -1;
    if(!keyboard) return 0;
    for(;;) {
        sb_key_event_t event;
        const int64_t result=sb_user_call3(SB_SYS_KEY_EVENT_READ,(uintptr_t)&event,sizeof(event),0u);
        if(result==SB_SYS_ERROR_WOULD_BLOCK) {
            if(sb_user_call3(SB_SYS_SLEEP,1u,0u,0u)!=0) return -1;
            continue;
        }
        if(result!=0) return -1;
        const int action=sb_shell_browser_event(&state,&listing,&event);
        if(action<0) return -1;
        if(action==SB_SHELL_NONE) continue;
        if(sb_shell_browser_action(&state,&listing,action,syscall,0)!=0) return -1;
        if(sb_shell_render(&r,&state,&listing,keyboard)!=0 || frame(++number,&state)!=0) return -1;
    }
}
