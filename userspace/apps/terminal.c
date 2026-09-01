#include "../syscall.h"

#define TERM_MAX_LINE 64u
#define TERM_MAX_OUTPUT 512u
#define TERM_MAX_PATH 64u

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

static char scancode_char(uint8_t scancode) {
    static const char alpha[26]={'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};
    static const uint8_t codes[26]={0x1Eu,0x30u,0x2Eu,0x20u,0x12u,0x21u,0x22u,0x23u,0x17u,0x24u,0x25u,0x26u,0x32u,0x31u,0x18u,0x19u,0x10u,0x13u,0x1Fu,0x14u,0x16u,0x2Fu,0x11u,0x2Du,0x15u,0x2Cu};
    static const char digits[10]={'0','1','2','3','4','5','6','7','8','9'};
    for (uint32_t i=0u;i<26u;++i) if (scancode==codes[i]) return alpha[i];
    if (scancode>=0x02u && scancode<=0x0Au) return digits[scancode-0x02u+1u];
    if (scancode==0x0Bu) return '0';
    if (scancode==0x39u) return ' ';
    if (scancode==0x35u) return '/';
    if (scancode==0x0Cu) return '-';
    return 0;
}

static uint64_t glyph_for(char c) {
    switch(c) {
        case '0':return G_0;case '1':return G_1;case '2':return G_2;case '3':return G_3;case '4':return G_4;case '5':return G_5;
        case '6':return G_6;case '7':return G_7;case '8':return G_8;case '9':return G_9;
        case 'A':return GA;case 'B':return GB;case 'C':return GC;case 'D':return GD;case 'E':return GE;case 'F':return GF;
        case 'H':return GH;case 'I':return GI;case 'J':return GJ;case 'K':return GK;case 'L':return GL;case 'M':return GM;case 'N':return GN;
        case 'O':return GO;case 'P':return GP;case 'Q':return GQ;case 'R':return GR;case 'S':return GS;case 'T':return GT;case 'U':return GU;
        case 'V':return GV;case 'W':return GW;case 'X':return GX;case 'Y':return GY;case 'Z':return GZ;default:return GC;
    }
}

static void draw_char(uint32_t x,uint32_t y,char c,uint32_t rgb) { if(c!=' ') (void)sb_display_glyph(x,y,glyph_for(c),rgb); }
static void draw_text(uint32_t x,uint32_t y,const char *text,uint32_t rgb) { if(text==0)return; while(*text!='\0'){draw_char(x,y,*text++,rgb);x+=10u;} }
static uint64_t hex_glyph(uint8_t v) { return glyph_for(v<10u?(char)('0'+v):(char)('A'+v-10u)); }
static void draw_hex(uint32_t x,uint32_t y,uint64_t value) { for(int32_t i=15;i>=0;--i){(void)sb_display_glyph(x,y,hex_glyph((uint8_t)(value>>((uint32_t)i*4u))),0xBFD8FFu);x+=10u;} }
static void clear_terminal(void) { (void)sb_display_clear(0x10151Bu); draw_text(24u,24u,"SB TERMINAL",0xE9F2FFu); draw_text(24u,56u,"READY",0x7FA8D8u); }

static int bounded_length(const char *text, uint32_t capacity, uint32_t *length) {
    uint32_t n=0u;
    if(text==0 || length==0u || capacity==0u) return -1;
    while(n<capacity && text[n]!='\0') ++n;
    if(n==capacity) return -1;
    *length=n;
    return 0;
}

static int build_path(const char *cwd, const char *argument, char out[TERM_MAX_PATH]) {
    uint32_t cwd_len=0u,arg_len=0u;
    if(cwd==0||argument==0||out==0||bounded_length(cwd,TERM_MAX_PATH,&cwd_len)!=0||bounded_length(argument,TERM_MAX_PATH,&arg_len)!=0||arg_len==0u) return -1;
    if(argument[0]=='/') {
        if(arg_len+1u>TERM_MAX_PATH) return -1;
        for(uint32_t i=0u;i<arg_len;++i) out[i]=argument[i];
        out[arg_len]='\0';
        return 0;
    }
    if(cwd_len==1u&&cwd[0]=='/') {
        if(1u+arg_len+1u>TERM_MAX_PATH) return -1;
        out[0]='/'; for(uint32_t i=0u;i<arg_len;++i) out[i+1u]=argument[i]; out[arg_len+1u]='\0'; return 0;
    }
    if(cwd_len+1u+arg_len+1u>TERM_MAX_PATH) return -1;
    for(uint32_t i=0u;i<cwd_len;++i) out[i]=cwd[i];
    out[cwd_len]='/';
    for(uint32_t i=0u;i<arg_len;++i) out[cwd_len+1u+i]=argument[i];
    out[cwd_len+1u+arg_len]='\0';
    return 0;
}

static int command_is(const char *line, const char *command, uint32_t length) {
    uint32_t command_length=0u;
    if(line==0||command==0) return 0;
    while(command[command_length]!='\0') ++command_length;
    if(length!=command_length)return 0;
    for(uint32_t i=0u;i<length;++i) if(line[i]!=command[i]) return 0;
    return 1;
}

static int command_argument(const char *line, const char *prefix, uint32_t length, const char **argument, uint32_t *argument_length) {
    uint32_t prefix_length=0u;
    while(prefix[prefix_length]!='\0') ++prefix_length;
    if(length<=prefix_length||argument==0||argument_length==0u)return -1;
    for(uint32_t i=0u;i<prefix_length;++i)if(line[i]!=prefix[i])return -1;
    if(line[prefix_length]!=' ')return -1;
    uint32_t start=prefix_length+1u;
    while(start<length&&line[start]==' ')++start;
    if(start>=length)return -1;
    *argument=&line[start];
    *argument_length=length-start;
    return 0;
}

static void draw_listing(const char *path) {
    sb_fs_dir_record_t records[TERM_MAX_OUTPUT/SB_FS_DIR_RECORD_SIZE];
    uint32_t path_length=0u;
    if(bounded_length(path,TERM_MAX_PATH,&path_length)!=0)return;
    const uint64_t bytes=sb_fs_list(path,path_length,records,sizeof(records));
    if(bytes==UINT64_MAX){draw_text(24u,100u,"LS ERROR",0xFF8080u);return;}
    const uint32_t count=(uint32_t)(bytes/SB_FS_DIR_RECORD_SIZE);
    for(uint32_t i=0u;i<count;++i){
        const uint32_t y=100u+i*28u;
        draw_text(24u,y,records[i].type==SB_FS_DIR_TYPE_DIRECTORY?"DIR":"FILE",0x7FA8D8u);
        for(uint32_t j=0u;j<records[i].name_length;++j)draw_char(86u+j*10u,y,records[i].name[j],0xBFD8FFu);
    }
}

static void show_file_data(const char *path, uint32_t path_length) {
    char data[128];
    uint64_t fd=sb_fs_open(path,path_length,SB_FS_OPEN_READ,0u);
    if(fd==UINT64_MAX){draw_text(24u,100u,"CAT ERROR",0xFF8080u);return;}
    draw_text(24u,100u,"DATA",0xBFD8FFu);
    uint32_t total=0u;
    for(;;){
        const uint64_t r=sb_fs_read(fd,data,sizeof(data));
        if(r==UINT64_MAX){draw_text(24u,128u,"READ ERROR",0xFF8080u);break;}
        if(r==0u)break;
        for(uint32_t i=0u;i<(uint32_t)r&&total<32u;++i,++total)draw_char(24u+(total%8u)*90u,128u+(total/8u)*28u,data[i],0xBFD8FFu);
        if(total>=32u)break;
    }
    (void)sb_fs_close(fd);
}

static uint64_t parse_decimal(const char *text,uint32_t start,uint32_t length){uint64_t value=0u;if(text==0||start>=length)return UINT64_MAX;for(uint32_t i=start;i<length;++i){const char c=text[i];if(c<'0'||c>'9')return UINT64_MAX;const uint64_t digit=(uint64_t)(c-'0');if(value>(UINT64_MAX-digit)/10u)return UINT64_MAX;value=value*10u+digit;}return value;}

static void run_command(const char *line, char cwd[TERM_MAX_PATH]) {
    if(line==0||cwd==0)return;
    (void)sb_display_rect(24u,88u,720u,380u,0x10151Bu);
    uint32_t length=0u; while(length<TERM_MAX_LINE&&line[length]!='\0') ++length;
    if(command_is(line,"HELP",length)) draw_text(24u,100u,"HELP LS PWD CD TICKS PID CLEAR CREATE WRITE CAT SLEEP EXIT",0xBFD8FFu);
    else if(command_is(line,"PWD",length)) draw_text(24u,100u,cwd,0xBFD8FFu);
    else if(command_is(line,"LS",length)) draw_listing(cwd);
    else if(length>=3u&&line[0]=='L'&&line[1]=='S'&&line[2]==' '){
        char path[TERM_MAX_PATH]; const char *arg; uint32_t arg_len;
        if(command_argument(line,"LS",length,&arg,&arg_len)!=0||arg_len+1u>TERM_MAX_PATH){draw_text(24u,100u,"LS ERROR",0xFF8080u);return;}
        char arg_copy[TERM_MAX_PATH]; for(uint32_t i=0u;i<arg_len;++i)arg_copy[i]=arg[i];arg_copy[arg_len]='\0';
        if(build_path(cwd,arg_copy,path)!=0){draw_text(24u,100u,"LS ERROR",0xFF8080u);return;} draw_listing(path);
    }
    else if(length>=3u&&line[0]=='C'&&line[1]=='D'&&line[2]==' '){
        char path[TERM_MAX_PATH];const char *arg;uint32_t arg_len;uint32_t path_len=0u;
        if(command_argument(line,"CD",length,&arg,&arg_len)!=0||arg_len+1u>TERM_MAX_PATH){draw_text(24u,100u,"CD ERROR",0xFF8080u);return;}
        char arg_copy[TERM_MAX_PATH];for(uint32_t i=0u;i<arg_len;++i)arg_copy[i]=arg[i];arg_copy[arg_len]='\0';
        if(build_path(cwd,arg_copy,path)!=0||bounded_length(path,TERM_MAX_PATH,&path_len)!=0){draw_text(24u,100u,"CD ERROR",0xFF8080u);return;}
        sb_fs_dir_record_t probe_record[1];
        const uint64_t listed=sb_fs_list(path,path_len,probe_record,sizeof(probe_record));
        if(listed==UINT64_MAX){draw_text(24u,100u,"CD ERROR",0xFF8080u);return;}
        for(uint32_t i=0u;i<TERM_MAX_PATH;++i)cwd[i]=path[i];
        draw_text(24u,100u,"CHANGED",0x80D8A0u);
    }
    else if(command_is(line,"TICKS",length)){draw_text(24u,100u,"TICKS",0xBFD8FFu);draw_hex(96u,100u,sb_get_ticks());}
    else if(command_is(line,"PID",length)){draw_text(24u,100u,"PID",0xBFD8FFu);draw_hex(96u,100u,sb_process_id());}
    else if(command_is(line,"CLEAR",length))clear_terminal();
    else if(command_is(line,"CREATE",length)){
        const char path[]="/SBCMD.TXT";const uint64_t fd=sb_fs_open(path,10u,SB_FS_OPEN_READ|SB_FS_OPEN_WRITE|SB_FS_OPEN_CREATE,128u);
        if(fd==UINT64_MAX)draw_text(24u,100u,"CREATE ERROR",0xFF8080u);else{(void)sb_fs_close(fd);draw_text(24u,100u,"CREATED",0x80D8A0u);}
    }
    else if(command_is(line,"WRITE",length)){
        const char path[]="/SBCMD.TXT",data[]="SB OK";const uint64_t fd=sb_fs_open(path,10u,SB_FS_OPEN_WRITE,0u);
        if(fd==UINT64_MAX)draw_text(24u,100u,"OPEN ERROR",0xFF8080u);else{const uint64_t r=sb_fs_write(fd,data,5u);(void)sb_fs_close(fd);draw_text(24u,100u,r==5u?"WRITTEN":"WRITE ERROR",r==5u?0x80D8A0u:0xFF8080u);}
    }
    else if(command_is(line,"CAT",length)){const char path[]="/SBCMD.TXT";show_file_data(path,10u);}
    else if(length>=7u&&line[0]=='S'&&line[1]=='L'&&line[2]=='E'&&line[3]=='E'&&line[4]=='P'&&line[5]==' '){const uint64_t ticks=parse_decimal(line,6u,length);if(ticks==UINT64_MAX||ticks==0u||ticks>1000000u){draw_text(24u,100u,"SLEEP ERROR",0xFF8080u);return;}draw_text(24u,100u,"SLEEPING",0xBFD8FFu);const uint64_t result=sb_sleep(ticks);if(result!=0u)draw_text(24u,128u,"SLEEP ERROR",0xFF8080u);else{draw_text(24u,128u,"AWAKE",0x80D8A0u);draw_hex(96u,128u,sb_get_ticks());}}
    else draw_text(24u,100u,"UNKNOWN COMMAND",0xFFB070u);
}

uint64_t sb_app_main(void) {
    char line[TERM_MAX_LINE+1u]={0}; char cwd[TERM_MAX_PATH]="/"; uint32_t length=0u;
    clear_terminal(); draw_text(24u,84u,"HELP",0x7FA8D8u); draw_text(24u,440u,"PROMPT",0x7FA8D8u);
    for(;;){const uint64_t key=sb_input_key();if(key==0u){(void)sb_yield();continue;}const uint8_t scancode=(uint8_t)key;if((scancode&0x80u)!=0u)continue;if(scancode==0x01u)return 0u;if(scancode==0x0Eu){if(length>0u){--length;line[length]='\0';}continue;}if(scancode==0x1Cu){line[length]='\0';run_command(line,cwd);length=0u;line[0]='\0';continue;}const char c=scancode_char(scancode);if(c!=0&&length<TERM_MAX_LINE){line[length++]=c;line[length]='\0';draw_char(24u+(length-1u)*10u,440u,c,0xE9F2FFu);}}
}
