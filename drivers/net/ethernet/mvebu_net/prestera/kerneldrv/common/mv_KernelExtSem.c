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
* mvKernelExtSem.c
*
* DESCRIPTION:
*       functions in kernel mode special for mainOs.
*       Semaphores implementation
*
* DEPENDENCIES:
*
*       $Revision: 6$
*******************************************************************************/

#define MV_SEM_STAT
typedef struct {
    int                 flags;
    int                 count;
    mv_waitqueue_t      waitqueue;
    struct task_struct  *owner;
    char name[MV_SEM_NAME_LEN+1];
#ifdef  MV_SEM_STAT
    int                 tcount;
    int                 gcount;
    int                 wcount;
#endif
} mvSemaphoreSTC;

static mvSemaphoreSTC   *mvSemaphores = NULL;
static int              mv_num_semaphores = MV_SEMAPHORES_DEF;

module_param(mv_num_semaphores, int, S_IRUGO);


/*******************************************************************************
* mvKernelExtSem_read_proc_mem
*
* DESCRIPTION:
*       proc read data rooutine.
*       Use cat /proc/mvKernelExtSem to show semaphore list
*
* INPUTS:
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Data length
*
* COMMENTS:
*
*******************************************************************************/
static int mvKernelExtSem_read_proc_mem(
        char * page,
        char **start,
        off_t offset,
        int count,
        int *eof,
        void *data)
{
    int len;
    int k;
    int begin = 0;

    len = 0;

    len += sprintf(page+len,"id type count owner");
#ifdef MV_SEM_STAT
    len += sprintf(page+len," tcount gcount wcount");
#endif
    len += sprintf(page+len," name\n");
    for (k = 1; k < mv_num_semaphores; k++)
    {
        mvSemaphoreSTC *sem;
        struct mv_task *p;

        if (!mvSemaphores[k].flags)
            continue;
        sem = mvSemaphores + k;

        len += sprintf(page+len,"%d %c %d %d",
                k,
                (sem->flags & MV_SEMAPTHORE_F_MTX) ? 'M' :
                    (sem->flags & MV_SEMAPTHORE_F_COUNT) ? 'C' :
                    (sem->flags & MV_SEMAPTHORE_F_BINARY) ? 'B' : '?',
                sem->count,
                sem->owner?(sem->owner->pid):0);
#ifdef MV_SEM_STAT
        len += sprintf(page+len," %d %d %d", sem->tcount, sem->gcount, sem->wcount);
#endif
        if (sem->name[0])
            len += sprintf(page+len," %s", sem->name);
        page[len++] = '\n';

        for (p = sem->waitqueue.first; p; p = p->wait_next)
            len += sprintf(page+len, "  q=%d\n", p->task->pid);

        if (len+begin < offset)
        {
            begin += len;
            len = 0;
        }
        if (len+begin >= offset+count)
            break;
    }
    if (len+begin < offset)
        *eof = 1;
    offset -= begin;
    *start = page + offset;
    len -= offset;
    if (len > count)
        len = count;
    if (len < 0)
        len = 0;

    return len;
}

/*******************************************************************************
* mvKernelExt_SemInit
*
* DESCRIPTION:
*       Initialize semaphore support, create /proc for semaphores info
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Non zero if successful
*       Zero if failed
*
* COMMENTS:
*
*******************************************************************************/
static int mvKernelExt_SemInit(void)
{
    if (mv_num_semaphores < MV_SEMAPHORES_MIN)
        mv_num_semaphores = MV_SEMAPHORES_MIN;

    mvSemaphores = (mvSemaphoreSTC*) kmalloc(
            mv_num_semaphores * sizeof(mvSemaphoreSTC), GFP_KERNEL);

    if (mvSemaphores == NULL)
    {
        if (mvSemaphores)
            kfree(mvSemaphores);
        mvSemaphores = NULL;
        return 0;
    }

    memset(mvSemaphores, 0, mv_num_semaphores * sizeof(mvSemaphoreSTC));

    /* create proc entry */
    create_proc_read_entry("mvKernelExtSem", 0, NULL, mvKernelExtSem_read_proc_mem, NULL);

    return 1;
}


