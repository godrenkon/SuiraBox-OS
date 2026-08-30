#include "../syscall.h"

#define TERM_MAX_LINE 64u
#define TERM_MAX_OUTPUT 512u

static const uint64_t G_0=0x003C42464A523C00ULL,G_1=0x0010181010107C00ULL,G_2=0x003C42021C20427EULL,G_3=0x007E020C02423C00ULL;
static const uint64_t G_4=0x000C1424447E0400ULL,G_5=0x007E407C02423C00ULL,G_6=0x001C20407C42423CULL,G_7=0x007E020408101000ULL;
static const uint64_t G_8=0x003C42423C42423CULL,G_9=0x003C42427E020438ULL;
static const uint64_t GA=0x003C42427E424242ULL,GB=0x007C42427C42427CULL,GC=0x003C42404040423CULL,GD=0x0078424242424278ULL;
static const uint64_t GE=0x007E40407C40407EULL,GF=0x007E40407C404040ULL;
static const uint64_t GH=0x0042427E42424242ULL,GI=0x007E18181818187EULL,GJ=0x001E080808484830ULL;
static const uint64_t GK=0x0042444870484400ULL,GL=0x004040404040407EULL,GM=0x0042665A42424242ULL,GN=0x004266525A424242ULL;
static const uint64_t GO=0x003C42424242423CULL,GP=0x007C42427C404040ULL,GQ=0x003C42424A4C423CULL,GR=0x007C42427C484442ULL;
static const uint64_t GS=0x003C40403C02023CULL,GT=0x007E181818181818ULL,GU=0x004242424242423CULL,GV=0x0042242424181800ULL;
static const uint64_t GW=0x004242425A5A6666ULL,GX=0x0042241818244242ULL,GY=0x0042241818181800ULL,GZ=0x007E02040810207EULL;
static const uint64_t GSPACE=0;

static uint64_t glyph_for(char c) {
    switch(c) {
        case '0':return G_0;case '1':return G_1;case '2':return G_2;case '3':return G_3;case '4':return G_4;case '5':return G_5;
        case '6':return G_6;case '7':return G_7;case '8':return G_8;case '9':return G_9;
        case 'A':return GA;case 'B':return GB;case 'C':return GC;case 'D':return GD;case 'E':return GE;case 'F':return GF;
        case 'H':return GH;case 'I':return GI;case 'J':return GJ;case 'K':return GK;case 'L':return GL;case 'M':return GM;case 'N':return GN;
        case 'O':return GO;case 'P':return GP;case 'Q':return GQ;case 'R':return GR;case 'S':return GS;case 'T':return GT;case 'U':return GU;
        case 'V':return GV;case 'W':return GW;case 'X':return GX;case 'Y':return GY;case 'Z':return GZ;case ' ':return GSPACE;default:return GC;
    }
}

