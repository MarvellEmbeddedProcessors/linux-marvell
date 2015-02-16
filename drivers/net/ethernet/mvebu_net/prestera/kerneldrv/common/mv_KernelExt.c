/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

********************************************************************************
* mvKernelExt.c
*
* DESCRIPTION:
*       functions in kernel mode special for mainOs.
*
* DEPENDENCIES:
*       mvKernelExt.h
*
*       $Revision: 11$
*******************************************************************************/

#include <linux/sched.h>
#include <linux/version.h>

/* task locking data */
static struct task_struct*  mv_tasklock_owner = NULL;
static mv_waitqueue_t       mv_tasklock_waitqueue;
static int                  mv_tasklock_count = 0;
#ifdef MV_TASKLOCK_STAT
static int                  mv_tasklock_lcount = 0;
static int                  mv_tasklock_wcount = 0;
#endif

/* task array */
static int                  mv_max_tasks = MV_MAX_TASKS;
static struct mv_task**     mv_tasks = NULL;
static struct mv_task*      mv_tasks_alloc = NULL;
static int                  mv_num_tasks = 0;

module_param(mv_max_tasks, int, S_IRUGO);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,6)
#define V306PLUS
#endif

/************************************************************************
 *
 * support functions
 *
 ************************************************************************/
#ifndef MVKERNELEXT_TASK_STRUCT
/*******************************************************************************
* gettask
*
* DESCRIPTION:
*       Search for a mv_task by pointer to kernel's task structure
*
* INPUTS:
*       tsk     - pointer to kernel's task structure
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Pointer to mv_task
*       NULL if task is not registered yet
*
* COMMENTS:
*       Interrupts must be disabled when this function called
*
*******************************************************************************/
static struct mv_task* gettask(
        struct task_struct *tsk
)
{
    int k;

    for (k = 0; k < mv_num_tasks; k++)
        if (mv_tasks[k]->task == tsk)
            return mv_tasks[k];

    return NULL;
}
#endif

#ifdef MVKERNELEXT_TASK_STRUCT
static void mv_cleanup(struct task_struct *tsk);
#endif

/*******************************************************************************
* gettask_cr
*
* DESCRIPTION:
*       Search for a mv_task by pointer to kernel's task structure
*       Register task in array if task was not registered yet
*
* INPUTS:
*       tsk     - pointer to kernel's task structure
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Pointer to mv_task
*       NULL if task cannot be registered (task array is full)
*
* COMMENTS:
*       Interrupts must be disabled when this function called
*
*******************************************************************************/
static struct mv_task* gettask_cr(
        struct task_struct *tsk
)
{
    struct mv_task *p;

    p = gettask(tsk);
    if (unlikely(p == NULL))
    {
        if (mv_num_tasks >= mv_max_tasks)
            return NULL;
        p = mv_tasks[mv_num_tasks++];
        memset(p, 0, sizeof(*p));
        p->task = tsk;
#ifdef MVKERNELEXT_TASK_STRUCT
        tsk->mv_ptr = p;
        tsk->mv_cleanup = mv_cleanup;
#endif
    }
    return p;
}

/************************************************************************
 *
 * task locking
 *
 ************************************************************************/
/*******************************************************************************
* mvKernelExt_TaskLock
*
* DESCRIPTION:
*       lock scheduller to current task
*
* INPUTS:
*       tsk     - pointer to kernel's task structure
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_ENOMEM  - current task is not registered
*                              and task array is full
*       -MVKERNELEXT_EINTR   - wait interrupted
*
* COMMENTS:
*       None
*
*******************************************************************************/
int mvKernelExt_TaskLock(struct task_struct* tsk)
{
    struct mv_task *p;

    MV_GLOBAL_LOCK();

    p = gettask_cr(tsk);
    if (unlikely(p == NULL))
    {
        MV_GLOBAL_UNLOCK();
        return -MVKERNELEXT_ENOMEM;
    }

    if (mv_tasklock_owner && mv_tasklock_owner != tsk)
    {
#ifdef MV_TASKLOCK_STAT
        mv_tasklock_wcount++;
        p->tasklock_wcount++;
#endif
        do {
            if (mv_do_short_wait_on_queue(&mv_tasklock_waitqueue, p, &mv_tasklock_owner))
            {
                MV_GLOBAL_UNLOCK();
                return -MVKERNELEXT_EINTR;
            }
        } while (mv_tasklock_owner);
    }

#ifdef MV_TASKLOCK_STAT
    mv_tasklock_lcount++;
    p->tasklock_lcount++;
#endif
    mv_tasklock_owner = tsk;
    mv_tasklock_count++;
    tsk->state = TASK_RUNNING;
    MV_GLOBAL_UNLOCK();

    return 0;
}