/*******************************************************************************
* mvKernelExt_DeleteAll
*
* DESCRIPTION:
*       Destroys all semaphores
*       This is safety action which is executed when all tasks closed
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
static void mvKernelExt_DeleteAll(void)
{
    int k;

    for (k = 1; k < mv_num_semaphores; k++)
    {
        if (mvSemaphores[k].flags)
        {
            mv_waitqueue_wake_all(&(mvSemaphores[k].waitqueue));
            mv_waitqueue_cleanup(&(mvSemaphores[k].waitqueue));
        }
        mvSemaphores[k].flags = 0;
    }
}

/*******************************************************************************
* mvKernelExt_SemCleanup
*
* DESCRIPTION:
*       Perform semaphore cleanup actions before module unload
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
static void mvKernelExt_SemCleanup(void)
{
    MV_GLOBAL_LOCK();

    if (mvSemaphores)
    {
        mvKernelExt_DeleteAll();
        kfree(mvSemaphores);
    }

    mvSemaphores = NULL;

    MV_GLOBAL_UNLOCK();

    remove_proc_entry("mvKernelExtSem", NULL);
}

/*******************************************************************************
* mvKernelExt_SemCreate
*
* DESCRIPTION:
*       Create a new semaphore or open existing one
*
* INPUTS:
*       arg   - pointer to structure with creation flags and semaphore name
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Positive value         - semaphore ID
*       -MVKERNELEXT_EINVAL    - invalid parameter passed
*       -MVKERNELEXT_ENOMEM    - semaphore array is full
*       -MVKERNELEXT_ECONFLICT - open existing semaphore with different type
*                                specified
*
*
* COMMENTS:
*
*******************************************************************************/
int mvKernelExt_SemCreate(int flags, const char *name)
{
    int k;
    mvSemaphoreSTC *sem = NULL;

    if ((flags & MV_SEMAPTHORE_F_TYPE_MASK) == 0)
        return -MVKERNELEXT_EINVAL;

    MV_GLOBAL_LOCK();

    if (flags & MV_SEMAPTHORE_F_OPENEXIST)
    {
        /* try to find existing semaphore first */
        for (k = 1; k < mv_num_semaphores; k++)
        {
            sem = mvSemaphores + k;
            if (!sem->flags)
                continue;

            if (!strncmp(sem->name, name, MV_SEM_NAME_LEN))
                break;
        }

        if (k < mv_num_semaphores)
        {
            /* found */
            if ((sem->flags & MV_SEMAPTHORE_F_TYPE_MASK) !=
                    (flags & MV_SEMAPTHORE_F_TYPE_MASK))
            {
                /* semaphore has different type */
                MV_GLOBAL_UNLOCK();
                return -MVKERNELEXT_ECONFLICT;
            }

            MV_GLOBAL_UNLOCK();
            return k;
        }
    }

    /* create semaphore */
    for (k = 1; k < mv_num_semaphores; k++)
    {
        if (mvSemaphores[k].flags == 0)
            break;
    }
    if (k >= mv_num_semaphores)
    {
        MV_GLOBAL_UNLOCK();
        return -MVKERNELEXT_ENOMEM;
    }
    sem = mvSemaphores + k;

    memset(sem, 0, sizeof(*sem));
    mv_waitqueue_init(&(sem->waitqueue));

    sem->flags = flags & MV_SEMAPTHORE_F_TYPE_MASK;

    if (sem->flags == MV_SEMAPTHORE_F_MTX)
        sem->count = 1;
    if (sem->flags == MV_SEMAPTHORE_F_BINARY)
        sem->count = (flags & MV_SEMAPTHORE_F_COUNT_MASK) ? 1 : 0;
    if (sem->flags == MV_SEMAPTHORE_F_COUNT)
        sem->count = flags & MV_SEMAPTHORE_F_COUNT_MASK;

    strncpy(sem->name, name, MV_SEM_NAME_LEN);
    sem->name[MV_SEM_NAME_LEN] = 0;

    MV_GLOBAL_UNLOCK();
    return k;
}

#define SEMAPHORE_BY_ID(semid) \
    MV_GLOBAL_LOCK(); \
    if (unlikely(semid == 0 || semid >= mv_num_semaphores)) \
    { \
ret_einval: \
        MV_GLOBAL_UNLOCK(); \
        return -MVKERNELEXT_EINVAL; \
    } \
    sem = mvSemaphores + semid; \
    if (unlikely(!sem->flags)) \
        goto ret_einval;

