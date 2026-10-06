#include "ps2_controller.h"
#include <stdio.h>
typedef struct {uint8_t data,config,next_config;unsigned pending,reads,writes,resends,f5_count;
    unsigned mode;uint16_t ports[32];uint8_t values[32];} mock_t;
static int check(int ok,const char *name){if(!ok){fprintf(stderr,"controller FAILED: %s\n",name);return 1;}return 0;}
static uint8_t read_port(void *context,uint16_t port){
    mock_t*m=context;++m->reads;
    if(port==0x64u){if(m->mode==1)return 0xffu;if(m->mode==2)return 2u;
        return m->pending?(uint8_t)(1u|(m->mode==6?0x80u:0u)):0u;}
    m->pending=0u;return m->data;
}
static void write_port(void *context,uint16_t port,uint8_t value){
    mock_t*m=context;
    if(m->writes<32u){m->ports[m->writes]=port;m->values[m->writes]=value;}++m->writes;
    if(port==0x64u){if(value==0x20u){m->data=m->config;m->pending=1u;}if(value==0x60u)m->next_config=1u;return;}
    if(m->next_config){m->config=value;m->next_config=0u;return;}
    if(value==0xf5u)++m->f5_count;
    if(m->mode==3)return; /* no command reply */
    m->data=m->mode==4?0xeeu:(m->mode==5||m->resends?0xfeu:0xfau);
    if(m->resends)--m->resends;
    m->pending=1u;
}
int main(void){
    mock_t m={.config=0x34u};sb_ps2_io_t io={.read=read_port,.write=write_port,.context=&m};
    if(check(sb_ps2_keyboard_start(&io)&&m.config==0x65u&&m.writes==12u,"configure translated set 1 and IRQ1 only"))return 1;
    const uint8_t values[]={0xad,0xa7,0x20,0x60,0x64,0xae,0xf5,0xf0,2,0xf4,0x60,0x65};
    const uint16_t ports[]={0x64,0x64,0x64,0x64,0x60,0x64,0x60,0x60,0x60,0x60,0x64,0x60};
    for(unsigned i=0;i<12u;++i)if(check(m.values[i]==values[i]&&m.ports[i]==ports[i],"command order"))return 1;
    for(unsigned mode=1;mode<=6u;++mode){m=(mock_t){.config=0x34u,.mode=mode};
        if(check(!sb_ps2_keyboard_start(&io)&&m.reads<=4u*SB_PS2_POLL_LIMIT+100u,"failure terminates boundedly"))return 1;
        if(mode==1||mode==2){if(check(!m.writes,"absent/busy controller untouched"))return 1;}
        else if(mode!=6){if(check((m.config&0x11u)==0x10u,"failure leaves IRQ disabled and port off"))return 1;}
        if(mode==5&&check(m.f5_count==3u,"bounded resend count"))return 1;
    }
    m=(mock_t){.config=0x34u,.resends=2u};
    if(check(sb_ps2_keyboard_start(&io)&&m.f5_count==3u,"resend then ACK recovers"))return 1;
    if(check(!sb_ps2_keyboard_start(0),"missing IO"))return 1;
    puts("PS/2 controller host test OK");return 0;
}