/**
 * sys_mv_taskunlock - unlock scheduller from current task
 */
/*******************************************************************************
* mvKernelExt_TaskUnlock
*
* DESCRIPTION:
*       unlock scheduller from current task
*
* INPUTS:
*       tsk     - pointer to kernel's task structure
*       force   - force unlock, reset recursion counter
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EPERM   - locked by enother task
*
* COMMENTS:
*       None
*
*******************************************************************************/
int mvKernelExt_TaskUnlock(struct task_struct* tsk, int force)
{
    MV_GLOBAL_LOCK();
    if (unlikely(mv_tasklock_owner != tsk))
    {
        MV_GLOBAL_UNLOCK();
        return -MVKERNELEXT_EPERM;
    }

    if (force)
        mv_tasklock_count = 0;
    else
        mv_tasklock_count--;

    if (mv_tasklock_count > 0)
    {
        MV_GLOBAL_UNLOCK();
        return 0;
    }

    mv_waitqueue_wake_all(&mv_tasklock_waitqueue);

    mv_tasklock_owner = NULL;
    MV_GLOBAL_UNLOCK();

    return 0;
}

/*******************************************************************************
* mv_registertask
*
* DESCRIPTION:
*       Register current task. Store name for this task
*
* INPUTS:
*       taskinfo - pointer to task info: name priority, etc
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_ENOMEM  - current task is not yet registered
*                              and task array is full
*
* COMMENTS:
*       None
*
*******************************************************************************/
static int mv_registertask(mv_registertask_stc* taskinfo)
{
	struct task_struct *curr;
    struct mv_task *p;

    curr = current;

    MV_GLOBAL_LOCK();

    p = gettask_cr(curr);
    if (unlikely(p == NULL))
    {
        MV_GLOBAL_UNLOCK();
        return -MVKERNELEXT_ENOMEM;
    }

    memcpy(p->name, taskinfo->name, MV_THREAD_NAME_LEN);
    p->name[MV_THREAD_NAME_LEN] = 0;
    p->vxw_priority = taskinfo->vxw_priority;
    p->pthread_id = taskinfo->pthread_id;

    MV_GLOBAL_UNLOCK();
    return 0;
}

/*******************************************************************************
* mv_unregistertask
*
* DESCRIPTION:
*       Unregister task. Unlock mutexes locked by task
*
* INPUTS:
*       tsk     - pointer to kernel's task structure
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       Non zero if task was not registered
*
* COMMENTS:
*       Interrupts must be disabled when this function called
*
*******************************************************************************/
static int mv_unregistertask(struct task_struct* tsk)
{
    struct mv_task* p = NULL;
    int k;

#ifdef MVKERNELEXT_TASK_STRUCT
    tsk->mv_ptr = NULL;
    tsk->mv_cleanup = NULL;
#endif

    mvKernelExt_SemUnlockMutexes(tsk);

    if (mv_tasklock_owner == tsk)
    {
        mv_tasklock_count = 0;
        mv_waitqueue_wake_all(&mv_tasklock_waitqueue);
        mv_tasklock_owner = NULL;
    }

    for (k = 0; k < mv_num_tasks; k++)
        if (mv_tasks[k]->task == tsk)
        {
            p = mv_tasks[k];
            break;
        }

    if (p == NULL)
        return 1;

    mv_num_tasks--;
    if (mv_num_tasks > k) /* task was not the last in array */
    {
        mv_tasks[k] = mv_tasks[mv_num_tasks];
        mv_tasks[mv_num_tasks] = p;
    }

    mv_delete_from_waitqueue(p);

    return 0;
}