#define TASK_WILL_WAIT(tsk) \
    struct mv_task *p; \
    if (unlikely((p = gettask_cr(tsk)) == NULL)) \
    { \
        MV_GLOBAL_UNLOCK(); \
        return -MVKERNELEXT_ENOMEM; \
    }
#define CHECK_SEM() \
    if (unlikely(!sem->flags)) \
    { \
        MV_GLOBAL_UNLOCK(); \
        return -MVKERNELEXT_EDELETED; \
    }

/*******************************************************************************
* mvKernelExt_SemDelete
*
* DESCRIPTION:
*       Destroys semaphore
*
* INPUTS:
*       semid   - semaphore ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL if bad ID passed
*
* COMMENTS:
*
*******************************************************************************/
int mvKernelExt_SemDelete(int semid)
{
    mvSemaphoreSTC *sem;

    SEMAPHORE_BY_ID(semid);
    mv_waitqueue_wake_all(&(sem->waitqueue));
    mv_waitqueue_cleanup(&(sem->waitqueue));
    sem->flags = 0;

    MV_GLOBAL_UNLOCK();
    return 0;
}

/*******************************************************************************
* mvKernelExt_SemSignal
*
* DESCRIPTION:
*       Signals to semaphore
*
* INPUTS:
*       semid   - semaphore ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL - bad ID passed
*       -MVKERNELEXT_EPERM  - Mutex semaphore is locked by another task
*
* COMMENTS:
*
*******************************************************************************/
int mvKernelExt_SemSignal(int semid)
{
    mvSemaphoreSTC *sem;

    SEMAPHORE_BY_ID(semid);

    if (sem->flags & MV_SEMAPTHORE_F_MTX)
    {
        if (unlikely(sem->owner != current))
        {
            MV_GLOBAL_UNLOCK();
            return -MVKERNELEXT_EPERM;
        }
        sem->count++;
        if (sem->count > 0)
        {
            sem->owner = NULL;
            mv_waitqueue_wake_first(&(sem->waitqueue));
        }
    } else {
        if (sem->flags & MV_SEMAPTHORE_F_COUNT)
            sem->count++;
        else
            sem->count = 1; /* binary */
        mv_waitqueue_wake_first(&(sem->waitqueue));
    }

#ifdef MV_SEM_STAT
    sem->gcount++;
#endif
    MV_GLOBAL_UNLOCK();
    return 0;
}

/*******************************************************************************
* mvKernelExt_SemUnlockMutexes
*
* DESCRIPTION:
*       Unlock all mutexes locked by dead task
*
* INPUTS:
*       owner   - pointer to kernel's structure of dead task
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
static void mvKernelExt_SemUnlockMutexes(
        struct task_struct  *owner
)
{
    int k;
    for (k = 1; k < mv_num_semaphores; k++)
    {
        mvSemaphoreSTC *sem = mvSemaphores + k;

        if ((sem->flags & MV_SEMAPTHORE_F_MTX) != 0
                && sem->owner == owner)
        {
            sem->count = 1;
            sem->owner = NULL;
            mv_waitqueue_wake_first(&(sem->waitqueue));
        }
    }
}

/*******************************************************************************
* mvKernelExt_SemTryWait_common
*
* DESCRIPTION:
*       Try to acquire semaphore without waiting
*
* INPUTS:
*       sem   - pointer to semaphore structure
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EBUSY   - semaphore cannot be taken immediatelly
*
* COMMENTS:
*
*******************************************************************************/
static int mvKernelExt_SemTryWait_common(
        mvSemaphoreSTC *sem
)
{
    if (sem->flags & MV_SEMAPTHORE_F_MTX)
    {
        if (sem->count > 0)
        {
            sem->count--;
            sem->owner = current;
        } else {
            if (sem->owner == current)
                sem->count--;
            else
                return -MVKERNELEXT_EBUSY;
        }
    } else {
        if (sem->count <= 0)
            return -MVKERNELEXT_EBUSY;
        sem->count--;
    }
#ifdef MV_SEM_STAT
    sem->tcount++;
#endif
    return 0;
}

