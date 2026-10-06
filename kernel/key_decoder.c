#include "key_decoder.h"
static uint16_t position(uint8_t scan, int extended) {
    if (extended) {
        switch (scan) {
            case 0x1c: return SB_KEY_ENTER; case 0x1d: return SB_KEY_RIGHT_CTRL;
            case 0x38: return SB_KEY_RIGHT_ALT; case 0x35: return '/';
            case 0x37: return SB_KEY_PRINT_SCREEN;
            case 0x47: return SB_KEY_HOME; case 0x48: return SB_KEY_UP;
            case 0x49: return SB_KEY_PAGE_UP; case 0x4b: return SB_KEY_LEFT;
            case 0x4d: return SB_KEY_RIGHT; case 0x4f: return SB_KEY_END;
            case 0x50: return SB_KEY_DOWN_ARROW; case 0x51: return SB_KEY_PAGE_DOWN;
            case 0x52: return SB_KEY_INSERT; case 0x53: return SB_KEY_DELETE;
            case 0x5b: return SB_KEY_LEFT_META; case 0x5c: return SB_KEY_RIGHT_META;
            default: return 0; /* Includes PrintScreen's fake shift prefix. */
        }
    }
    static const uint16_t keys[128] = {
        [1]=SB_KEY_ESCAPE,[2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',
        [12]='-',[13]='=',[14]=SB_KEY_BACKSPACE,[15]=SB_KEY_TAB,
        [16]='Q',[17]='W',[18]='E',[19]='R',[20]='T',[21]='Y',[22]='U',[23]='I',[24]='O',[25]='P',[26]='[',[27]=']',
        [28]=SB_KEY_ENTER,[29]=SB_KEY_LEFT_CTRL,[30]='A',[31]='S',[32]='D',[33]='F',[34]='G',[35]='H',
        [36]='J',[37]='K',[38]='L',[39]=';',[40]='\'',[41]='`',[42]=SB_KEY_LEFT_SHIFT,[43]='\\',
        [44]='Z',[45]='X',[46]='C',[47]='V',[48]='B',[49]='N',[50]='M',[51]=',',[52]='.',[53]='/',
        [54]=SB_KEY_RIGHT_SHIFT,[55]='*',[56]=SB_KEY_LEFT_ALT,[57]=' ',[58]=SB_KEY_CAPS_LOCK,
        [59]=SB_KEY_F1,[60]=SB_KEY_F1+1,[61]=SB_KEY_F1+2,[62]=SB_KEY_F1+3,[63]=SB_KEY_F1+4,
        [64]=SB_KEY_F1+5,[65]=SB_KEY_F1+6,[66]=SB_KEY_F1+7,[67]=SB_KEY_F1+8,[68]=SB_KEY_F1+9,
        [87]=SB_KEY_F1+10,[88]=SB_KEY_F1+11,
    };
    return keys[scan];
}
static uint32_t modifiers(const sb_key_decoder_t *d) {
    return ((d->pressed[SB_KEY_LEFT_SHIFT] || d->pressed[SB_KEY_RIGHT_SHIFT]) ? SB_KEY_MOD_SHIFT : 0u) |
        ((d->pressed[SB_KEY_LEFT_CTRL] || d->pressed[SB_KEY_RIGHT_CTRL]) ? SB_KEY_MOD_CTRL : 0u) |
        ((d->pressed[SB_KEY_LEFT_ALT] || d->pressed[SB_KEY_RIGHT_ALT]) ? SB_KEY_MOD_ALT : 0u) |
        ((d->pressed[SB_KEY_LEFT_META] || d->pressed[SB_KEY_RIGHT_META]) ? SB_KEY_MOD_META : 0u) |
        (d->caps ? SB_KEY_MOD_CAPS : 0u);
}
static void overflow(sb_key_decoder_t *d, uint64_t sequence) {
    const uint32_t lost = d->count + 1u;
    d->dropped = lost > UINT32_MAX - d->dropped ? UINT32_MAX : d->dropped + lost;
    d->head = 0u; d->count = 1u;
    d->events[0] = (sb_key_event_t){.size=SB_KEY_EVENT_SIZE,.version=SB_INPUT_ABI_VERSION,
        .type=SB_INPUT_EVENT_OVERFLOW,.modifiers=modifiers(d),.sequence=sequence,.dropped=d->dropped};
}
static void key(sb_key_decoder_t *d, uint16_t code, int down) {
    if (code == 0u || (!down && !d->pressed[code])) return;
    const int repeat = down && d->pressed[code];
    d->pressed[code] = down != 0;
    if (code == SB_KEY_CAPS_LOCK && down && !repeat) d->caps ^= 1u;
    const uint64_t seq = ++d->sequence;
    if (d->count == SB_KEY_QUEUE_CAPACITY) { overflow(d, seq); return; }
    const uint32_t slot = (d->head + d->count++) % SB_KEY_QUEUE_CAPACITY;
    d->events[slot] = (sb_key_event_t){.size=SB_KEY_EVENT_SIZE,.version=SB_INPUT_ABI_VERSION,
        .type=SB_INPUT_EVENT_KEY,.keycode=code,.flags=(down ? SB_KEY_DOWN : 0u)|(repeat ? SB_KEY_REPEAT : 0u),
        .modifiers=modifiers(d),.sequence=seq,.dropped=d->dropped};
}
void sb_key_decoder_init(sb_key_decoder_t *d) {
    if (!d) return;
    d->head=d->count=d->dropped=0u; d->sequence=0u;
    d->caps=d->extended=d->pause_index=0u;
    /* Queued storage is unreachable while count is zero. Volatile stores keep
     * a freestanding compiler from introducing a runtime memset dependency. */
    for (unsigned i=0u; i<512u; ++i) ((volatile uint8_t *)d->pressed)[i]=0u;
}
void sb_key_decoder_feed(sb_key_decoder_t *d, uint8_t byte) {
    if (!d) return;
    static const uint8_t pause_tail[5] = {0x1d,0x45,0xe1,0x9d,0xc5};
    if (d->pause_index) {
        if (byte != pause_tail[d->pause_index-1u]) { d->pause_index=0u; d->extended=0u; return; }
        if (++d->pause_index == 6u) { d->pause_index=0u; key(d, SB_KEY_PAUSE, 1); key(d, SB_KEY_PAUSE, 0); }
        return;
    }
    if (byte == 0xe1u) { d->pause_index=1u; d->extended=0u; return; }
    if (byte == 0xe0u) { d->extended=1u; return; }
    if (byte == 0xfau || byte == 0xfeu || byte == 0xffu || byte == 0u) { d->extended=0u; return; }
    const int extended = d->extended;
    d->extended=0u;
    key(d, position(byte & 0x7fu, extended), (byte & 0x80u) == 0u);
}
void sb_key_decoder_error(sb_key_decoder_t *d) {
    if (!d) return;
    for (unsigned i=0u; i<512u; ++i) ((volatile uint8_t *)d->pressed)[i]=0u;
    d->caps=d->extended=d->pause_index=0u;
    overflow(d, ++d->sequence);
}
int sb_key_decoder_peek(const sb_key_decoder_t *d, sb_key_event_t *event) {
    if (!d || !event || !d->count) return 0;
    *event=d->events[d->head]; return 1;
}
void sb_key_decoder_pop(sb_key_decoder_t *d) {
    if (d && d->count) { d->head=(d->head+1u)%SB_KEY_QUEUE_CAPACITY; --d->count; }
}