#ifdef  MVKERNELEXT_TASK_STRUCT
/*******************************************************************************
* mv_cleanup
*
* DESCRIPTION:
*       Execute cleanup actions when task finished
*       Called from kernel's do_exit() if patch applied to kernel
*
* INPUTS:
*       tsk     - pointer to kernel's task structure
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static void mv_cleanup(struct task_struct *tsk)
{
    MV_GLOBAL_LOCK();
    mv_unregistertask(tsk);
    MV_GLOBAL_UNLOCK();
}
#endif

/*******************************************************************************
* mv_unregister_current_task
*
* DESCRIPTION:
*       Unregister current task
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static int mv_unregister_current_task(void)
{
	struct task_struct *curr;

    curr = current;

    MV_GLOBAL_LOCK();

    mv_unregistertask(curr);

    MV_GLOBAL_UNLOCK();
    return 0;
}

/*******************************************************************************
* get_task_by_id
*
* DESCRIPTION:
*       Search task by task ID
*
* INPUTS:
*       tid    - Task ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Pointer to mv_task
*       Null if task not found
*
* COMMENTS:
*       Interrupts must be disabled when this function called
*
*******************************************************************************/
static struct mv_task* get_task_by_id(int tid)
{
    int k;

    if (tid == 0)
        return gettask(current);

    for (k = 0; k < mv_num_tasks; k++)
        if (mv_tasks[k]->task->pid == tid)
            return mv_tasks[k];

    return NULL;
}

/*******************************************************************************
* mv_get_pthrid
*
* DESCRIPTION:
*       Return pthread ID for task
*       Pthread ID associated with task in mv_registertask
*
* INPUTS:
*       param->taskid      - task ID
*
* OUTPUTS:
*       param->pthread_id  - associated pthread ID
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL  - if task is not registered
*
* COMMENTS:
*
*******************************************************************************/
static int mv_get_pthrid(mv_get_pthrid_stc *param)
{
    struct mv_task* p;

    MV_GLOBAL_LOCK();
    p = get_task_by_id(param->taskid);
    MV_GLOBAL_UNLOCK();

    if (p == NULL)
        return -MVKERNELEXT_EINVAL;

    param->pthread_id = p->pthread_id;

    return 0;
}

/*******************************************************************************
* mv_get_prio
*
* DESCRIPTION:
*       Set task priority
*
* INPUTS:
*       param->taskid       - task ID
*
* OUTPUTS:
*       param->vxw_priority - vxWorks task priority
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL  - if task is not registered
*
* COMMENTS:
*
*******************************************************************************/
static int mv_get_prio(mv_priority_stc *param)
{
    struct mv_task* p;

    MV_GLOBAL_LOCK();
    p = get_task_by_id(param->taskid);
    MV_GLOBAL_UNLOCK();

    if (p == NULL)
        return -MVKERNELEXT_EINVAL;

    param->vxw_priority = p->vxw_priority;

    return 0;
}

/*******************************************************************************
* mv_suspend
*
* DESCRIPTION:
*       Suspend task execution
*
* INPUTS:
*       taskid       - task ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL  - if task is not registered
*
* COMMENTS:
*
*******************************************************************************/
static int mv_suspend(int taskid)
{
    struct mv_task* p;

    MV_GLOBAL_LOCK();
    p = get_task_by_id(taskid);
    MV_GLOBAL_UNLOCK();

    if (p == NULL)
        return -MVKERNELEXT_EINVAL;

    if (p->task == current)
        mvKernelExt_TaskUnlock(p->task, 1);

    send_sig(SIGSTOP, p->task, 1);

    return 0;
}

/*******************************************************************************
* mv_resume
*
* DESCRIPTION:
*       Resume task execution
*
* INPUTS:
*       taskid       - task ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL  - if task is not registered
*
* COMMENTS:
*
*******************************************************************************/
static int mv_resume(int taskid)
{
    struct mv_task* p;

    MV_GLOBAL_LOCK();
    p = get_task_by_id(taskid);
    MV_GLOBAL_UNLOCK();

    if (p == NULL)
        return -MVKERNELEXT_EINVAL;

    send_sig(SIGCONT, p->task, 1);

    return 0;
}

/*******************************************************************************
* mv_delete
*
* DESCRIPTION:
*       Destroy task
*
* INPUTS:
*       taskid       - task ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL  - if task is not registered
*
* COMMENTS:
*
*******************************************************************************/
static int mv_delete(int taskid)
{
    struct mv_task* p;
    struct task_struct *tsk;

    MV_GLOBAL_LOCK();
    p = get_task_by_id(taskid);
    MV_GLOBAL_UNLOCK();

    if (p == NULL)
        return -MVKERNELEXT_EINVAL;

    tsk = p->task;

    MV_GLOBAL_LOCK();
    mv_unregistertask(tsk);
    MV_GLOBAL_UNLOCK();

    send_sig(SIGRTMIN/*SIGCANCEL*/, tsk, 1);
    return 0;
}

