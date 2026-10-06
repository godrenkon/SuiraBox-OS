#include "shell.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { unsigned entries, reads, closes, bad_at; int open_error, read_error, close_error; } fixture_t;
static int64_t call(void *context,uint64_t n,uint64_t a,uint64_t b,uint64_t c) {
    fixture_t *f=context; assert(c==0u);
    if(n==SB_SYS_DIRECTORY_OPEN) { assert(b==5u && (!memcmp((void *)(uintptr_t)a,"/boot",5u) || !memcmp((void *)(uintptr_t)a,"/disk",5u))); return f->open_error?f->open_error:123; }
    assert(a==123u);
    if(n==SB_SYS_HANDLE_CLOSE) { ++f->closes; return f->close_error; }
    assert(n==SB_SYS_DIRECTORY_READ);
    unsigned index=f->reads++;
    if(f->read_error && index==1u) return f->read_error;
    if(index>=f->entries) return SB_SYS_ERROR_NOT_FOUND;
    sb_directory_entry_t *e=(void *)(uintptr_t)b;
    memset(e,0xff,sizeof(*e)); /* Non-name padding must never be interpreted. */
    e->type=1u; e->reserved=0u; e->size=UINT64_MAX; e->name_length=3u;
    memcpy(e->name,"a\nb",3u);
    if(f->bad_at==index+1u) e->name_length=64u;
    return 0;
}
static sb_key_event_t key(unsigned code,unsigned flags,unsigned mods) {
    return (sb_key_event_t){.size=SB_KEY_EVENT_SIZE,.version=SB_INPUT_ABI_VERSION,.type=SB_INPUT_EVENT_KEY,
        .keycode=(uint16_t)code,.flags=(uint16_t)flags,.modifiers=mods};
}
int main(void) {
    sb_shell_state_t s; sb_shell_init(&s); sb_key_event_t e=key((SB_KEY_F1+1u),1u,0u);
    assert(sb_shell_event(&s,&e)==SB_SHELL_RELOAD && s.view==SB_SHELL_FILES);
    e=key('D',1u,0u); assert(sb_shell_event(&s,&e)==SB_SHELL_RELOAD && s.disk==1u);
    e.flags=0u; assert(sb_shell_event(&s,&e)==0);
    e=key('R',3u,0u); assert(sb_shell_event(&s,&e)==0);
    e=key('R',1u,SB_KEY_MOD_CTRL); assert(sb_shell_event(&s,&e)==0);
    e=key('R',1u,0u); assert(sb_shell_event(&s,&e)==SB_SHELL_RELOAD);
    e=key((SB_KEY_F1+2u),1u,0u); assert(sb_shell_event(&s,&e)==SB_SHELL_REDRAW && s.view==2u);
    e=key(SB_KEY_TAB,1u,0u); assert(sb_shell_event(&s,&e)==SB_SHELL_REDRAW && s.view==0u);
    e.modifiers=SB_KEY_MOD_SHIFT; assert(sb_shell_event(&s,&e)==SB_SHELL_REDRAW && s.view==2u);
    e=key(SB_KEY_ESCAPE,1u,0u); assert(sb_shell_event(&s,&e)==SB_SHELL_REDRAW && s.view==0u && s.disk==1u);
    e=key('B',1u,0u); assert(sb_shell_event(&s,&e)==0 && s.disk==1u);
    e.type=SB_INPUT_EVENT_OVERFLOW; e.keycode=0u; e.flags=0u; e.dropped=4u;
    assert(sb_shell_event(&s,&e)==SB_SHELL_REDRAW && s.overflow);
    e.dropped=0u; assert(sb_shell_event(&s,&e)==-1);
    e=key((SB_KEY_F1+1u),SB_KEY_REPEAT,0u); assert(sb_shell_event(&s,&e)==-1 && s.view==0u);
    e=key((SB_KEY_F1+1u),1u,0u); e.reserved=1u; assert(sb_shell_event(&s,&e)==-1);
    e.reserved=0u; e.dropped=1u; assert(sb_shell_event(&s,&e)==-1);
    sb_shell_listing_t l; fixture_t f={.entries=2u};
    assert(sb_shell_load(&l,0u,call,&f)==0 && l.count==2u && !l.error && !l.truncated && f.closes==1u && f.reads==3u);
    assert(l.entries[0].name[3]==0 && l.entries[0].size==UINT64_MAX);
    f=(fixture_t){.entries=12u}; sb_shell_load(&l,1u,call,&f); assert(l.count==12u && !l.truncated && f.reads==13u && f.closes==1u);
    f=(fixture_t){.entries=100u}; sb_shell_load(&l,1u,call,&f); assert(l.count==12u && l.truncated && f.reads==13u && f.closes==1u);
    f=(fixture_t){0}; sb_shell_load(&l,0u,call,&f); assert(!l.count && !l.error && f.closes==1u);
    f=(fixture_t){.open_error=SB_SYS_ERROR_NOT_FOUND}; sb_shell_load(&l,0u,call,&f); assert(l.error==-6 && !f.closes && !f.reads);
    f=(fixture_t){.entries=3u,.read_error=SB_SYS_ERROR_IO}; sb_shell_load(&l,0u,call,&f); assert(l.error==-7 && !l.count && f.closes==1u);
    f=(fixture_t){.entries=3u,.bad_at=2u}; sb_shell_load(&l,0u,call,&f); assert(l.error==-1 && !l.count && f.closes==1u);
    f=(fixture_t){.close_error=SB_SYS_ERROR_STALE}; sb_shell_load(&l,0u,call,&f); assert(l.error==-4 && !l.count && f.closes==1u);
    assert(sb_shell_load(&l,2u,call,&f)==-1);
    char decimal[21]; size_t n=sb_shell_decimal(decimal,UINT64_MAX); decimal[n]=0;
    assert(n==20u && !strcmp(decimal,"18446744073709551615"));
    n=sb_shell_decimal(decimal,0u); assert(n==1u && decimal[0]=='0');
    puts("desktop shell model host test OK: navigation, modifiers, overflow, bounded snapshots and close on every exit");
    return 0;
}
