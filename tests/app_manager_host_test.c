#include <assert.h>
#include <stdint.h>
#include "../kernel/app_manager.h"
#include "../kernel/process.h"
#include "../kernel/process_exec.h"
#include "../kernel/user_scheduler.h"

static sb_process_t processes[16];
static sb_thread_t threads[16];
static uint32_t process_count_value;
static uint32_t prepare_calls;
static uint32_t scheduler_calls;
static int fail_prepare;
static int fail_scheduler;
static sb_process_t *current_parent;

sb_process_t *process_create(uint64_t pid) {
    if (process_count_value >= 16u) return 0;
    for (uint32_t i=0u;i<16u;++i) if(processes[i].state==SB_PROCESS_UNUSED){sb_process_t *process=&processes[i];*process=(sb_process_t){.pid=pid,.state=SB_PROCESS_CREATED};++process_count_value;return process;}
    return 0;
}

sb_process_t *process_create_child(sb_process_t *parent,uint64_t pid){
    if(parent==0||parent->state==SB_PROCESS_EXITED) return 0;
    sb_process_t *child=process_create(pid); if(child!=0) child->parent_pid=parent->pid; return child;
}

void process_destroy(sb_process_t *process){if(process==0)return;*process=(sb_process_t){.state=SB_PROCESS_UNUSED};if(process_count_value>0u)--process_count_value;}

int process_prepare_boot_module(sb_process_t *process,uint64_t multiboot_info,const char *module_name,sb_process_image_t *image){(void)multiboot_info;++prepare_calls;if(fail_prepare!=0||process==0||module_name==0||image==0)return -1;image->entry_point=0x400000u;image->user_stack_top=0x800000u;image->user_stack_bottom=0x7FC000u;return 0;}

int process_prepare_elf_thread(sb_process_t *process,uint64_t tid,uint32_t priority,sb_user_context_t *context,const sb_process_image_t *image,sb_thread_t **thread_out){(void)priority;if(process==0||context==0||image==0||thread_out==0)return -1;uint32_t index=0u;while(index<16u&&&processes[index]!=process)++index;if(index>=16u)return -1;sb_thread_t *thread=&threads[index];*thread=(sb_thread_t){.tid=tid,.state=SB_PROCESS_CREATED,.user_context=context,.kernel_stack_base=0x10000u+index*0x1000u,.kernel_stack_top=0x10800u+index*0x1000u,.kernel_resume_stack_pointer=0x10400u+index*0x1000u};process->threads[0]=*thread;process->thread_count=1u;*thread_out=thread;return 0;}
int user_scheduler_add(sb_process_t *process,sb_thread_t *thread){++scheduler_calls;return fail_scheduler!=0||process==0||thread==0?-1:0;}
sb_process_t *user_scheduler_current_process(void){return current_parent;}
sb_thread_t *user_scheduler_current_thread(void){return 0;}
int user_scheduler_remove(sb_process_t *process,sb_thread_t *thread){(void)process;(void)thread;return 0;}

int main(void){
    sb_app_manager_init(0x1000u); assert(sb_app_count()==0u); assert(sb_app_launch(0u)!=0); assert(sb_app_launch(99u)!=0);
    fail_prepare=1; assert(sb_app_launch(SB_APP_SETTINGS)!=0); assert(prepare_calls==1u); fail_prepare=0;
    assert(sb_app_launch(SB_APP_SETTINGS)==0); assert(sb_app_count()==1u); assert(sb_app_launch(SB_APP_SETTINGS)!=0); assert(prepare_calls==2u); assert(scheduler_calls==1u);
    assert(sb_app_launch(SB_APP_FILES)==0); assert(sb_app_launch(SB_APP_TERMINAL)==0); assert(sb_app_count()==3u);
    fail_scheduler=1; assert(sb_app_launch(4u)!=0); fail_scheduler=0; assert(sb_app_count()==3u); assert(sb_app_launch(SB_APP_TERMINAL)!=0); assert(sb_app_count()==3u);
    processes[0].state=SB_PROCESS_EXITED; assert(sb_app_reap_exited()==1u); assert(sb_app_count()==2u); assert(sb_app_launch(SB_APP_SETTINGS)==0); assert(sb_app_count()==3u);

    /* Start a genuinely fresh mock process table for the second lifecycle. */
    for (uint32_t i=0u;i<16u;++i) { processes[i]=(sb_process_t){0}; threads[i]=(sb_thread_t){0}; }
    process_count_value=0u;
    current_parent=0;
    sb_app_manager_init(0x2000u);
    processes[0]=(sb_process_t){.pid=0xABCDEF1234567890ull,.state=SB_PROCESS_RUNNING};
    current_parent=&processes[0];
    assert(sb_app_launch(SB_APP_SETTINGS)==0);
    assert(process_count_value==2u);
    assert(processes[1].parent_pid==processes[0].pid);
    current_parent=0;
    return 0;
}