/*******************************************************************************
* mvKernelExt_SemTryWait
*
* DESCRIPTION:
*       Try to acquire semaphore without waiting
*
* INPUTS:
*       semid   - semaphore ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL  - bad ID passed
*       -MVKERNELEXT_EBUSY   - semaphore cannot be taken immediatelly
*
* COMMENTS:
*
*******************************************************************************/
int mvKernelExt_SemTryWait(int semid)
{
    mvSemaphoreSTC *sem;
    int ret;

    SEMAPHORE_BY_ID(semid);

    ret = mvKernelExt_SemTryWait_common(sem);

    MV_GLOBAL_UNLOCK();
    return ret;
}


/*******************************************************************************
* mvKernelExt_SemWait
*
* DESCRIPTION:
*       Wait for semaphore
*
* INPUTS:
*       semid   - semaphore ID
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL  - bad ID passed
*       -MVKERNELEXT_ENOMEM  - current task is not registered
*                              and task array is full
*       -MVKERNELEXT_EINTR   - wait interrupted
*
* COMMENTS:
*
*******************************************************************************/
int mvKernelExt_SemWait(int semid)
{
    mvSemaphoreSTC *sem;

    SEMAPHORE_BY_ID(semid);

    if (mvKernelExt_SemTryWait_common(sem) != 0)
    {
        TASK_WILL_WAIT(current);
#ifdef  MV_SEM_STAT
        sem->wcount++;
#endif
        if (sem->flags & MV_SEMAPTHORE_F_MTX)
        {
            do {
                if (unlikely(mv_do_short_wait_on_queue(&(sem->waitqueue), p, &(sem->owner))))
                {
                    MV_GLOBAL_UNLOCK();
                    return -MVKERNELEXT_EINTR;
                }
                CHECK_SEM();
            } while (sem->count <= 0);

            sem->owner = p->task;
        } else {
            do {
                if (unlikely(mv_do_wait_on_queue(&(sem->waitqueue), p)))
                {
                    MV_GLOBAL_UNLOCK();
                    return -MVKERNELEXT_EINTR;
                }
                CHECK_SEM();
            } while (sem->count == 0);
        }
        sem->count--;

#ifdef MV_SEM_STAT
        sem->tcount++;
#endif
    }

    MV_GLOBAL_UNLOCK();
    return 0;
}


/*******************************************************************************
* mvKernelExt_SemWaitTimeout
*
* DESCRIPTION:
*       Wait for semaphore
*
* INPUTS:
*       semid   - semaphore ID
*       timeout - timeout in milliseconds
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if successful
*       -MVKERNELEXT_EINVAL   - bad ID passed
*       -MVKERNELEXT_ENOMEM   - current task is not registered
*                               and task array is full
*       -MVKERNELEXT_EINTR    - wait interrupted
*       -MVKERNELEXT_ETIMEOUT - wait timeout
*
* COMMENTS:
*
*******************************************************************************/
int mvKernelExt_SemWaitTimeout(
        int semid,
        unsigned long timeout
)
{
    mvSemaphoreSTC *sem;

    SEMAPHORE_BY_ID(semid);

    if (mvKernelExt_SemTryWait_common(sem) != 0)
    {
        TASK_WILL_WAIT(current);
#ifdef  MV_SEM_STAT
        sem->wcount++;
#endif
#if HZ != 1000
        timeout += 1000 / HZ - 1;
        timeout /= 1000 / HZ;
#endif
        do {
            timeout = mv_do_wait_on_queue_timeout(&(sem->waitqueue), p, timeout);
            CHECK_SEM();
            if (!timeout)
            {
                MV_GLOBAL_UNLOCK();
                return -MVKERNELEXT_ETIMEOUT;
            }
            if (timeout == (unsigned long)(-1))
            {
                MV_GLOBAL_UNLOCK();
                return -MVKERNELEXT_EINTR;
            }
        } while (sem->count <= 0);

        if (sem->flags & MV_SEMAPTHORE_F_MTX)
            sem->owner = p->task;

        sem->count--;
#ifdef MV_SEM_STAT
        sem->tcount++;
#endif
    }

    MV_GLOBAL_UNLOCK();
    return 0;
}

EXPORT_SYMBOL(mvKernelExt_SemCreate);
EXPORT_SYMBOL(mvKernelExt_SemDelete);
EXPORT_SYMBOL(mvKernelExt_SemSignal);
EXPORT_SYMBOL(mvKernelExt_SemTryWait);
EXPORT_SYMBOL(mvKernelExt_SemWait);
EXPORT_SYMBOL(mvKernelExt_SemWaitTimeout);
