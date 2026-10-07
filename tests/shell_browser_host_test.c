#include "shell.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned opens, closes, reads, length, offset, short_read, invalid_read, fail_read;
    int open_error, close_error, directory_error;
    char opened[256]; unsigned char data[600];
} fixture_t;
static int64_t call(void *context,uint64_t n,uint64_t a,uint64_t b,uint64_t c) {
    fixture_t *f=context;
    if(n==SB_SYS_FILE_OPEN || n==SB_SYS_DIRECTORY_OPEN) {
        assert(b<=255u); memcpy(f->opened,(void *)(uintptr_t)a,(size_t)b); f->opened[b]=0;
        assert(c==(n==SB_SYS_FILE_OPEN?SB_FILE_ACCESS_READ:0u)); ++f->opens; f->offset=f->reads=0u;
        if(f->open_error) return f->open_error;
        return n==SB_SYS_FILE_OPEN?321:123;
    }
    if(n==SB_SYS_HANDLE_CLOSE) { assert(a==123u || a==321u); ++f->closes; return f->close_error; }
    if(n==SB_SYS_DIRECTORY_READ) {
        assert(a==123u && c==0u); ++f->reads;
        if(f->directory_error) return f->directory_error;
        if(f->reads>1u) return SB_SYS_ERROR_NOT_FOUND;
        sb_directory_entry_t *entry=(void *)(uintptr_t)b;
        *entry=(sb_directory_entry_t){.type=1u,.name_length=9u,.size=3u}; memcpy(entry->name,"LEVEL.TXT",9u); return 0;
    }
    assert(n==SB_SYS_FILE_READ && a==321u && c && c<=256u); ++f->reads;
    if(f->fail_read==f->reads) return SB_SYS_ERROR_IO;
    if(f->invalid_read==f->reads) return (int64_t)c+1;
    unsigned amount=f->length-f->offset;
    if(amount>c) amount=(unsigned)c;
    if(f->short_read && amount>7u) amount=7u;
    memcpy((void *)(uintptr_t)b,f->data+f->offset,amount); f->offset+=amount; return amount;
}
static sb_key_event_t key(unsigned k) { return (sb_key_event_t){.size=32u,.version=1u,.type=1u,.keycode=(uint16_t)k,.flags=1u}; }
static void list(sb_shell_state_t *s,sb_shell_listing_t *l,unsigned type,const char *name) {
    sb_shell_init(s); s->view=SB_SHELL_FILES; memset(l,0,sizeof(*l)); l->count=1u;
    l->entries[0].type=type; l->entries[0].name_length=(uint16_t)strlen(name); memcpy(l->entries[0].name,name,strlen(name));
}
static int event(sb_shell_state_t *s,sb_shell_listing_t *l,unsigned k) {
    sb_key_event_t e=key(k); return sb_shell_browser_event(s,l,&e);
}
int main(void) {
    sb_shell_state_t s; sb_shell_listing_t l; fixture_t f={0};
    list(&s,&l,2u,"SAVES");
    assert(event(&s,&l,SB_KEY_ENTER)==SB_SHELL_OPEN);
    assert(sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f)==0);
    assert(!strcmp(s.path,"/boot/SAVES") && !strcmp(f.opened,s.path) && f.closes==1u && l.count==1u);
    assert(event(&s,&l,SB_KEY_BACKSPACE)==SB_SHELL_PARENT);
    assert(sb_shell_browser_action(&s,&l,SB_SHELL_PARENT,call,&f)==0 && !strcmp(s.path,"/boot") && f.closes==2u);
    assert(event(&s,&l,SB_KEY_BACKSPACE)==SB_SHELL_NONE);
    list(&s,&l,2u,"SAVES"); f=(fixture_t){.directory_error=SB_SYS_ERROR_IO};
    sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.error==-7 && !strcmp(s.path,"/boot") && !strcmp(l.entries[0].name,"SAVES") && f.closes==1u);
    list(&s,&l,1u,"WORLD.TXT"); f=(fixture_t){.length=600u};
    for(unsigned i=0u;i<600u;++i) f.data[i]=(unsigned char)i;
    sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.preview && l.content.length==512u && l.content.more && f.offset==513u && f.closes==1u && !memcmp(l.content.bytes,f.data,512u));
    assert(event(&s,&l,SB_KEY_ESCAPE)==SB_SHELL_REDRAW && !s.preview && s.view==SB_SHELL_FILES);
    f=(fixture_t){.length=512u}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.preview && l.content.length==512u && !l.content.more && f.closes==1u);
    assert(event(&s,&l,SB_KEY_BACKSPACE)==SB_SHELL_REDRAW && !s.preview);
    f=(fixture_t){.length=23u,.short_read=1u}; memcpy(f.data,"SuiraBox runtime file\r\n",23u);
    sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.preview && l.content.length==23u && !l.content.more && f.reads==5u && f.closes==1u);
    f=(fixture_t){0}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.preview && !l.content.length && !l.content.more && f.closes==1u);
    f=(fixture_t){.open_error=-6}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.error==-6 && !s.preview && !f.closes);
    f=(fixture_t){.length=600u,.fail_read=2u}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.error==-7 && !s.preview && !l.content.length && f.closes==1u);
    f=(fixture_t){.length=600u,.fail_read=3u}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.error==-7 && !s.preview && !l.content.length && f.closes==1u);
    f=(fixture_t){.invalid_read=1u}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.error==-1 && !s.preview && f.closes==1u);
    f=(fixture_t){.close_error=-4}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
    assert(s.error==-4 && !s.preview && f.closes==1u);
    const char *bad[]={".","..","x/y","x\\y","bad\nname"};
    for(unsigned i=0u;i<sizeof(bad)/sizeof(bad[0]);++i) {
        list(&s,&l,1u,bad[i]); f=(fixture_t){0}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f);
        assert(s.error==-1 && !f.opens);
    }
    list(&s,&l,3u,"DEVICE"); f=(fixture_t){0}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f); assert(s.error==-5 && !f.opens);
    list(&s,&l,1u,"WORLD.TXT"); s.path_length=250u; memset(s.path+5u,'a',245u); s.path[5]='/'; s.path[250]=0;
    f=(fixture_t){0}; sb_shell_browser_action(&s,&l,SB_SHELL_OPEN,call,&f); assert(s.error==-1 && !f.opens);
    assert(event(&s,&l,'B')==SB_SHELL_RELOAD && !strcmp(s.path,"/boot"));
    l.count=2u; l.entries[1]=l.entries[0];
    assert(event(&s,&l,SB_KEY_DOWN)==SB_SHELL_REDRAW && s.selected==1u);
    assert(event(&s,&l,SB_KEY_DOWN)==SB_SHELL_NONE);
    sb_key_event_t e=key(SB_KEY_UP); e.flags|=SB_KEY_REPEAT;
    assert(sb_shell_browser_event(&s,&l,&e)==SB_SHELL_NONE && s.selected==1u);
    assert(event(&s,&l,SB_KEY_UP)==SB_SHELL_REDRAW && s.selected==0u);
    char rows[12][49]; unsigned clipped;
    sb_shell_preview_t p={.length=10u}; memcpy(p.bytes,"A\r\nB\tC\0\xffZ",10u);
    assert(sb_shell_preview_lines(&p,rows,&clipped)==2u && !clipped);
    assert(rows[0][0]=='A' && !memcmp(rows[1],"B   C..Z.",9u));
    p.length=512u; memset(p.bytes,'\t',512u);
    assert(sb_shell_preview_lines(&p,rows,&clipped)==12u && clipped);
    p.length=0u; assert(sb_shell_preview_lines(&p,rows,&clipped)==0u && !clipped);
    puts("Files browser host test OK: traversal, root limits, selection, short/empty/binary previews, truncation and failure cleanup");
    return 0;
}
