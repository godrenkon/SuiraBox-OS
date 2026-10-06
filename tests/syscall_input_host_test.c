#include "syscall_input.h"
#include "scheduler.h"
#include "process.h"
#include "user_access.h"
#include "keyboard.h"
#include "key_decoder.h"
#include <suirabox/syscall_abi.h>
#include <stdio.h>
#include <string.h>
static sb_task_t task={.process_id=1u,.user_task=1u};static sb_process_t process={.pid=1u};
static sb_key_decoder_t queue;static sb_key_event_t copied;static int available=1,fail_copy,validations;
sb_task_t *scheduler_current(void){return &task;}
sb_process_t *process_get(uint64_t pid){return pid==task.process_id?&process:0;}
int sb_keyboard_available(void){return available;}
int sb_keyboard_peek(sb_key_event_t *e){return sb_key_decoder_peek(&queue,e);}
void sb_keyboard_pop(void){sb_key_decoder_pop(&queue);}
int user_access_validate(const sb_process_t*p,uint64_t addr,uint64_t size,uint32_t access){
    (void)p;++validations;return addr==0x1000u&&size==32u&&access==1u?0:-1;}
int user_copy_to(const sb_process_t*p,uint64_t addr,const void*src,uint64_t size){
    (void)p;if(fail_copy||addr!=0x1000u||size!=32u)return -1;memcpy(&copied,src,32u);return 0;}
static int64_t call(uint64_t pointer,uint64_t size,uint64_t flags){sb_irq_frame_t f={.rax=45u,.rdi=pointer,.rsi=size,.rdx=flags};
    sb_syscall_dispatch_input(&f);return (int64_t)f.rax;}
static int check(int ok,const char*n){if(!ok){fprintf(stderr,"input syscall FAILED: %s\n",n);return 1;}return 0;}
int main(void){sb_key_decoder_init(&queue);sb_key_decoder_feed(&queue,0x1e);
    if(check(call(1,32,0)==-2&&call(0x2000,32,0)==-2&&queue.count==1,"fault preserves pending key"))return 1;
    if(check(call(0x1000,31,0)==-1&&call(0x1000,32,1)==-1&&call(0,32,0)==-1&&queue.count==1,"bad args"))return 1;
    validations=0;process.pid=2;
    if(check(call(1,32,0)==-5&&!validations&&queue.count==1,"rights before user access"))return 1;
    process.pid=1;fail_copy=1;
    if(check(call(0x1000,32,0)==-2&&queue.count==1,"copy failure preserves head"))return 1;
    fail_copy=0;
    if(check(call(0x1000,32,0)==0&&!queue.count&&copied.keycode=='A'&&copied.flags==1&&copied.sequence==1,
        "copy then commit"))return 1;
    if(check(call(0x1000,32,0)==-8&&call(1,32,0)==-2,"empty nonblocking validates pointer"))return 1;
    available=0;if(check(call(0x1000,32,0)==-6,"unavailable"))return 1;
    if(check(!sb_syscall_dispatch_input(0),"null frame"))return 1;
    puts("Keyboard input syscall host test OK");return 0;}
