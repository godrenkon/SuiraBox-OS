#include "shell.h"
#include "key_decoder.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned entries, opens, acquired, closes, reads, file_offset, bad_at, fail_at;
    int open_error, close_error;
    char path[256], payload[280];
} fixture_t;
static int64_t call(void *context,uint64_t n,uint64_t a,uint64_t b,uint64_t c) {
    fixture_t *f=context;
    if(n==SB_SYS_DIRECTORY_OPEN || n==SB_SYS_FILE_OPEN) {
        assert(b<=255u); memcpy(f->path,(const void *)(uintptr_t)a,(size_t)b); f->path[b]=0;
        assert(c==(n==SB_SYS_FILE_OPEN?SB_FILE_ACCESS_READ:0u));
        ++f->opens; f->reads=f->file_offset=0u;
        if(f->open_error) return f->open_error;
        ++f->acquired;
        if(n==SB_SYS_FILE_OPEN) { snprintf(f->payload,sizeof(f->payload),"Read %s\n",f->path); return 321; }
        return 123;
    }
    if(n==SB_SYS_HANDLE_CLOSE) { assert(a==123u || a==321u); ++f->closes; return f->close_error; }
    if(n==SB_SYS_FILE_READ) {
        assert(a==321u && c && c<=256u);
        unsigned remaining=(unsigned)strlen(f->payload)-f->file_offset;
        unsigned amount=remaining<c?remaining:(unsigned)c;
        memcpy((void *)(uintptr_t)b,f->payload+f->file_offset,amount); f->file_offset+=amount;
        return amount;
    }
    assert(n==SB_SYS_DIRECTORY_READ && a==123u && !c); ++f->reads;
    if(f->reads==f->fail_at) return SB_SYS_ERROR_IO;
    if(f->reads>f->entries) return SB_SYS_ERROR_NOT_FOUND;
    sb_directory_entry_t *entry=(void *)(uintptr_t)b;
    memset(entry,0xff,sizeof(*entry)); entry->type=SB_DIRECTORY_ENTRY_TYPE_REGULAR;
    entry->reserved=0u; entry->size=f->reads; entry->name_length=10u;
    assert(snprintf(entry->name,sizeof(entry->name),"FILE%02u.TXT",f->reads)==10);
    if(f->reads==f->bad_at) entry->name_length=64u;
    return 0;
}
static int press(sb_shell_state_t *s,sb_shell_listing_t *l,unsigned char scan) {
    sb_key_decoder_t decoder; sb_key_decoder_init(&decoder); sb_key_event_t e;
    sb_key_decoder_feed(&decoder,0xe0); sb_key_decoder_feed(&decoder,scan);
    assert(sb_key_decoder_peek(&decoder,&e)); sb_key_decoder_pop(&decoder);
    int action=sb_shell_browser_event(s,l,&e);
    sb_key_decoder_feed(&decoder,0xe0); sb_key_decoder_feed(&decoder,(unsigned char)(scan|0x80u));
    assert(sb_key_decoder_peek(&decoder,&e));
    assert(sb_shell_browser_event(s,l,&e)==SB_SHELL_NONE);
    return action;
}
static int key(sb_shell_state_t *s,sb_shell_listing_t *l,unsigned code) {
    sb_key_event_t e={.size=32u,.version=1u,.type=SB_INPUT_EVENT_KEY,.keycode=(uint16_t)code,.flags=SB_KEY_DOWN};
    return sb_shell_browser_event(s,l,&e);
}
static void apply(sb_shell_state_t *s,sb_shell_listing_t *l,fixture_t *f,int action) {
    assert(sb_shell_browser_action(s,l,action,call,f)==0);
}
static void start(sb_shell_state_t *s,sb_shell_listing_t *l,fixture_t *f,unsigned entries) {
    sb_shell_init(s); s->view=SB_SHELL_FILES; *f=(fixture_t){.entries=entries};
    apply(s,l,f,SB_SHELL_RELOAD);
    assert(f->acquired==f->closes && !l->error && !s->offset);
}
int main(void) {
    sb_shell_state_t s; sb_shell_listing_t l; fixture_t f;
    start(&s,&l,&f,27u);
    assert(l.count==12u && l.truncated && f.reads==13u && !strcmp(l.entries[0].name,"FILE01.TXT"));
    assert(press(&s,&l,0x49)==SB_SHELL_NONE); /* PgUp at the beginning. */
    int action=press(&s,&l,0x51); assert(action==SB_SHELL_PAGE_NEXT); apply(&s,&l,&f,action);
    assert(s.offset==12u && !s.selected && l.count==12u && l.truncated && f.reads==25u);
    assert(!strcmp(l.entries[0].name,"FILE13.TXT") && f.acquired==f.closes);
    apply(&s,&l,&f,key(&s,&l,SB_KEY_ENTER));
    assert(s.preview && !strcmp(f.path,"/boot/FILE13.TXT") &&
           l.content.length==strlen(f.payload) && !memcmp(l.content.bytes,f.payload,l.content.length));
    assert(press(&s,&l,0x51)==SB_SHELL_NONE && s.offset==12u); /* Preview cannot page the hidden list. */
    apply(&s,&l,&f,key(&s,&l,SB_KEY_ESCAPE)); assert(!s.preview && s.offset==12u && !s.selected);
    apply(&s,&l,&f,press(&s,&l,0x51));
    assert(s.offset==24u && l.count==3u && !l.truncated && f.reads==28u);
    assert(!strcmp(l.entries[0].name,"FILE25.TXT"));
    assert(press(&s,&l,0x51)==SB_SHELL_NONE);
    apply(&s,&l,&f,press(&s,&l,0x50)); apply(&s,&l,&f,press(&s,&l,0x50));
    assert(s.selected==2u && press(&s,&l,0x50)==SB_SHELL_NONE);
    apply(&s,&l,&f,press(&s,&l,0x49)); assert(s.offset==12u && !s.selected);
    action=press(&s,&l,0x48); assert(action==SB_SHELL_PAGE_PREV_LAST); apply(&s,&l,&f,action);
    assert(!s.offset && s.selected==11u && !strcmp(l.entries[s.selected].name,"FILE12.TXT"));
    action=press(&s,&l,0x50); assert(action==SB_SHELL_PAGE_NEXT); apply(&s,&l,&f,action);
    assert(s.offset==12u && !s.selected);
    apply(&s,&l,&f,key(&s,&l,SB_KEY_F1)); assert(s.view==SB_SHELL_HOME && s.offset==12u);
    apply(&s,&l,&f,key(&s,&l,SB_KEY_F1+1u)); assert(s.view==SB_SHELL_FILES && s.offset==12u);
    apply(&s,&l,&f,key(&s,&l,'R')); assert(s.offset==12u && !strcmp(l.entries[0].name,"FILE13.TXT"));
    apply(&s,&l,&f,key(&s,&l,'B')); assert(!s.offset && !s.selected && !strcmp(l.entries[0].name,"FILE01.TXT"));
    assert(f.acquired==f.closes);

    /* Failed/vanished pages never replace the visible snapshot or selection. */
    start(&s,&l,&f,27u); s.selected=11u; f.fail_at=15u;
    apply(&s,&l,&f,SB_SHELL_PAGE_NEXT);
    assert(s.error==SB_SYS_ERROR_IO && !s.offset && s.selected==11u && l.count==12u && !l.error && f.acquired==f.closes);
    f.fail_at=0u; f.bad_at=5u; apply(&s,&l,&f,SB_SHELL_PAGE_NEXT);
    assert(s.error==SB_SYS_ERROR_INVALID && !s.offset && s.selected==11u && f.acquired==f.closes);
    f.bad_at=0u; f.close_error=SB_SYS_ERROR_STALE; apply(&s,&l,&f,SB_SHELL_PAGE_NEXT);
    assert(s.error==SB_SYS_ERROR_STALE && !s.offset && s.selected==11u && f.acquired==f.closes);
    f.close_error=0; f.open_error=SB_SYS_ERROR_NOT_FOUND; unsigned closed=f.closes;
    apply(&s,&l,&f,SB_SHELL_PAGE_NEXT);
    assert(s.error==SB_SYS_ERROR_NOT_FOUND && !s.offset && s.selected==11u && f.closes==closed);
    f.open_error=0;
    f.close_error=0; f.entries=5u; apply(&s,&l,&f,SB_SHELL_PAGE_NEXT);
    assert(s.error==SB_SYS_ERROR_NOT_FOUND && !s.offset && s.selected==11u && l.count==12u);
    apply(&s,&l,&f,SB_SHELL_RELOAD); assert(!s.error && l.count==5u && !l.truncated && !s.selected);
    start(&s,&l,&f,27u); apply(&s,&l,&f,SB_SHELL_PAGE_NEXT); f.entries=5u;
    unsigned closes=f.closes; apply(&s,&l,&f,SB_SHELL_RELOAD);
    assert(!s.offset && l.count==5u && !s.selected && f.closes==closes+2u);

    /* A full final page has EOF, rather than a spurious next-page action. */
    start(&s,&l,&f,12u); s.selected=11u;
    assert(!l.truncated && press(&s,&l,0x50)==SB_SHELL_NONE && press(&s,&l,0x51)==SB_SHELL_NONE);
    start(&s,&l,&f,13u); apply(&s,&l,&f,SB_SHELL_PAGE_NEXT);
    assert(s.offset==12u && l.count==1u && !l.truncated && !strcmp(l.entries[0].name,"FILE13.TXT"));
    strcpy(s.path,"/boot/MANY"); s.path_length=10u;
    apply(&s,&l,&f,SB_SHELL_PARENT); assert(!strcmp(s.path,"/boot") && !s.offset);
    start(&s,&l,&f,27u); s.offset=UINT32_MAX/12u*12u;
    unsigned opens=f.opens; apply(&s,&l,&f,SB_SHELL_PAGE_NEXT);
    assert(s.error==SB_SYS_ERROR_LIMIT && f.opens==opens);
    s.offset=1u;
    assert(key(&s,&l,SB_KEY_PAGE_DOWN)==-1 && sb_shell_browser_action(&s,&l,SB_SHELL_PAGE_NEXT,call,&f)==-1);
    puts("Files paging host test OK: decoded Page/arrow keys, >12 entries, preview identity, EOF, shrink and failure recovery");
    return 0;
}
