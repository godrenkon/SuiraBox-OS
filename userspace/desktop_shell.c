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
    char log[96]="Userspace: shell frame "; size_t n=23u;
    n+=sb_shell_decimal(log+n,number); log[n++]=' ';
    const char *name=s->view==SB_SHELL_HOME?"Home":s->view==SB_SHELL_SETTINGS?"Settings":s->disk?"Files /disk":"Files /boot";
    while(*name) log[n++]=*name++;
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
        const int action=sb_shell_event(&state,&event);
        if(action<0) return -1;
        if(action==SB_SHELL_NONE) continue;
        if(action==SB_SHELL_RELOAD && sb_shell_load(&listing,state.disk,syscall,0)!=0) return -1;
        if(sb_shell_render(&r,&state,&listing,keyboard)!=0 || frame(++number,&state)!=0) return -1;
    }
}
