#include <assert.h>
#include <stdint.h>
#include "../kernel/process.h"

static uint8_t pages[64][4096];
static uint8_t page_used[64];
static int rebind_calls;
static sb_thread_t *rebind_old;
static sb_thread_t *rebind_new;

int address_space_create(sb_address_space_t *space) { if (space == 0) return -1; space->pml4_physical = 1u; return 0; }
void address_space_destroy(sb_address_space_t *space) { if (space != 0) space->pml4_physical = 0u; }
int address_space_activate(const sb_address_space_t *space) { return space != 0 && space->pml4_physical != 0u ? 0 : -1; }
void *pmm_alloc_page(void) { for (uint32_t i=0u;i<64u;++i) if(page_used[i]==0u){page_used[i]=1u;return pages[i];} return 0; }
void pmm_free_page(void *page) { for(uint32_t i=0u;i<64u;++i) if((void*)pages[i]==page){page_used[i]=0u;return;} }
int user_scheduler_remove(sb_process_t *process, sb_thread_t *thread){(void)process;(void)thread;return 0;}
int user_scheduler_rebind_thread(sb_process_t *process,sb_thread_t *old_thread,sb_thread_t *new_thread){(void)process;rebind_old=old_thread;rebind_new=new_thread;++rebind_calls;return 0;}
int user_scheduler_request_exit(sb_process_t *process,sb_thread_t *thread){(void)process;(void)thread;return -1;}
sb_process_t *user_scheduler_current_process(void){return 0;}
sb_thread_t *user_scheduler_current_thread(void){return 0;}
int sb_user_context_init(sb_user_context_t *context,uint64_t entry_point,uint64_t user_stack_top){(void)entry_point;(void)user_stack_top;if(context!=0)*context=(sb_user_context_t){0};return 0;}
int sb_user_context_validate(const sb_user_context_t *context){return context!=0?0:-1;}
void sb_fs_release_process(void *process){(void)process;}

int main(void) {
    sb_process_t process = {0};
    sb_thread_t *first, *middle, *last;
    process.state = SB_PROCESS_CREATED;
    first=process_create_thread(&process,1u,1u); middle=process_create_thread(&process,2u,2u); last=process_create_thread(&process,3u,3u);
    assert(first!=0&&middle!=0&&last!=0); assert(process.thread_count==3u); assert(process.exit_code==0u);
    rebind_calls=0; assert(process_destroy_thread(&process,middle)==0); assert(process.thread_count==2u);
    assert(process.threads[0].tid==1u&&process.threads[1].tid==3u); assert(page_used[1]==0u&&page_used[2]==1u);
    assert(rebind_calls==1&&rebind_old!=0&&rebind_new==&process.threads[1]&&rebind_old->tid==3u);
    assert(process_destroy_thread(&process,&process.threads[0])==0); assert(process.thread_count==1u); assert(page_used[0]==0u&&page_used[2]==1u);
    assert(process_destroy_thread(&process,&process.threads[0])==0); assert(process.thread_count==0u&&page_used[2]==0u);
    assert(process_destroy_thread(&process,&process.threads[0])!=0);

    {
        sb_process_t lifecycle={0}; sb_thread_t *a,*b;
        lifecycle.state=SB_PROCESS_CREATED; a=process_create_thread(&lifecycle,10u,10u); b=process_create_thread(&lifecycle,11u,10u);
        assert(a!=0&&b!=0); assert(process_exit_thread(&lifecycle,a,41u)==0); assert(a->state==SB_PROCESS_EXITED&&b->state==SB_PROCESS_CREATED);
        assert(lifecycle.state==SB_PROCESS_CREATED&&lifecycle.exit_code==0u); assert(process_exit_thread(&lifecycle,b,42u)==0);
        assert(b->state==SB_PROCESS_EXITED&&lifecycle.state==SB_PROCESS_EXITED&&lifecycle.exit_code==42u);
        assert(process_exit_thread(&lifecycle,b,43u)!=0); assert(process_destroy_thread(&lifecycle,&lifecycle.threads[1])==0); assert(process_destroy_thread(&lifecycle,&lifecycle.threads[0])==0);
    }

    {
        sb_process_t terminated={0}; sb_thread_t *a,*b;
        terminated.state=SB_PROCESS_RUNNING; a=process_create_thread(&terminated,20u,20u); b=process_create_thread(&terminated,21u,20u);
        assert(a!=0&&b!=0); assert(process_terminate(&terminated,99u)==0); assert(terminated.state==SB_PROCESS_EXITED&&terminated.exit_code==99u);
        assert(a->state==SB_PROCESS_EXITED&&b->state==SB_PROCESS_EXITED); assert(process_terminate(&terminated,100u)!=0);
        assert(process_destroy_thread(&terminated,&terminated.threads[0])==0); assert(process_destroy_thread(&terminated,&terminated.threads[0])==0);
    }

    process_init();
    {
        const uint64_t child_pid=0x100000001ull;
        sb_process_t *parent=process_create(1000u); sb_process_t *child=process_create_child(parent,child_pid); uint64_t code=0u;
        assert(parent!=0&&child!=0); assert(child->parent_pid==parent->pid); assert(process_wait_child(parent,child_pid,&code)==0u);
        child->state=SB_PROCESS_EXITED; child->exit_code=0x1122334455667788ull;
        assert(process_wait_child(parent,child_pid,&code)==child_pid); assert(code==0x1122334455667788ull);
        assert(process_get(child_pid)==0); assert(process_count()==1u);
        assert(process_wait_child(parent,child_pid,&code)==UINT64_MAX);
        assert(process_create_child(parent,0u)==0);
        assert(process_create_child(0,1001u)==0);
        assert(process_create(child_pid)!=0);
        process_destroy(parent); assert(process_count()==1u);
        process_destroy(process_get(child_pid)); assert(process_count()==0u);
    }
    return 0;
}