static char scancode_char(uint8_t scancode) {
    static const char alpha[26]={'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};
    static const uint8_t codes[26]={0x1Eu,0x30u,0x2Eu,0x20u,0x12u,0x21u,0x22u,0x23u,0x17u,0x24u,0x25u,0x26u,0x32u,0x31u,0x18u,0x19u,0x10u,0x13u,0x1Fu,0x14u,0x16u,0x2Fu,0x11u,0x2Du,0x15u,0x2Cu};
    static const char digits[10]={'0','1','2','3','4','5','6','7','8','9'};
    for (uint32_t i=0;i<26u;++i) if (scancode==codes[i]) return alpha[i];
    if (scancode>=0x02u && scancode<=0x0Bu) return digits[scancode-0x02u+1u==10u?0u:scancode-0x02u];
    if (scancode==0x39u) return ' ';
    return 0;
}

static void draw_char(uint32_t x,uint32_t y,char c,uint32_t rgb) {
    if (c!=' ') (void)sb_display_glyph(x,y,glyph_for(c),rgb);
}
static void draw_text(uint32_t x,uint32_t y,const char *text,uint32_t rgb) {
    if (text==0) return;
    while (*text!='\0') { draw_char(x,y,*text++,rgb); x+=10u; }
}
static uint64_t hex_glyph(uint8_t v) { return glyph_for(v<10u?(char)('0'+v):(char)('A'+v-10u)); }
static void draw_hex(uint32_t x,uint32_t y,uint64_t value) {
    for (int32_t i=15;i>=0;--i) { (void)sb_display_glyph(x,y,hex_glyph((uint8_t)(value>>((uint32_t)i*4u))),0xBFD8FFu); x+=10u; }
}
static void clear_terminal(void) {
    (void)sb_display_clear(0x10151Bu);
    draw_text(24u,24u,"SB TERMINAL",0xE9F2FFu);
    draw_text(24u,56u,"READY",0x7FA8D8u);
}

static void run_command(const char *line) {
    if (line==0) return;
    (void)sb_display_rect(24u,88u,720u,380u,0x10151Bu);
    if (line[0]=='H'&&line[1]=='E'&&line[2]=='L'&&line[3]=='P'&&line[4]=='\0') {
        draw_text(24u,100u,"HELP LS TICKS PID CLEAR CREATE WRITE CAT EXIT",0xBFD8FFu);
    } else if (line[0]=='L'&&line[1]=='S'&&line[2]=='\0') {
        char names[TERM_MAX_OUTPUT]; uint64_t n=sb_fs_list_root(names,sizeof(names));
        if(n==UINT64_MAX) draw_text(24u,100u,"LS ERROR",0xFF8080u);
        else { uint32_t off=0u,row=0u; while(off<(uint32_t)n && row<12u){ uint32_t len=0u; while(off+len<(uint32_t)n&&names[off+len]!='\0')++len; if(len){ for(uint32_t i=0u;i<len&&i<60u;++i) draw_char(24u+i*10u,100u+row*28u,names[off+i],0xBFD8FFu); ++row;} off+=len+1u; } }
    } else if(line[0]=='T'&&line[1]=='I'&&line[2]=='C'&&line[3]=='K'&&line[4]=='S'&&line[5]=='\0') {
        draw_text(24u,100u,"TICKS",0xBFD8FFu); draw_hex(96u,100u,sb_get_ticks());
    } else if(line[0]=='P'&&line[1]=='I'&&line[2]=='D'&&line[3]=='\0') {
        draw_text(24u,100u,"PID",0xBFD8FFu); draw_hex(96u,100u,sb_process_id());
    } else if(line[0]=='C'&&line[1]=='L'&&line[2]=='E'&&line[3]=='A'&&line[4]=='R'&&line[5]=='\0') {
        clear_terminal();
    } else if(line[0]=='C'&&line[1]=='R'&&line[2]=='E'&&line[3]=='A'&&line[4]=='T'&&line[5]=='E'&&line[6]=='\0') {
        const char name[]="SBCMD.TXT"; const uint64_t r=sb_fs_create_root(name,9u,128u);
        draw_text(24u,100u,r==0u?"CREATED":"CREATE ERROR",r==0u?0x80D8A0u:0xFF8080u);
    } else if(line[0]=='W'&&line[1]=='R'&&line[2]=='I'&&line[3]=='T'&&line[4]=='E'&&line[5]=='\0') {
        const char name[]="SBCMD.TXT"; const char data[]="SB OK"; const uint64_t r=sb_fs_write_root(name,9u,data,5u,0u);
        draw_text(24u,100u,r==5u?"WRITTEN":"WRITE ERROR",r==5u?0x80D8A0u:0xFF8080u);
    } else if(line[0]=='C'&&line[1]=='A'&&line[2]=='T'&&line[3]=='\0') {
        const char name[]="SBCMD.TXT"; char data[32]={0}; const uint64_t r=sb_fs_read_root(name,9u,data,sizeof(data),0u);
        if(r==UINT64_MAX) draw_text(24u,100u,"CAT ERROR",0xFF8080u); else { draw_text(24u,100u,"DATA",0xBFD8FFu); for(uint32_t i=0u;i<(uint32_t)r&&i<16u;++i){ (void)sb_display_glyph(74u+i*20u,100u,hex_glyph(((uint8_t)data[i])>>4),0xBFD8FFu); (void)sb_display_glyph(84u+i*20u,100u,hex_glyph(((uint8_t)data[i])&0x0Fu),0xBFD8FFu); } }
    } else {
        draw_text(24u,100u,"UNKNOWN COMMAND",0xFFB070u);
    }
}

uint64_t sb_app_main(void) {
    char line[TERM_MAX_LINE+1u]={0}; uint32_t length=0u;
    clear_terminal(); draw_text(24u,84u,"HELP",0x7FA8D8u); draw_text(24u,440u,"PROMPT",0x7FA8D8u);
    for (;;) {
        const uint64_t key=sb_input_key();
        if(key==0u){(void)sb_yield();continue;}
        const uint8_t scancode=(uint8_t)key;
        if((scancode&0x80u)!=0u) continue;
        if(scancode==0x01u) return 0u;
        if(scancode==0x0Eu){ if(length>0u){--length;line[length]='\0';} continue; }
        if(scancode==0x1Cu){ line[length]='\0'; run_command(line); length=0u; line[0]='\0'; continue; }
        const char c=scancode_char(scancode);
        if(c!=0 && length<TERM_MAX_LINE){ line[length++]=c; line[length]='\0'; draw_char(24u+(length-1u)*10u,440u,c,0xE9F2FFu); }
    }
}
