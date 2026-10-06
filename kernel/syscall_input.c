#include "syscall_input.h"
#include "scheduler.h"
#include "process.h"
#include "user_access.h"
#include "keyboard.h"
#include <suirabox/syscall_abi.h>
_Static_assert(sizeof(sb_key_event_t)==SB_KEY_EVENT_SIZE,"input event ABI size");
_Static_assert(__builtin_offsetof(sb_key_event_t,sequence)==16u,"input sequence ABI offset");
static sb_irq_frame_t *result(sb_irq_frame_t *f, int64_t code) { f->rax=(uint64_t)code; return f; }
sb_irq_frame_t *sb_syscall_dispatch_input(sb_irq_frame_t *f) {
    if (!f) return 0;
    sb_task_t *task=scheduler_current();
    sb_process_t *process=task && task->user_task ? process_get(task->process_id) : 0;
    if (!process || f->rax!=SB_SYS_KEY_EVENT_READ) return result(f,SB_SYS_ERROR_INVALID);
    if (process->pid!=1u) return result(f,SB_SYS_ERROR_RIGHTS);
    if (!f->rdi || f->rsi!=SB_KEY_EVENT_SIZE || f->rdx) return result(f,SB_SYS_ERROR_INVALID);
    if (!sb_keyboard_available()) return result(f,SB_SYS_ERROR_NOT_FOUND);
    /* Validate even if empty, then copy before pop. The interrupt gate keeps
     * the UP queue stable across this transaction; failed copy preserves head. */
    if (user_access_validate(process,f->rdi,SB_KEY_EVENT_SIZE,SB_USER_ACCESS_WRITE)!=0)
        return result(f,SB_SYS_ERROR_FAULT);
    sb_key_event_t event;
    if (!sb_keyboard_peek(&event)) return result(f,SB_SYS_ERROR_WOULD_BLOCK);
    if (user_copy_to(process,f->rdi,&event,sizeof(event))!=0) return result(f,SB_SYS_ERROR_FAULT);
    sb_keyboard_pop();
    return result(f,0);
}
