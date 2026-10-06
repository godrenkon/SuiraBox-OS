#include "key_decoder.h"
#include <stdio.h>
static int check(int ok, const char *name) { if(!ok){fprintf(stderr,"key decoder FAILED: %s\n",name);return 1;}return 0; }
static int next(sb_key_decoder_t *d, uint16_t code, uint16_t flags, uint32_t mods) {
    sb_key_event_t e;
    if(check(sb_key_decoder_peek(d,&e) && e.size==32u && e.version==1u && e.type==SB_INPUT_EVENT_KEY &&
        e.keycode==code && e.flags==flags && e.modifiers==mods && !e.reserved,"ordered key metadata"))return 1;
    sb_key_decoder_pop(d);return 0;
}
int main(void) {
    sb_key_decoder_t d; sb_key_decoder_init(&d); sb_key_event_t e;
    if(check(!sb_key_decoder_peek(&d,&e),"empty"))return 1;
    sb_key_decoder_feed(&d,0x2a);sb_key_decoder_feed(&d,0x1e);sb_key_decoder_feed(&d,0x1e);
    sb_key_decoder_feed(&d,0x9e);sb_key_decoder_feed(&d,0xaa);
    if(next(&d,SB_KEY_LEFT_SHIFT,1,1)||next(&d,'A',1,1)||next(&d,'A',3,1)||next(&d,'A',0,1)||
        next(&d,SB_KEY_LEFT_SHIFT,0,0))return 1;
    sb_key_decoder_feed(&d,0x3a);sb_key_decoder_feed(&d,0x3a);sb_key_decoder_feed(&d,0xba);
    sb_key_decoder_feed(&d,0x3a);sb_key_decoder_feed(&d,0xba);
    if(next(&d,SB_KEY_CAPS_LOCK,1,8)||next(&d,SB_KEY_CAPS_LOCK,3,8)||next(&d,SB_KEY_CAPS_LOCK,0,8)||
        next(&d,SB_KEY_CAPS_LOCK,1,0)||next(&d,SB_KEY_CAPS_LOCK,0,0))return 1;
    const uint8_t bytes[]={0xe0,0x1d,0xe0,0x38,0xe0,0x4d,0xe0,0xcd,0xe0,0xb8,0xe0,0x9d};
    for(unsigned i=0;i<sizeof(bytes);++i)sb_key_decoder_feed(&d,bytes[i]);
    if(next(&d,SB_KEY_RIGHT_CTRL,1,2)||next(&d,SB_KEY_RIGHT_ALT,1,6)||next(&d,SB_KEY_RIGHT,1,6)||
        next(&d,SB_KEY_RIGHT,0,6)||next(&d,SB_KEY_RIGHT_ALT,0,2)||next(&d,SB_KEY_RIGHT_CTRL,0,0))return 1;
    const uint8_t special[]={0xe0,0x2a,0xe0,0x37,0xe0,0xb7,0xe0,0xaa,0xe1,0x1d,0x45,0xe1,0x9d,0xc5};
    for(unsigned i=0;i<sizeof(special);++i)sb_key_decoder_feed(&d,special[i]);
    if(next(&d,SB_KEY_PRINT_SCREEN,1,0)||next(&d,SB_KEY_PRINT_SCREEN,0,0)||
        next(&d,SB_KEY_PAUSE,1,0)||next(&d,SB_KEY_PAUSE,0,0))return 1;
    const uint8_t ignored[]={0xfa,0xfe,0xff,0,0x7f,0x9e,0xe1,0x12,0xe0,0xfa};
    for(unsigned i=0;i<sizeof(ignored);++i)sb_key_decoder_feed(&d,ignored[i]);
    if(check(!d.count && !d.extended && !d.pause_index,"unknown/reply/stray break/malformed pause"))return 1;
    sb_key_decoder_feed(&d,0x2a);sb_key_decoder_feed(&d,0x36);sb_key_decoder_feed(&d,0xaa);sb_key_decoder_feed(&d,0xb6);
    if(next(&d,SB_KEY_LEFT_SHIFT,1,1)||next(&d,SB_KEY_RIGHT_SHIFT,1,1)||next(&d,SB_KEY_LEFT_SHIFT,0,1)||
        next(&d,SB_KEY_RIGHT_SHIFT,0,0))return 1;
    sb_key_decoder_init(&d);
    for(unsigned i=0;i<64u;++i)sb_key_decoder_feed(&d,0x1e);
    if(check(d.count==64u && d.sequence==64u,"full bounded queue"))return 1;
    sb_key_decoder_feed(&d,0x9e);
    if(check(d.count==1u && sb_key_decoder_peek(&d,&e) && e.type==SB_INPUT_EVENT_OVERFLOW &&
        e.sequence==65u && e.dropped==65u && !e.keycode && !e.flags && !e.modifiers,"overflow resynchronization"))return 1;
    sb_key_decoder_pop(&d);sb_key_decoder_feed(&d,0x1e);
    if(check(sb_key_decoder_peek(&d,&e)&&e.sequence==66u&&e.dropped==65u&&e.flags==1u,"state updates despite overflow"))return 1;
    sb_key_decoder_error(&d);
    if(check(sb_key_decoder_peek(&d,&e)&&e.type==SB_INPUT_EVENT_OVERFLOW&&e.dropped==67u&&
        !e.modifiers&&!d.pressed['A'],"transport error clears held state"))return 1;
    sb_key_decoder_pop(&d); d.dropped=UINT32_MAX-1u; sb_key_decoder_error(&d);sb_key_decoder_error(&d);
    if(check(d.dropped==UINT32_MAX,"drop counter saturates"))return 1;
    /* Exercise head wrap and release filtering without reallocating. */
    sb_key_decoder_init(&d);
    for(unsigned i=0;i<300u;++i){sb_key_decoder_feed(&d,0x30);if(next(&d,'B',1,0))return 1;
        sb_key_decoder_feed(&d,0xb0);if(next(&d,'B',0,0))return 1;}
    if(check(d.sequence==600u&&!d.count,"ring wrap"))return 1;
    puts("PS/2 key decoder host test OK");return 0;
}
