#include "line_edit.h"
void sb_line_edit_init(sb_line_edit_t *e) {
    if(e) { e->length=e->submitted_length=0u; e->text[0]=e->submitted[0]=0; }
}
static char character(uint16_t key, uint32_t mods) {
    if (mods&(SB_KEY_MOD_CTRL|SB_KEY_MOD_ALT|SB_KEY_MOD_META)) return 0;
    const int shift=(mods&SB_KEY_MOD_SHIFT)!=0;
    if (key>='A' && key<='Z') return (char)(key+((shift!=((mods&SB_KEY_MOD_CAPS)!=0))?0:'a'-'A'));
    if (!(key>=32u && key<=126u)) return 0;
    if (!shift) return (char)key;
    static const char base[]="1234567890-=[];\'`,./\\";
    static const char upper[]="!@#$%^&*()_+{}:\"~<>?|";
    for(unsigned i=0u;i<sizeof(base)-1u;++i) if (key==(uint16_t)(unsigned char)base[i]) return upper[i];
    return (char)key;
}
int sb_line_edit_event(sb_line_edit_t *e, const sb_key_event_t *event) {
    if (!e || !event || event->size!=SB_KEY_EVENT_SIZE || event->version!=SB_INPUT_ABI_VERSION ||
        event->reserved || (event->modifiers&~31u) || (event->flags&~3u) ||
        ((event->flags&SB_KEY_REPEAT) && !(event->flags&SB_KEY_DOWN))) return -1;
    if (event->type==SB_INPUT_EVENT_OVERFLOW)
        return !event->keycode && !event->flags && event->dropped ? SB_LINE_OVERFLOW : -1;
    if (event->type!=SB_INPUT_EVENT_KEY || !event->keycode || event->keycode>=512u) return -1;
    if (!(event->flags&SB_KEY_DOWN)) return 0;
    if (event->keycode==SB_KEY_BACKSPACE) {
        if (!e->length) return 0;
        e->text[--e->length]=0; return SB_LINE_CHANGED;
    }
    if (event->keycode==SB_KEY_ENTER) {
        if (event->flags&SB_KEY_REPEAT) return 0;
        e->submitted_length=e->length;
        for(unsigned i=0u;i<=e->length;++i) e->submitted[i]=e->text[i];
        e->length=0u; e->text[0]=0; return SB_LINE_SUBMITTED;
    }
    const char ascii=character(event->keycode,event->modifiers);
    if (!ascii || e->length==SB_LINE_EDIT_CAPACITY) return 0;
    e->text[e->length++]=ascii; e->text[e->length]=0; return SB_LINE_CHANGED;
}