/*******************************************************************************
* mvKernelExt_ioctl
*
* DESCRIPTION:
*       The device ioctl() implementation
*
* INPUTS:
*       inode        - unused
*       filp         - unused
*       cmd          - Ioctl number
*       arg          - parameter
*
* OUTPUTS:
*       Depends on cmd
*
* RETURNS:
*       -ENOTTY   - wrong ioctl magic key
*       Depends on cmd
*
* COMMENTS:
*
*******************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
static long mvKernelExt_ioctl(
        struct file *filp,
        unsigned int cmd,
        unsigned long arg
)
#else
static int mvKernelExt_ioctl(
        struct inode *inode,
        struct file *filp,
        unsigned int cmd,
        unsigned long arg
)
#endif
{
    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (unlikely(_IOC_TYPE(cmd) != MVKERNELEXT_IOC_MAGIC))
    {
        printk("wrong ioctl magic key\n");
        return -ENOTTY;
    }

    /* GETTING DATA */
    switch(cmd)
    {
        case MVKERNELEXT_IOC_NOOP:
            return 0;

        case MVKERNELEXT_IOC_TASKLOCK:
            return mvKernelExt_TaskLock(current);

        case MVKERNELEXT_IOC_TASKUNLOCK:
            return mvKernelExt_TaskUnlock(current, 0);

        case MVKERNELEXT_IOC_TASKUNLOCKFORCE:
            return mvKernelExt_TaskUnlock(current, 1);

        case MVKERNELEXT_IOC_REGISTER:
            {
                mv_registertask_stc lparam;
                if (copy_from_user(&lparam,
                            (mv_registertask_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;

                return mv_registertask(&lparam);
            }
        case MVKERNELEXT_IOC_UNREGISTER:
            return mv_unregister_current_task();

        case MVKERNELEXT_IOC_SET_PRIO:
            {
                mv_priority_stc lparam;

                if (copy_from_user(&lparam,
                            (mv_priority_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;

                return mv_set_prio(&lparam);
            }
        case MVKERNELEXT_IOC_GET_PRIO:
            {
                mv_priority_stc lparam;
                int retval;

                if (copy_from_user(&lparam,
                            (mv_priority_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;

                retval = mv_get_prio(&lparam);

                if (copy_to_user((mv_priority_stc*)arg,
                            &lparam, sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;

                return retval;
            }

        case MVKERNELEXT_IOC_SUSPEND:
            return mv_suspend(arg);
        case MVKERNELEXT_IOC_RESUME:
            return mv_resume(arg);

        case MVKERNELEXT_IOC_GET_PTHRID:
            {
                mv_get_pthrid_stc lparam;
                int retval;

                if (copy_from_user(&lparam,
                            (mv_get_pthrid_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;

                retval = mv_get_pthrid(&lparam);

                if (copy_to_user((mv_get_pthrid_stc*)arg,
                        &lparam, sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;

                return retval;
            }

        case MVKERNELEXT_IOC_DELETE:
            return mv_delete(arg);

        case MVKERNELEXT_IOC_SEMCREATE:
            {
                mv_sem_create_stc lparam;
                if (copy_from_user(&lparam,
                            (mv_sem_create_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;

                return mvKernelExt_SemCreate(lparam.flags, lparam.name);
            }
        case MVKERNELEXT_IOC_SEMDELETE:
            return mvKernelExt_SemDelete(arg);
        case MVKERNELEXT_IOC_SEMSIGNAL:
            return mvKernelExt_SemSignal(arg);
        case MVKERNELEXT_IOC_SEMWAIT:
            return mvKernelExt_SemWait(arg);
        case MVKERNELEXT_IOC_SEMTRYWAIT:
            return mvKernelExt_SemTryWait(arg);
        case MVKERNELEXT_IOC_SEMWAITTMO:
            {
                mv_sem_timedwait_stc lparam;
                if (copy_from_user(&lparam,
                            (mv_sem_timedwait_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;

                return mvKernelExt_SemWaitTimeout(lparam.semid, lparam.timeout);
            }

        case MVKERNELEXT_IOC_TEST:
            printk("mvKernelExt_TEST()\n");
            return 0;

        case MVKERNELEXT_IOC_MSGQCREATE:
            {
                mv_msgq_create_stc lparam;
                if (copy_from_user(&lparam,
                            (mv_msgq_create_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;
                return mvKernelExt_MsgQCreate(
                        lparam.name,
                        lparam.maxMsgs,
                        lparam.maxMsgSize);
            }
        case MVKERNELEXT_IOC_MSGQDELETE:
            return mvKernelExt_MsgQDelete(arg);
        case MVKERNELEXT_IOC_MSGQSEND:
            {
                mv_msgq_sr_stc lparam;
                if (copy_from_user(&lparam,
                            (mv_msgq_sr_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;
                return mvKernelExt_MsgQSend(
                        lparam.msgqId,
                        lparam.message,
                        lparam.messageSize,
                        lparam.timeOut,
                        1/*from userspace*/);
            }
        case MVKERNELEXT_IOC_MSGQRECV:
            {
                mv_msgq_sr_stc lparam;
                if (copy_from_user(&lparam,
                            (mv_msgq_sr_stc*)arg,
                            sizeof(lparam)))
                    return -MVKERNELEXT_EINVAL;
                return mvKernelExt_MsgQRecv(
                        lparam.msgqId,
                        lparam.message,
                        lparam.messageSize,
                        lparam.timeOut,
                        1/*to userspace*/);
            }
        case MVKERNELEXT_IOC_MSGQNUMMSGS:
            return mvKernelExt_MsgQNumMsgs(arg);

        default:
            printk (KERN_WARNING "Unknown ioctl (%x).\n", cmd);
            break;
    }
    return 0;
}


/*******************************************************************************
* mvKernelExt_open
*
* DESCRIPTION:
*       The device open() implementation
*
* INPUTS:
*       inode        - unused
*       filp         - unused
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -EIO   - module uninitialized
*
* COMMENTS:
*
*******************************************************************************/
static int mvKernelExt_open(
        struct inode * inode,
        struct file * filp
)
{
    if (!mvKernelExt_initialized)
    {
        return -EIO;
    }

    filp->private_data = NULL;

    mv_check_tasks();

    MV_GLOBAL_LOCK();
    mvKernelExt_opened++;
    MV_GLOBAL_UNLOCK();

    return 0;
}


/*******************************************************************************
* mvKernelExt_release
*
* DESCRIPTION:
*       The device close() implementation
*
* INPUTS:
*       inode        - unused
*       filp         - unused
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*
* COMMENTS:
*
*******************************************************************************/
static int mvKernelExt_release(
        struct inode * inode,
        struct file * file
)
{
    printk("mvKernelExt_release\n");

    /*!!! check task list */
    mv_check_tasks();

    MV_GLOBAL_LOCK();
    mv_unregistertask(current);
    mvKernelExt_opened--;
    if (mvKernelExt_opened == 0)
    {
        mvKernelExt_DeleteAll();
        mvKernelExt_DeleteAllMsgQ();
        mv_tasklock_owner = NULL;
        mv_tasklock_count = 0;
        /* clean statistics */
#ifdef MV_TASKLOCK_STAT
        mv_tasklock_lcount = 0;
        mv_tasklock_wcount = 0;
#endif
    }
    MV_GLOBAL_UNLOCK();

    return 0;
}




static struct file_operations mvKernelExt_fops =
{
    .llseek = mvKernelExt_lseek,
    .read   = mvKernelExt_read,
    .write  = mvKernelExt_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
    .unlocked_ioctl  = mvKernelExt_ioctl,
#else
    .ioctl  = mvKernelExt_ioctl,
#endif
    .open   = mvKernelExt_open,
    .release= mvKernelExt_release /* A.K.A close */
};

#ifdef MVKERNELEXT_SYSCALLS
/************************************************************************
 *
 * syscall entries for fast calls
 *
 ************************************************************************/
/* fast call to KernelExt ioctl */
asmlinkage long sys_mv_ctl(unsigned int cmd, unsigned long arg)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
    return mvKernelExt_ioctl(NULL, cmd, arg);
#else
    return mvKernelExt_ioctl(NULL, NULL, cmd, arg);
#endif
}

extern long sys_call_table[];

#define OWN_SYSCALLS 1

#ifdef __NR_SYSCALL_BASE
#  define __SYSCALL_TABLE_INDEX(name) (__NR_##name-__NR_SYSCALL_BASE)
#else
#  define __SYSCALL_TABLE_INDEX(name) (__NR_##name)
#endif

#define __TBL_ENTRY(name) { __SYSCALL_TABLE_INDEX(name), (long)sys_##name, 0 }
static struct {
    int     entry_number;
    long    own_entry;
    long    saved_entry;
} override_syscalls[OWN_SYSCALLS] = {
    __TBL_ENTRY(mv_ctl)
};
#undef  __TBL_ENTRY


/*******************************************************************************
* mv_OverrideSyscalls
*
* DESCRIPTION:
*       Override entries in syscall table.
*       Store original pointers in array.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static int
mv_OverrideSyscalls(void)
{
    int k;
    for (k = 0; k < OWN_SYSCALLS; k++)
    {
        override_syscalls[k].saved_entry =
            sys_call_table[override_syscalls[k].entry_number];
        sys_call_table[override_syscalls[k].entry_number] =
            override_syscalls[k].own_entry;
    }
    return 0;
}

/*******************************************************************************
* mv_RestoreSyscalls
*
* DESCRIPTION:
*       Restore original syscall entries.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static int
mv_RestoreSyscalls(void)
{
    int k;
    for (k = 0; k < OWN_SYSCALLS; k++)
    {
        if (override_syscalls[k].saved_entry)
            sys_call_table[override_syscalls[k].entry_number] =
                override_syscalls[k].saved_entry;
    }
    return 0;
}
#endif /* MVKERNELEXT_SYSCALLS */


/************************************************************************
* mvKernelExt_cleanup_common
*
* DESCRIPTION:
*       Perform cleanup actions while module unloading
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static void mvKernelExt_cleanup_common(void)
{
#ifdef MVKERNELEXT_TASK_STRUCT
    int k;
#endif

#ifdef MVKERNELEXT_SYSCALLS
    mv_RestoreSyscalls();
#endif

#ifdef MVKERNELEXT_TASK_STRUCT
    MV_GLOBAL_LOCK();
    for (k = 0; k < mv_num_tasks; k++)
    {
        mv_tasks[k]->task->mv_ptr = NULL;
        mv_tasks[k]->task->mv_cleanup = NULL;
    }
    MV_GLOBAL_UNLOCK();
#endif

    mvKernelExt_SemCleanup();
    mvKernelExt_MsgQCleanup();

    if (mv_tasks)
        kfree(mv_tasks);
    mv_tasks = NULL;

    if (mv_tasks_alloc)
        kfree(mv_tasks_alloc);
    mv_tasks_alloc = NULL;
    mv_waitqueue_cleanup(&mv_tasklock_waitqueue);

}

/************************************************************************
* mvKernelExt_init_common
*
* DESCRIPTION:
*       Module initialization
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -ENOMEM if failed to allocate memory for tasks/semaphores
*
* COMMENTS:
*
*******************************************************************************/
static int mvKernelExt_init_common(void)
{
    int         result = 0;

    /* allocate task array for tasklock */
    mv_tasks_alloc = (struct mv_task*) kmalloc(sizeof(struct mv_task) * mv_max_tasks, GFP_KERNEL);
    mv_tasks = (struct mv_task**) kmalloc(sizeof(struct mv_task*) * mv_max_tasks, GFP_KERNEL);
    if (mv_tasks_alloc == NULL || mv_tasks == NULL)
    {
        if (mv_tasks)
            kfree(mv_tasks);
        if (mv_tasks_alloc)
            kfree(mv_tasks_alloc);
        mv_tasks = NULL;
        mv_tasks_alloc = NULL;
        result = -ENOMEM;
        printk("mvKernelExt_init: unable to allocate task array\n");
        return result;
    }
    for (result = 0; result < mv_max_tasks; result++)
        mv_tasks[result] = &(mv_tasks_alloc[result]);
    mv_waitqueue_init(&mv_tasklock_waitqueue);

    if (!mvKernelExt_SemInit())
    {
        mvKernelExt_cleanup_common();
        result = -ENOMEM;
        return result;
    }

    if (!mvKernelExt_MsgQInit())
    {
        mvKernelExt_cleanup_common();
        result = -ENOMEM;
        return result;
    }

#ifdef MVKERNELEXT_SYSCALLS
    mv_OverrideSyscalls();
#endif

    return 0;
}

EXPORT_SYMBOL(mvKernelExt_TaskLock);
EXPORT_SYMBOL(mvKernelExt_TaskUnlock);
