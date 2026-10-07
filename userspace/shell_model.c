#include "shell.h"

void sb_shell_init(sb_shell_state_t *s) {
    if (s) {
        s->view=SB_SHELL_HOME; s->disk=s->overflow=s->selected=s->preview=0u; s->error=0;
        s->path_length=5u;
        static const char root[]="/boot";
        for(unsigned i=0u;i<=5u;++i) s->path[i]=root[i];
    }
}
int sb_shell_event(sb_shell_state_t *s, const sb_key_event_t *e) {
    if (!s || s->view>SB_SHELL_SETTINGS || s->disk>1u || !e || e->size!=SB_KEY_EVENT_SIZE ||
        e->version!=SB_INPUT_ABI_VERSION || e->reserved || (e->modifiers&~31u) ||
        (e->flags&~3u) || ((e->flags&SB_KEY_REPEAT) && !(e->flags&SB_KEY_DOWN))) return -1;
    if (e->type==SB_INPUT_EVENT_OVERFLOW) {
        if (e->keycode || e->flags || !e->dropped) return -1;
        s->overflow=1u; return SB_SHELL_REDRAW;
    }
    if (e->type!=SB_INPUT_EVENT_KEY || !e->keycode || e->keycode>=512u || e->dropped) return -1;
    if (!(e->flags&SB_KEY_DOWN) || (e->flags&SB_KEY_REPEAT) ||
        (e->modifiers&(SB_KEY_MOD_CTRL|SB_KEY_MOD_ALT|SB_KEY_MOD_META))) return SB_SHELL_NONE;
    unsigned next=s->view;
    if (e->keycode==SB_KEY_F1 || e->keycode==SB_KEY_ESCAPE) next=SB_SHELL_HOME;
    else if (e->keycode==(SB_KEY_F1+1u)) next=SB_SHELL_FILES;
    else if (e->keycode==(SB_KEY_F1+2u)) next=SB_SHELL_SETTINGS;
    else if (e->keycode==SB_KEY_TAB)
        next=(s->view+((e->modifiers&SB_KEY_MOD_SHIFT)?2u:1u))%3u;
    else if (s->view==SB_SHELL_FILES) {
        if (e->keycode=='R') return SB_SHELL_RELOAD;
        if (e->keycode=='B' || e->keycode=='D') {
            unsigned disk=e->keycode=='D';
            if (s->disk==disk) return SB_SHELL_NONE;
            s->disk=disk; return SB_SHELL_RELOAD;
        }
    }
    if (next==s->view) return SB_SHELL_NONE;
    s->view=next;
    return next==SB_SHELL_FILES ? SB_SHELL_RELOAD : SB_SHELL_REDRAW;
}
size_t sb_shell_decimal(char *out, uint64_t value) {
    char reverse[20]; size_t n=0u;
    do { reverse[n++]=(char)('0'+value%10u); value/=10u; } while (value);
    for (size_t i=0u;i<n;++i) out[i]=reverse[n-1u-i];
    return n;
}
int sb_shell_load(sb_shell_listing_t *l, unsigned disk, sb_shell_call_t call, void *context) {
    if (!l || !call || disk>1u) return -1;
    static const char boot[]="/boot", volume[]="/disk";
    const char *path=disk?volume:boot;
    return sb_shell_load_path(l,path,5u,call,context);
}
int sb_shell_load_path(sb_shell_listing_t *l,const char *path,size_t length,sb_shell_call_t call,void *context) {
    if (!l || !call || !path || !length || length>SB_SYS_PATH_MAX) return -1;
    l->count=l->truncated=0u; l->error=0;
    const int64_t handle=call(context,SB_SYS_DIRECTORY_OPEN,(uintptr_t)path,length,0u);
    if (handle<=0) { l->error=handle<0?(int)handle:SB_SYS_ERROR_INVALID; return 0; }
    for (unsigned index=0u;index<=SB_SHELL_ROWS;++index) {
        sb_directory_entry_t entry;
        const int64_t status=call(context,SB_SYS_DIRECTORY_READ,(uint64_t)handle,(uintptr_t)&entry,0u);
        if (status==SB_SYS_ERROR_NOT_FOUND) break;
        if (status!=0) { l->error=(int)status; break; }
        if (!entry.name_length || entry.name_length>SB_SYS_FILE_NAME_MAX || entry.reserved ||
            entry.type<SB_DIRECTORY_ENTRY_TYPE_REGULAR || entry.type>SB_DIRECTORY_ENTRY_TYPE_DEVICE) {
            l->error=SB_SYS_ERROR_INVALID; break;
        }
        if (index==SB_SHELL_ROWS) { l->truncated=1u; break; }
        /* Only initialized, length-delimited name bytes cross this boundary. */
        sb_directory_entry_t *dst=&l->entries[l->count++];
        dst->type=entry.type; dst->name_length=entry.name_length; dst->reserved=0u; dst->size=entry.size;
        for (unsigned i=0u;i<entry.name_length;++i) dst->name[i]=entry.name[i];
        dst->name[entry.name_length]=0;
    }
    const int64_t close=call(context,SB_SYS_HANDLE_CLOSE,(uint64_t)handle,0u,0u);
    if (close!=0 && !l->error) l->error=(int)close;
    if (l->error) { l->count=l->truncated=0u; }
    return 0;
}
