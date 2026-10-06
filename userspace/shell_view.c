#include "shell.h"

#define BG 0x0c1018u
#define PANEL 0x152536u
#define FG 0xe6eef2u
#define ACCENT 0x259b72u
static size_t length(const char *s) { size_t n=0u; while(s[n]) ++n; return n; }
static int text(const sb_text_renderer_t *r, int32_t x, int32_t y, const char *s,
                uint32_t fg, uint32_t bg, unsigned scale) {
    sb_text_renderer_t copy=*r; copy.foreground=fg; copy.background=bg; copy.scale=scale;
    return sb_text_draw(&copy,x,y,s,length(s));
}
/* All fills are clipped; every transport rectangle contains <=256 pixels.
 * The emitter must consume the temporary solid tile synchronously. */
static int fill(const sb_text_renderer_t *r,uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t color) {
    if (x>=r->width || y>=r->height) return 0;
    if (w>r->width-x) w=r->width-x;
    if (h>r->height-y) h=r->height-y;
    uint32_t tile[256]; for(unsigned i=0u;i<256u;++i) tile[i]=color;
    for(uint32_t row=0u;row<h;row+=16u) for(uint32_t col=0u;col<w;col+=16u) {
        uint32_t tw=w-col<16u?w-col:16u, th=h-row<16u?h-row:16u;
        if(r->emit(r->context,x+col,y+row,tw,th,tile)!=0) return -1;
    }
    return 0;
}
int sb_shell_render(const sb_text_renderer_t *r,const sb_shell_state_t *s,
                    const sb_shell_listing_t *l,unsigned keyboard) {
    if(!r || !r->emit || !r->width || !r->height || r->width>4096u || r->height>4096u ||
       !s || s->view>SB_SHELL_SETTINGS || s->disk>1u || !l || l->count>SB_SHELL_ROWS) return -1;
    /* Validate snapshot metadata before any draw; names are sanitized below. */
    for(unsigned i=0u;i<l->count;++i) if (!l->entries[i].name_length ||
        l->entries[i].name_length>SB_SYS_FILE_NAME_MAX || l->entries[i].reserved ||
        l->entries[i].type<SB_DIRECTORY_ENTRY_TYPE_REGULAR || l->entries[i].type>SB_DIRECTORY_ENTRY_TYPE_DEVICE) return -1;
    if(fill(r,0u,0u,r->width,r->height,BG)!=0) return -1;
    if(r->width<640u || r->height<480u)
        return text(r,8,8,"Shell requires 640x480",FG,BG,1u);
#define TEXT(X,Y,S,F,B,Z) do { if(text(r,(X),(Y),(S),(F),(B),(Z))!=0) return -1; } while(0)
    if(fill(r,0,0,r->width,48u,PANEL)!=0 || fill(r,16,72,160,r->height-112u,PANEL)!=0 ||
       fill(r,0,r->height-28u,r->width,28u,PANEL)!=0) return -1;
    TEXT(24,16,"SuiraBox OS",FG,PANEL,2u);
    static const char *const names[]={"Home","Files","Settings"};
    for(unsigned i=0u;i<3u;++i) {
        uint32_t color=s->view==i?ACCENT:PANEL;
        if(fill(r,24,88u+i*40u,144,32,color)!=0) return -1;
        TEXT(32,(int32_t)(96u+i*40u),names[i],FG,color,2u);
    }
    TEXT(208,80,names[s->view],FG,BG,2u);
    if(s->view==SB_SHELL_HOME) {
        TEXT(208,128,"Welcome to SuiraBox",FG,BG,2u);
        TEXT(208,176,keyboard?"Kernel, storage and keyboard are ready.":"Kernel and storage are ready.",FG,BG,1u);
        TEXT(208,200,"Use F2 to browse boot files or the disk.",FG,BG,1u);
        TEXT(208,248,"Minecraft runtime: not installed",0xf2ae43u,BG,1u);
        TEXT(208,272,"Network, JVM and game launcher are pending.",FG,BG,1u);
    } else if(s->view==SB_SHELL_SETTINGS) {
        char resolution[48]="Display: "; size_t n=9u;
        n+=sb_shell_decimal(resolution+n,r->width); resolution[n++]='x';
        n+=sb_shell_decimal(resolution+n,r->height); resolution[n]=0;
        TEXT(208,128,resolution,FG,BG,1u);
        TEXT(208,160,keyboard?"Keyboard: PS/2, US layout":"Keyboard: unavailable",FG,BG,1u);
        TEXT(208,192,"Text: ASCII bitmap, UTF-8 fallback",FG,BG,1u);
        TEXT(208,224,"CJK fonts and IME are pending.",0xf2ae43u,BG,1u);
        TEXT(208,256,"Persistent settings are pending.",FG,BG,1u);
    } else {
        TEXT(208,112,s->disk?"/disk":"/boot",ACCENT,BG,2u);
        TEXT(208,140,"Type / name                  Size (bytes)",FG,BG,1u);
        if(l->error) TEXT(208,176,"Directory unavailable; B / D / R to retry",0xf2ae43u,BG,1u);
        else if(!l->count) TEXT(208,176,"Directory is empty",FG,BG,1u);
        for(unsigned i=0u;i<l->count;++i) {
            const sb_directory_entry_t *e=&l->entries[i]; char row[64];
            for(unsigned j=0u;j<29u;++j) row[j]=' ';
            row[0]='['; row[1]=e->type==SB_DIRECTORY_ENTRY_TYPE_REGULAR?'F':e->type==SB_DIRECTORY_ENTRY_TYPE_DIRECTORY?'D':'V'; row[2]=']';
            unsigned visible=e->name_length<24u?e->name_length:24u;
            for(unsigned j=0u;j<visible;++j) { unsigned char c=(unsigned char)e->name[j]; row[4u+j]=c>=32u && c<=126u?(char)c:'?'; }
            if(e->name_length>24u) row[27]='~';
            size_t n=29u+sb_shell_decimal(row+29u,e->size); row[n]=0;
            TEXT(208,(int32_t)(160u+i*24u),row,FG,BG,1u);
        }
    }
    const char *footer="F1 Home  F2 Files  F3 Settings  Tab cycle  Esc Home";
    if(s->view==SB_SHELL_FILES) footer=l->truncated?"First 12 entries only  B boot  D disk  R refresh":"B boot  D disk  R refresh  F1 Home  F3 Settings";
    if(s->overflow) footer="Input overflow: keys resynchronized; release and retry";
    if(!keyboard) footer="Keyboard unavailable";
    TEXT(24,(int32_t)r->height-20,footer,FG,PANEL,1u);
#undef TEXT
    return 0;
}
