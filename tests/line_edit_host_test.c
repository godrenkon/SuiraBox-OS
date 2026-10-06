#include "line_edit.h"
#include <stdio.h>
#include <string.h>
static int check(int ok,const char*n){if(!ok){fprintf(stderr,"line edit FAILED: %s\n",n);return 1;}return 0;}
static int feed(sb_line_edit_t*e,uint16_t code,uint16_t flags,uint32_t mods){
    const sb_key_event_t event={.size=32u,.version=1u,.type=1u,.keycode=code,.flags=flags,.modifiers=mods};
    return sb_line_edit_event(e,&event);
}
int main(void){sb_line_edit_t e;sb_line_edit_init(&e);
    if(check(feed(&e,'A',1,1)==1&&feed(&e,'B',1,0)==1&&feed(&e,SB_KEY_BACKSPACE,1,0)==1&&
        feed(&e,'C',1,8)==1&&!strcmp(e.text,"AC"),"Shift, Caps and Backspace"))return 1;
    if(check(feed(&e,SB_KEY_RIGHT,1,0)==0&&feed(&e,'D',0,0)==0&&feed(&e,'E',1,2)==0&&
        feed(&e,'F',1,4)==0&&feed(&e,'G',1,16)==0,"release/arrows/shortcut do not insert"))return 1;
    if(check(feed(&e,SB_KEY_ENTER,1,0)==2&&!e.length&&!strcmp(e.submitted,"AC")&&
        feed(&e,SB_KEY_ENTER,3,0)==0&&!strcmp(e.submitted,"AC"),"Enter commits once"))return 1;
    if(check(feed(&e,'A',1,9)==1&&feed(&e,'1',1,1)==1&&feed(&e,'/',1,1)==1&&
        feed(&e,'\'',1,1)==1&&feed(&e,'\\',1,1)==1&&!strcmp(e.text,"a!?\"|"),"US shift punctuation and Caps XOR"))return 1;
    sb_line_edit_init(&e);
    for(unsigned i=0;i<SB_LINE_EDIT_CAPACITY;++i)if(check(feed(&e,'Z',3,0)==1,"bounded repeat insertion"))return 1;
    if(check(feed(&e,'Z',1,0)==0&&e.length==32u&&!e.text[32],"full line"))return 1;
    for(unsigned i=0;i<SB_LINE_EDIT_CAPACITY;++i)if(check(feed(&e,SB_KEY_BACKSPACE,3,0)==1,"repeat backspace"))return 1;
    if(check(feed(&e,SB_KEY_BACKSPACE,1,0)==0&&!e.length,"empty backspace"))return 1;
    sb_key_event_t event={.size=32,.version=1,.type=2,.dropped=65,.sequence=65};
    if(check(sb_line_edit_event(&e,&event)==3,"overflow notification"))return 1;
    event.reserved=1;if(check(sb_line_edit_event(&e,&event)==-1,"bad metadata"))return 1;
    event.reserved=0;event.flags=2;if(check(sb_line_edit_event(&e,&event)==-1,"repeat without make"))return 1;
    puts("Keyboard line editor host test OK");return 0;}
