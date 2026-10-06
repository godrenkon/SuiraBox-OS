#include "display_client.h"
#include "syscall_client.h"
#include "line_edit.h"
#include <suirabox/syscall_abi.h>
static int message(const char *text, size_t length) {
    return sb_user_call3(SB_SYS_LOG_WRITE,(uintptr_t)text,length,0u)==(int64_t)length ? 0 : -1;
}
static int read_event(sb_key_event_t *event) {
    return (int)sb_user_call3(SB_SYS_KEY_EVENT_READ,(uintptr_t)event,SB_KEY_EVENT_SIZE,0u);
}
static int repaint(sb_text_renderer_t *r, const sb_line_edit_t *e, int submitted) {
    char line[SB_LINE_EDIT_CAPACITY+2u]; line[0]='>'; line[1]=' ';
    for(unsigned i=0u;i<SB_LINE_EDIT_CAPACITY;++i) line[i+2u]=i<e->length ? e->text[i] : ' ';
    if (sb_text_draw(r,24,112,line,sizeof(line))!=0) return -1;
    if (submitted) {
        static const char prefix[]="Last submitted: ";
        char status[sizeof(prefix)-1u+SB_LINE_EDIT_CAPACITY];
        for(unsigned i=0u;i<sizeof(prefix)-1u;++i) status[i]=prefix[i];
        for(unsigned i=0u;i<SB_LINE_EDIT_CAPACITY;++i)
            status[sizeof(prefix)-1u+i]=i<e->submitted_length ? e->submitted[i] : ' ';
        r->foreground=0x259b72u;
        const int result=sb_text_draw(r,24,144,status,sizeof(status));
        r->foreground=0xe6eef2u;
        if (result!=0) return -1;
    }
    return 0;
}
#ifdef SB_INPUT_PROOF
int sb_keyboard_proof_event(const sb_key_event_t *e, unsigned index) {
    static const uint16_t codes[]={SB_KEY_LEFT_SHIFT,'A','A',SB_KEY_LEFT_SHIFT,'B','B',SB_KEY_BACKSPACE,SB_KEY_BACKSPACE,
        SB_KEY_CAPS_LOCK,SB_KEY_CAPS_LOCK,'C','C',SB_KEY_CAPS_LOCK,SB_KEY_CAPS_LOCK,SB_KEY_RIGHT,SB_KEY_RIGHT,
        SB_KEY_RIGHT_CTRL,SB_KEY_RIGHT_CTRL,SB_KEY_RIGHT_ALT,SB_KEY_RIGHT_ALT,SB_KEY_ENTER,SB_KEY_ENTER};
    static const uint16_t flags[]={1,1,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
    static const uint32_t mods[]={1,1,1,0,0,0,0,0,8,8,8,8,0,0,0,0,2,0,4,0,0,0};
    if (index>=sizeof(codes)/sizeof(codes[0]) || e->size!=SB_KEY_EVENT_SIZE || e->version!=SB_INPUT_ABI_VERSION ||
        e->type!=SB_INPUT_EVENT_KEY || e->keycode!=codes[index] || e->flags!=flags[index] || e->modifiers!=mods[index] ||
        e->sequence!=index+1u || e->dropped || e->reserved) return -1;
    char text[]="Userspace: keyboard event 00 OK\r\n";
    text[26]=(char)('0'+(index+1u)/10u); text[27]=(char)('0'+(index+1u)%10u);
    return message(text,sizeof(text)-1u);
}
#endif
int sb_keyboard_demo(void) {
    sb_text_renderer_t r;
    const int setup=sb_text_display_setup(&r);
    if (setup==1) return 0;
    if (setup!=0) return -1;
    sb_key_event_t event;
    /* Validate availability before exposing a keyboard prompt. No event is
     * consumed: invalid output must fail even if the queue is empty. */
    const int64_t probe=sb_user_call3(SB_SYS_KEY_EVENT_READ,1u,SB_KEY_EVENT_SIZE,0u);
    if (probe==SB_SYS_ERROR_NOT_FOUND) return 0;
    if (probe!=SB_SYS_ERROR_FAULT) return -1;
    sb_line_edit_t editor; sb_line_edit_init(&editor);
    static const char hint[]="Keyboard ready: type, Backspace, Enter";
    if (sb_text_draw(&r,24,80,hint,sizeof(hint)-1u)!=0 || repaint(&r,&editor,0)!=0) return -1;
    static const char ready[]="Userspace: keyboard demo ready\r\n";
    if (message(ready,sizeof(ready)-1u)!=0) return -1;
#ifdef SB_INPUT_PROOF
    unsigned index=0u;
    const uint64_t deadline=(uint64_t)sb_user_call3(SB_SYS_GET_TICKS,0u,0u,0u)+1500u;
#endif
    for (;;) {
#ifdef SB_INPUT_PROOF
        /* Run fault/argument probes while the first event is queued; its
         * sequence/key must survive and still be read as the first shift make. */
        if (!index) {
            if (sb_user_call3(SB_SYS_KEY_EVENT_READ,1u,SB_KEY_EVENT_SIZE,0u)!=SB_SYS_ERROR_FAULT ||
                sb_user_call3(SB_SYS_KEY_EVENT_READ,(uintptr_t)sb_keyboard_demo,SB_KEY_EVENT_SIZE,0u)!=SB_SYS_ERROR_FAULT ||
                sb_user_call3(SB_SYS_KEY_EVENT_READ,(uintptr_t)&event,31u,0u)!=SB_SYS_ERROR_INVALID ||
                sb_user_call3(SB_SYS_KEY_EVENT_READ,(uintptr_t)&event,SB_KEY_EVENT_SIZE,1u)!=SB_SYS_ERROR_INVALID) return -1;
        }
#endif
        const int status=read_event(&event);
        if (status==SB_SYS_ERROR_WOULD_BLOCK) {
#ifdef SB_INPUT_PROOF
            if ((uint64_t)sb_user_call3(SB_SYS_GET_TICKS,0u,0u,0u)>deadline) return -1;
#endif
            if (sb_user_call3(SB_SYS_SLEEP,1u,0u,0u)!=0) return -1;
            continue;
        }
        if (status!=0) return -1;
#ifdef SB_INPUT_PROOF
        if (sb_keyboard_proof_event(&event,index++)!=0) return -1;
#endif
        const int changed=sb_line_edit_event(&editor,&event);
        if (changed<0) return -1;
        if (changed==SB_LINE_OVERFLOW) {
            static const char overflow[]="Input overflow: key state resynchronized";
            if (sb_text_draw(&r,24,176,overflow,sizeof(overflow)-1u)!=0) return -1;
        } else if (changed && repaint(&r,&editor,changed==SB_LINE_SUBMITTED)!=0) return -1;
#ifdef SB_INPUT_PROOF
        if (index==22u) {
            if (editor.length || editor.submitted_length!=2u || editor.submitted[0]!='A' || editor.submitted[1]!='C') return -1;
            if (read_event(&event)!=SB_SYS_ERROR_WOULD_BLOCK) return -1;
            static const char done[]="Userspace: PS/2 IRQ keyboard editing and rejection lifecycle OK\r\n";
            return message(done,sizeof(done)-1u);
        }
#endif
    }
}
