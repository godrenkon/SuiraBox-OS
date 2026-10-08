#include "shell.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned calls, fail, width, height;
static uint32_t pixels[640u*480u];
static int emit(void *ctx,uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint32_t *rgb) {
    (void)ctx; ++calls;
    assert(w && h && w*h<=256u && x<width && y<height && w<=width-x && h<=height-y);
    if(calls==fail) return -1;
    for(unsigned row=0u;row<h;++row) for(unsigned col=0u;col<w;++col) pixels[(y+row)*width+x+col]=rgb[row*w+col];
    return 0;
}
int main(void) {
    width=640u; height=480u;
    sb_text_renderer_t r={.width=640u,.height=480u,.emit=emit};
    sb_shell_state_t s; sb_shell_init(&s); sb_shell_listing_t l={0};
    assert(sb_shell_render(&r,&s,&l,1u)==0 && calls);
    assert(pixels[0]==0x152536u && pixels[479u*640u+639u]==0x152536u && pixels[60u*640u]==0x0c1018u);
    /* A file name containing newline, NUL and invalid UTF-8 is inert text,
     * and the maximum u64 size fits the supported minimum display. */
    s.view=SB_SHELL_FILES; l.count=1u;
    l.entries[0].type=1u; l.entries[0].name_length=63u; l.entries[0].size=UINT64_MAX;
    memset(l.entries[0].name,'A',63u); l.entries[0].name[0]='\n'; l.entries[0].name[1]=0; l.entries[0].name[2]=(char)0xff;
    assert(sb_shell_render(&r,&s,&l,1u)==0);
    assert(pixels[160u*640u+616u]==0x0c1018u); /* Long row is bounded to x<608. */
    s.offset=UINT32_MAX/12u*12u; assert(sb_shell_render(&r,&s,&l,1u)==0);
    s.offset=1u; unsigned invalid_calls=calls;
    assert(sb_shell_render(&r,&s,&l,1u)==-1 && calls==invalid_calls);
    s.offset=0u;
    l.entries[0].name_length=64u; unsigned before=calls;
    assert(sb_shell_render(&r,&s,&l,1u)==-1 && calls==before);
    l.count=0u; l.error=-6; assert(sb_shell_render(&r,&s,&l,1u)==0);
    s.view=SB_SHELL_SETTINGS; assert(sb_shell_render(&r,&s,&l,0u)==0);
    fail=calls+2u; before=calls; assert(sb_shell_render(&r,&s,&l,1u)==-1 && calls==before+2u);
    fail=0u; width=r.width=11u; height=r.height=9u; assert(sb_shell_render(&r,&s,&l,1u)==0);
    r.width=0u; before=calls; assert(sb_shell_render(&r,&s,&l,1u)==-1 && calls==before);
    puts("desktop shell view host test OK: bounded tiles, sanitized names, minimum display, clipping and emitter failure");
    return 0;
}
