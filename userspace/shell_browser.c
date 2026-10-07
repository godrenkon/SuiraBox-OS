#include "shell.h"

static int location_valid(const sb_shell_state_t *s) {
    if(!s || s->disk>1u || s->preview>1u || s->path_length<5u || s->path_length>SB_SYS_PATH_MAX ||
       s->path[s->path_length]!=0) return 0;
    const char *root=s->disk?"/disk":"/boot";
    for(unsigned i=0u;i<5u;++i) if(s->path[i]!=root[i]) return 0;
    return s->path_length==5u || s->path[5]=='/';
}
static void root(sb_shell_state_t *s) {
    const char *path=s->disk?"/disk":"/boot";
    s->path_length=5u; s->selected=s->preview=0u;
    for(unsigned i=0u;i<=5u;++i) s->path[i]=path[i];
}
int sb_shell_browser_event(sb_shell_state_t *s,const sb_shell_listing_t *l,const sb_key_event_t *e) {
    if(!location_valid(s) || !l || l->count>SB_SHELL_ROWS) return -1;
    const unsigned old_view=s->view, old_preview=s->preview, old_disk=s->disk;
    const int action=sb_shell_event(s,e); /* Shared ABI/modifier/repeat validation. */
    if(action<0) return -1;
    const int press=e->type==SB_INPUT_EVENT_KEY && (e->flags&SB_KEY_DOWN) && !(e->flags&SB_KEY_REPEAT) &&
        !(e->modifiers&(SB_KEY_MOD_CTRL|SB_KEY_MOD_ALT|SB_KEY_MOD_META));
    if(press && old_view==SB_SHELL_FILES) {
        if(e->keycode==SB_KEY_ESCAPE && old_preview) {
            s->view=SB_SHELL_FILES; s->preview=0u; s->error=0; return SB_SHELL_REDRAW;
        }
        if(e->keycode=='B' || e->keycode=='D') {
            if(old_disk!=s->disk || s->path_length!=5u || old_preview) {
                root(s); s->error=0; return SB_SHELL_RELOAD;
            }
        }
        if(e->keycode==SB_KEY_BACKSPACE) {
            s->error=0;
            if(old_preview) { s->preview=0u; return SB_SHELL_REDRAW; }
            return s->path_length>5u?SB_SHELL_PARENT:SB_SHELL_NONE;
        }
        if(!old_preview && !l->error && l->count) {
            if(e->keycode==SB_KEY_UP && s->selected>0u) { --s->selected; s->error=0; return SB_SHELL_REDRAW; }
            if(e->keycode==SB_KEY_DOWN && s->selected+1u<l->count) { ++s->selected; s->error=0; return SB_SHELL_REDRAW; }
            if(e->keycode==SB_KEY_ENTER) return SB_SHELL_OPEN;
        }
    }
    if(action==SB_SHELL_RELOAD) { s->preview=0u; s->error=0; }
    return action;
}
static int child_path(const sb_shell_state_t *s,const sb_directory_entry_t *entry,char *out,unsigned *length) {
    unsigned n=entry->name_length;
    if(!n || n>SB_SYS_FILE_NAME_MAX || s->path_length+1u+n>SB_SYS_PATH_MAX || entry->reserved ||
       (n==1u && entry->name[0]=='.') || (n==2u && entry->name[0]=='.' && entry->name[1]=='.')) return -1;
    for(unsigned i=0u;i<n;++i) {
        unsigned char c=(unsigned char)entry->name[i];
        if(c<32u || c==127u || c=='/' || c=='\\') return -1;
    }
    for(unsigned i=0u;i<s->path_length;++i) out[i]=s->path[i];
    out[s->path_length]='/';
    for(unsigned i=0u;i<n;++i) out[s->path_length+1u+i]=entry->name[i];
    *length=s->path_length+1u+n; out[*length]=0; return 0;
}
static int preview_file(sb_shell_preview_t *p,const char *path,unsigned length,sb_shell_call_t call,void *ctx) {
    p->length=p->more=0u;
    const int64_t handle=call(ctx,SB_SYS_FILE_OPEN,(uintptr_t)path,length,SB_FILE_ACCESS_READ);
    if(handle<=0) return handle<0?(int)handle:SB_SYS_ERROR_INVALID;
    int error=0;
    while(p->length<SB_SHELL_PREVIEW_BYTES) {
        unsigned n=SB_SHELL_PREVIEW_BYTES-p->length;
        if(n>SB_SYS_FILE_IO_MAX) n=SB_SYS_FILE_IO_MAX;
        const int64_t result=call(ctx,SB_SYS_FILE_READ,(uint64_t)handle,(uintptr_t)(p->bytes+p->length),n);
        if(result<0 || result>(int64_t)n) { error=result<0?(int)result:SB_SYS_ERROR_INVALID; break; }
        if(!result) break;
        p->length+=(unsigned)result;
    }
    if(!error && p->length==SB_SHELL_PREVIEW_BYTES) {
        unsigned char extra;
        const int64_t result=call(ctx,SB_SYS_FILE_READ,(uint64_t)handle,(uintptr_t)&extra,1u);
        if(result<0 || result>1) error=result<0?(int)result:SB_SYS_ERROR_INVALID;
        else p->more=result==1;
    }
    const int64_t close=call(ctx,SB_SYS_HANDLE_CLOSE,(uint64_t)handle,0u,0u);
    if(!error && close!=0) error=(int)close;
    if(error) p->length=p->more=0u;
    return error;
}
int sb_shell_browser_action(sb_shell_state_t *s,sb_shell_listing_t *l,int action,sb_shell_call_t call,void *ctx) {
    if(!location_valid(s) || !l || l->count>SB_SHELL_ROWS || !call) return -1;
    if(action==SB_SHELL_RELOAD) {
        s->preview=0u; s->error=0;
        if(sb_shell_load_path(l,s->path,s->path_length,call,ctx)!=0) return -1;
        if(s->selected>=l->count) s->selected=0u;
        return 0;
    }
    if(action!=SB_SHELL_OPEN && action!=SB_SHELL_PARENT) return action<0?-1:0;
    char path[SB_SYS_PATH_MAX+1u]; unsigned length=0u;
    if(action==SB_SHELL_OPEN) {
        if(l->error || s->selected>=l->count) { s->error=SB_SYS_ERROR_NOT_FOUND; return 0; }
        const sb_directory_entry_t *e=&l->entries[s->selected];
        if(e->type!=SB_DIRECTORY_ENTRY_TYPE_REGULAR && e->type!=SB_DIRECTORY_ENTRY_TYPE_DIRECTORY) {
            s->error=SB_SYS_ERROR_RIGHTS; return 0;
        }
        if(child_path(s,e,path,&length)!=0) { s->error=SB_SYS_ERROR_INVALID; return 0; }
        if(e->type==SB_DIRECTORY_ENTRY_TYPE_REGULAR) {
            s->error=preview_file(&l->content,path,length,call,ctx);
            s->preview=s->error==0; return 0;
        }
    } else {
        if(s->path_length==5u) return 0;
        length=s->path_length;
        while(length>5u && s->path[length-1u]!='/') --length;
        if(length>5u) --length;
        for(unsigned i=0u;i<length;++i) path[i]=s->path[i];
        path[length]=0;
    }
    sb_shell_listing_t candidate;
    if(sb_shell_load_path(&candidate,path,length,call,ctx)!=0) return -1;
    s->error=candidate.error;
    if(s->error) return 0; /* Keep the previous directory/selection on failure. */
    l->count=candidate.count; l->truncated=candidate.truncated; l->error=0;
    for(unsigned i=0u;i<l->count;++i) {
        sb_directory_entry_t *dst=&l->entries[i], *src=&candidate.entries[i];
        dst->type=src->type; dst->name_length=src->name_length; dst->reserved=0; dst->size=src->size;
        for(unsigned j=0u;j<=src->name_length;++j) dst->name[j]=src->name[j];
    }
    for(unsigned i=0u;i<=length;++i) s->path[i]=path[i];
    s->path_length=length; s->selected=s->preview=0u; return 0;
}
unsigned sb_shell_preview_lines(const sb_shell_preview_t *p,char rows[SB_SHELL_ROWS][SB_SHELL_PREVIEW_COLUMNS+1u],unsigned *clipped) {
    if(!p || !rows || !clipped || p->length>SB_SHELL_PREVIEW_BYTES) return 0u;
    for(unsigned r=0u;r<SB_SHELL_ROWS;++r) {
        for(unsigned c=0u;c<SB_SHELL_PREVIEW_COLUMNS;++c) rows[r][c]=' ';
        rows[r][SB_SHELL_PREVIEW_COLUMNS]=0;
    }
    unsigned row=0u,col=0u,index=0u;
    for(;index<p->length;++index) {
        unsigned char c=p->bytes[index];
        if(c=='\r' || c=='\n') {
            if(c=='\r' && index+1u<p->length && p->bytes[index+1u]=='\n') ++index;
            if(col==SB_SHELL_PREVIEW_COLUMNS) col=0u; /* Avoid a second wrap for LF. */
            ++row; col=0u;
            if(row==SB_SHELL_ROWS) { ++index; break; }
            continue;
        }
        if(col==SB_SHELL_PREVIEW_COLUMNS) { col=0u; if(++row==SB_SHELL_ROWS) break; }
        if(c=='\t') { unsigned spaces=4u-col%4u; col+=spaces; }
        else rows[row][col++]=c>=32u && c<=126u?(char)c:'.';
    }
    *clipped=p->more || index<p->length;
    return p->length?(row<SB_SHELL_ROWS?row+1u:SB_SHELL_ROWS):0u;
}
