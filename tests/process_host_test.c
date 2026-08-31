#include <assert.h>
#include <stdint.h>
#include "../kernel/process.h"

static uint8_t pages[64][4096];
static uint8_t page_used[64];
static int rebind_calls;
static sb_thread_t *rebind_old;
static sb_thread_t *rebind_new;
static sb_process_t *current_process_for_test;
static sb_thread_t *current_thread_for_test;
static int request_exit_result = -1;
static int request_exit_calls;
static int request_exit_saw_runnable;

int address_space_create(sb_address_space_t *space) { if (space == 0) return -1; space->pml4_physical = 1u; return 0; }
void address_space_destroy(sb_address_space_t *space) { if (space != 0) space->pml4_physical = 0u; }
int address_space_activate(const sb_address_space_t *space) { return space != 0 && space->pml4_physical != 0u ? 0 : -1; }
void *pmm_alloc_page(void) { for (uint32_t i=0u;i<64u;++i) if(page_used[i]==0u){page_used[i]=1u;return pages[i];} return 0; }
void pmm_free_page(void *page) { for(uint32_t i=0u;i<64u;++i) if((void*)pages[i]==page){page_used[i]=0u;return;} }
int user_scheduler_remove(sb_process_t *process, sb_thread_t *thread){(void)process;(void)thread;return 0;}
int user_scheduler_rebind_thread(sb_process_t *process,sb_thread_t *old_thread,sb_thread_t *new_thread){(void)process;rebind_old=old_thread;rebind_new=new_thread;++rebind_calls;return 0;}
int user_scheduler_request_exit(sb_process_t *process,sb_thread_t *thread){(void)process;++request_exit_calls;request_exit_saw_runnable = thread != 0 && (thread->state == SB_PROCESS_CREATED || thread->state == SB_PROCESS_RUNNING);return request_exit_result;}
sb_process_t *user_scheduler_current_process(void){return current_process_for_test;}
sb_thread_t *user_scheduler_current_thread(void){return current_thread_for_test;}
int sb_user_context_init(sb_user_context_t *context,uint64_t entry_point,uint64_t user_stack_top){(void)entry_point;(void)user_stack_top;if(context!=0)*context=(sb_user_context_t){0};return 0;}
int sb_user_context_validate(const sb_user_context_t *context){return context!=0?0:-1;}

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
        lifecycle.state=SB_PROCESS_CREATED; a=process_create_thread(&lifecycle,10u,10u); b=process_create_thread(&lifecycle,11u,11u);
        assert(a!=0&&b!=0); assert(process_exit_thread(&lifecycle,a,41u)==0); assert(a->state==SB_PROCESS_EXITED&&b->state==SB_PROCESS_CREATED);
        assert(lifecycle.state==SB_PROCESS_CREATED&&lifecycle.exit_code==0u); assert(process_exit_thread(&lifecycle,b,42u)==0);
        assert(b->state==SB_PROCESS_EXITED&&lifecycle.state==SB_PROCESS_EXITED&&lifecycle.exit_code==42u);
        assert(process_exit_thread(&lifecycle,b,43u)!=0); assert(process_destroy_thread(&lifecycle,&lifecycle.threads[1])==0); assert(process_destroy_thread(&lifecycle,&lifecycle.threads[0])==0);
    }

    {
        sb_process_t terminated={0}; sb_thread_t *a,*b;
        terminated.state=SB_PROCESS_RUNNING; a=process_create_thread(&terminated,20u,20u); b=process_create_thread(&terminated,21u,21u);
        assert(a!=0&&b!=0); assert(process_terminate(&terminated,99u)==0); assert(terminated.state==SB_PROCESS_EXITED&&terminated.exit_code==99u);
        assert(a->state==SB_PROCESS_EXITED&&b->state==SB_PROCESS_EXITED); assert(process_terminate(&terminated,100u)!=0);
        assert(process_destroy_thread(&terminated,&terminated.threads[0])==0); assert(process_destroy_thread(&terminated,&terminated.threads[0])==0);
    }

    {
        sb_process_t rollback={0};
        sb_thread_t *thread;
        rollback.state=SB_PROCESS_RUNNING;
        thread=process_create_thread(&rollback,30u,30u);
        assert(thread!=0);
        thread->state=SB_PROCESS_RUNNING;
        thread->runtime_ticks=77u;
        current_process_for_test=&rollback;
        current_thread_for_test=thread;
        request_exit_result=-1;
        request_exit_calls=0;
        request_exit_saw_runnable=0;
        assert(process_exit_thread(&rollback,thread,55u)!=0);
        assert(request_exit_calls==1&&request_exit_saw_runnable==1);
        assert(thread->state==SB_PROCESS_RUNNING);
        assert(thread->runtime_ticks==77u);
        assert(rollback.state==SB_PROCESS_RUNNING);
        current_process_for_test=0;
        current_thread_for_test=0;
        assert(process_destroy_thread(&rollback,thread)==0);
    }

    {
        sb_process_t current={0};
        sb_thread_t *thread;
        current.state=SB_PROCESS_RUNNING;
        thread=process_create_thread(&current,35u,35u);
        assert(thread!=0);
        thread->state=SB_PROCESS_RUNNING;
        thread->runtime_ticks=17u;
        current_process_for_test=&current;
        current_thread_for_test=thread;
        request_exit_result=0;
        request_exit_calls=0;
        request_exit_saw_runnable=0;
        assert(process_exit_thread(&current,thread,56u)==0);
        assert(request_exit_calls==1&&request_exit_saw_runnable==1);
        assert(thread->state==SB_PROCESS_EXITED);
        assert(thread->runtime_ticks==0u);
        assert(current.state==SB_PROCESS_EXITED&&current.exit_code==56u);
        current_process_for_test=0;
        current_thread_for_test=0;
        request_exit_result=-1;
        assert(process_destroy_thread(&current,thread)==0);
    }

    {
        sb_process_t terminate_rollback={0};
        sb_thread_t *a,*b;
        terminate_rollback.state=SB_PROCESS_RUNNING;
        a=process_create_thread(&terminate_rollback,40u,40u);
        b=process_create_thread(&terminate_rollback,41u,40u);
        assert(a!=0&&b!=0);
        a->state=SB_PROCESS_RUNNING;
        b->state=SB_PROCESS_RUNNING;
        a->runtime_ticks=88u;
        b->runtime_ticks=99u;
        current_process_for_test=&terminate_rollback;
        current_thread_for_test=a;
        request_exit_result=-1;
        request_exit_calls=0;
        request_exit_saw_runnable=0;
        assert(process_terminate(&terminate_rollback,123u)!=0);
        assert(request_exit_calls==1&&request_exit_saw_runnable==1);
        assert(terminate_rollback.state==SB_PROCESS_RUNNING);
        assert(terminate_rollback.exit_code==0u);
        assert(a->state==SB_PROCESS_RUNNING&&a->runtime_ticks==88u);
        assert(b->state==SB_PROCESS_RUNNING&&b->runtime_ticks==99u);
        current_process_for_test=0;
        current_thread_for_test=0;
        assert(process_destroy_thread(&terminate_rollback,a)==0);
        assert(process_destroy_thread(&terminate_rollback,&terminate_rollback.threads[0])==0);
    }

    {
        sb_process_t current_destroy={0};
        sb_thread_t *thread;
        current_destroy.state=SB_PROCESS_RUNNING;
        thread=process_create_thread(&current_destroy,50u,50u);
        assert(thread!=0);
        thread->state=SB_PROCESS_RUNNING;
        current_process_for_test=&current_destroy;
        current_thread_for_test=thread;
        assert(process_destroy_thread(&current_destroy,thread)==-2);
        assert(current_destroy.thread_count==1u);
        assert(thread->tid==50u&&thread->state==SB_PROCESS_RUNNING);
        assert(page_used[0]==1u);
        current_process_for_test=0;
        current_thread_for_test=0;
        assert(process_destroy_thread(&current_destroy,thread)==0);
        assert(page_used[0]==0u);
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

    process_init();
    {
        sb_process_t *active = process_create(2000u);
        sb_thread_t *thread = active != 0 ? process_create_thread(active, 60u, 60u) : 0;
        assert(active != 0 && thread != 0);
        thread->state = SB_PROCESS_RUNNING;
        current_process_for_test = active;
        current_thread_for_test = thread;
        process_destroy(active);
        assert(process_count() == 1u);
        assert(active->state == SB_PROCESS_CREATED);
        assert(active->thread_count == 1u);
        assert(page_used[0] == 1u);
        current_process_for_test = 0;
        current_thread_for_test = 0;
        process_destroy(active);
        assert(process_count() == 0u);
        assert(page_used[0] == 0u);
    }
    return 0;
}
