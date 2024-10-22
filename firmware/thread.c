/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: thread.c 31339 2011-12-17 07:27:24Z jethead71 $
 *
 * Copyright (C) 2002 by Ulf Ralberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include "thread.h"
#include "panic.h"
#include "system.h"
#include "kernel.h"
#include "cpu.h"
#include "string.h"
#ifdef RB_PROFILE
#include <profile.h>
#endif
#include "gcc_extensions.h"

/****************************************************************************
 *                              ATTENTION!!                                 *
 *    See notes below on implementing processor-specific portions!          *
 ***************************************************************************/

/* Define THREAD_EXTRA_CHECKS as 1 to enable additional state checks */
#ifdef DEBUG
#define THREAD_EXTRA_CHECKS 1 /* Always 1 for DEBUG */
#else
#define THREAD_EXTRA_CHECKS 0
#endif

/**
 * General locking order to guarantee progress. Order must be observed but
 * all stages are not nescessarily obligatory. Going from 1) to 3) is
 * perfectly legal.
 *
 * 1) IRQ
 * This is first because of the likelyhood of having an interrupt occur that
 * also accesses one of the objects farther down the list. Any non-blocking
 * synchronization done may already have a lock on something during normal
 * execution and if an interrupt handler running on the same processor as
 * the one that has the resource locked were to attempt to access the
 * resource, the interrupt handler would wait forever waiting for an unlock
 * that will never happen. There is no danger if the interrupt occurs on
 * a different processor because the one that has the lock will eventually
 * unlock and the other processor's handler may proceed at that time. Not
 * nescessary when the resource in question is definitely not available to
 * interrupt handlers.
 *  
 * 2) Kernel Object
 * 1) May be needed beforehand if the kernel object allows dual-use such as
 * event queues. The kernel object must have a scheme to protect itself from
 * access by another processor and is responsible for serializing the calls
 * to block_thread(_w_tmo) and wakeup_thread both to themselves and to each
 * other. Objects' queues are also protected here.
 * 
 * 3) Thread Slot
 * This locks access to the thread's slot such that its state cannot be
 * altered by another processor when a state change is in progress such as
 * when it is in the process of going on a blocked list. An attempt to wake
 * a thread while it is still blocking will likely desync its state with
 * the other resources used for that state.
 *
 * 4) Core Lists
 * These lists are specific to a particular processor core and are accessible
 * by all processor cores and interrupt handlers. The running (rtr) list is
 * the prime example where a thread may be added by any means.
 */

/*---------------------------------------------------------------------------
 * Processor specific: core_sleep/core_wake/misc. notes
 *
 * ARM notes:
 * FIQ is not dealt with by the scheduler code and is simply restored if it
 * must by masked for some reason - because threading modifies a register
 * that FIQ may also modify and there's no way to accomplish it atomically.
 * s3c2440 is such a case.
 *
 * Audio interrupts are generally treated at a higher priority than others
 * usage of scheduler code with interrupts higher than HIGHEST_IRQ_LEVEL
 * are not in general safe. Special cases may be constructed on a per-
 * source basis and blocking operations are not available.
 *
 * core_sleep procedure to implement for any CPU to ensure an asychronous
 * wakup never results in requiring a wait until the next tick (up to
 * 10000uS!). May require assembly and careful instruction ordering.
 *
 * 1) On multicore, stay awake if directed to do so by another. If so, goto
 *    step 4.
 * 2) If processor requires, atomically reenable interrupts and perform step
 *    3.
 * 3) Sleep the CPU core. If wakeup itself enables interrupts (stop #0x2000
 *    on Coldfire) goto step 5.
 * 4) Enable interrupts.
 * 5) Exit procedure.
 *
 * core_wake and multprocessor notes for sleep/wake coordination:
 * If possible, to wake up another processor, the forcing of an interrupt on
 * the woken core by the waker core is the easiest way to ensure a non-
 * delayed wake and immediate execution of any woken threads. If that isn't
 * available then some careful non-blocking synchonization is needed (as on
 * PP targets at the moment).
 *---------------------------------------------------------------------------
 */

/* Cast to the the machine pointer size, whose size could be < 4 or > 32
 * (someday :). */
#define DEADBEEF ((uintptr_t)0xdeadbeefdeadbeefull)
static struct core_entry cores[NUM_CORES] IBSS_ATTR;
struct thread_entry threads[MAXTHREADS] IBSS_ATTR;

static const char main_thread_name[] = "main";
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
extern uintptr_t stackbegin[];
extern uintptr_t stackend[];
#else
extern uintptr_t *stackbegin;
extern uintptr_t *stackend;
#endif

static inline void core_sleep(IF_COP_VOID(unsigned int core))
        __attribute__((always_inline));

void check_tmo_threads(void)
        __attribute__((noinline));

static inline void block_thread_on_l(struct thread_entry *thread, unsigned state)
        __attribute__((always_inline));

static void add_to_list_tmo(struct thread_entry *thread)
        __attribute__((noinline));

static void core_schedule_wakeup(struct thread_entry *thread)
        __attribute__((noinline));

#if NUM_CORES > 1
static inline void run_blocking_ops(
    unsigned int core, struct thread_entry *thread)
        __attribute__((always_inline));
#endif

static void thread_stkov(struct thread_entry *thread)
        __attribute__((noinline));

static inline void store_context(void* addr)
        __attribute__((always_inline));

static inline void load_context(const void* addr)
        __attribute__((always_inline));

#if NUM_CORES > 1
static void thread_final_exit_do(struct thread_entry *current)
    __attribute__((noinline)) NORETURN_ATTR USED_ATTR;
#else
static inline void thread_final_exit(struct thread_entry *current)
    __attribute__((always_inline)) NORETURN_ATTR;
#endif

void switch_thread(void)
        __attribute__((noinline));

/****************************************************************************
 * Processor/OS-specific section - include necessary core support
 */

#if defined(HAVE_WIN32_FIBER_THREADS)
#include "thread-win32.c"
#elif defined(HAVE_SIGALTSTACK_THREADS)
#include "thread-unix.c"
#elif defined(CPU_ARM)
#include "thread-arm.c"
#if defined (CPU_PP)
#include "thread-pp.c"
#endif /* CPU_PP */
#elif defined(CPU_COLDFIRE)
#include "thread-coldfire.c"
#elif CONFIG_CPU == SH7034
#include "thread-sh.c"
#elif defined(CPU_MIPS) && CPU_MIPS == 32
#include "thread-mips32.c"
#else
/* Wouldn't compile anyway */
#error Processor not implemented.
#endif /* CONFIG_CPU == */

#ifndef IF_NO_SKIP_YIELD
#define IF_NO_SKIP_YIELD(...)
#endif

/*
 * End Processor-specific section
 ***************************************************************************/

#if THREAD_EXTRA_CHECKS
static void thread_panicf(const char *msg, struct thread_entry *thread)
{
    IF_COP( const unsigned int core = thread->core; )
    static char name[32];
    thread_get_name(name, 32, thread);
    panicf ("%s %s" IF_COP(" (%d)"), msg, name IF_COP(, core));
}
static void thread_stkov(struct thread_entry *thread)
{
    thread_panicf("Stkov", thread);
}
#define THREAD_PANICF(msg, thread) \
    thread_panicf(msg, thread)
#define THREAD_ASSERT(exp, msg, thread) \
    ({ if (!({ exp; })) thread_panicf((msg), (thread)); })
#else
static void thread_stkov(struct thread_entry *thread)
{
    IF_COP( const unsigned int core = thread->core; )
    static char name[32];
    thread_get_name(name, 32, thread);
    panicf("Stkov %s" IF_COP(" (%d)"), name IF_COP(, core));
}
#define THREAD_PANICF(msg, thread)
#define THREAD_ASSERT(exp, msg, thread)
#endif /* THREAD_EXTRA_CHECKS */

/* Thread locking */
#if NUM_CORES > 1
#define LOCK_THREAD(thread) \
    ({ corelock_lock(&(thread)->slot_cl); })
#define TRY_LOCK_THREAD(thread) \
    ({ corelock_try_lock(&(thread)->slot_cl); })
#define UNLOCK_THREAD(thread) \
    ({ corelock_unlock(&(thread)->slot_cl); })
#define UNLOCK_THREAD_AT_TASK_SWITCH(thread) \
    ({ unsigned int _core = (thread)->core; \
       cores[_core].blk_ops.flags |= TBOP_UNLOCK_CORELOCK; \
       cores[_core].blk_ops.cl_p = &(thread)->slot_cl; })
#else
#define LOCK_THREAD(thread) \
    ({ })
#define TRY_LOCK_THREAD(thread) \
    ({ })
#define UNLOCK_THREAD(thread) \
    ({ })
#define UNLOCK_THREAD_AT_TASK_SWITCH(thread) \
    ({ })
#endif

/* RTR list */
#define RTR_LOCK(core) \
    ({ corelock_lock(&cores[core].rtr_cl); })
#define RTR_UNLOCK(core) \
    ({ corelock_unlock(&cores[core].rtr_cl); })

#ifdef HAVE_PRIORITY_SCHEDULING
#define rtr_add_entry(core, priority) \
    prio_add_entry(&cores[core].rtr, (priority))

#define rtr_subtract_entry(core, priority) \
    prio_subtract_entry(&cores[core].rtr, (priority))

#define rtr_move_entry(core, from, to) \
    prio_move_entry(&cores[core].rtr, (from), (to))
#else
#define rtr_add_entry(core, priority)
#define rtr_add_entry_inl(core, priority)
#define rtr_subtract_entry(core, priority)
#define rtr_subtract_entry_inl(core, priotity)
#define rtr_move_entry(core, from, to)
#define rtr_move_entry_inl(core, from, to)
#endif

/*---------------------------------------------------------------------------
 * Thread list structure - circular:
 *    +------------------------------+
 *    |                              |
 *    +--+---+<-+---+<-+---+<-+---+<-+
 * Head->| T |  | T |  | T |  | T |
 *    +->+---+->+---+->+---+->+---+--+
 *    |                              |
 *    +------------------------------+
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * Adds a thread to a list of threads using "insert last". Uses the "l"
 * links.
 *---------------------------------------------------------------------------
 */
static void add_to_list_l(struct thread_entry **list,
                          struct thread_entry *thread)
{
    struct thread_entry *l = *list;

    if (l == NULL)
    {
        /* Insert into unoccupied list */
        thread->l.prev = thread;
        thread->l.next = thread;
        *list = thread;
        return;
    }

    /* Insert last */
    thread->l.prev = l->l.prev;
    thread->l.next = l;
    l->l.prev->l.next = thread;
    l->l.prev = thread;
}

/*---------------------------------------------------------------------------
 * Removes a thread from a list of threads. Uses the "l" links.
 *---------------------------------------------------------------------------
 */
static void remove_from_list_l(struct thread_entry **list,
                               struct thread_entry *thread)
{
    struct thread_entry *prev, *next;

    next = thread->l.next;

    if (thread == next)
    {
        /* The only item */
        *list = NULL;
        return;
    }

    if (thread == *list)
    {
        /* List becomes next item */
        *list = next;
    }

    prev = thread->l.prev;
    
    /* Fix links to jump over the removed entry. */
    next->l.prev = prev;
    prev->l.next = next;
}

/*---------------------------------------------------------------------------
 * Timeout list structure - circular reverse (to make "remove item" O(1)),
 * NULL-terminated forward (to ease the far more common forward traversal):
 *    +------------------------------+
 *    |                              |
 *    +--+---+<-+---+<-+---+<-+---+<-+
 * Head->| T |  | T |  | T |  | T |
 *       +---+->+---+->+---+->+---+-X
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * Add a thread from the core's timout list by linking the pointers in its
 * tmo structure.
 *---------------------------------------------------------------------------
 */
static void add_to_list_tmo(struct thread_entry *thread)
{
    struct thread_entry *tmo = cores[IF_COP_CORE(thread->core)].timeout;
    THREAD_ASSERT(thread->tmo.prev == NULL,
                  "add_to_list_tmo->already listed", thread);

    thread->tmo.next = NULL;

    if (tmo == NULL)
    {
        /* Insert into unoccupied list */
        thread->tmo.prev = thread;
        cores[IF_COP_CORE(thread->core)].timeout = thread;
        return;
    }

    /* Insert Last */
    thread->tmo.prev = tmo->tmo.prev;
    tmo->tmo.prev->tmo.next = thread;
    tmo->tmo.prev = thread;
}

/*---------------------------------------------------------------------------
 * Remove a thread from the core's timout list by unlinking the pointers in
 * its tmo structure. Sets thread->tmo.prev to NULL to indicate the timeout
 * is cancelled.
 *---------------------------------------------------------------------------
 */
static void remove_from_list_tmo(struct thread_entry *thread)
{
    struct thread_entry **list = &cores[IF_COP_CORE(thread->core)].timeout;
    struct thread_entry *prev = thread->tmo.prev;
    struct thread_entry *next = thread->tmo.next;

    THREAD_ASSERT(prev != NULL, "remove_from_list_tmo->not listed", thread);

    if (next != NULL)
        next->tmo.prev = prev;

    if (thread == *list)
    {
        /* List becomes next item and empty if next == NULL */
        *list = next;
        /* Mark as unlisted */
        thread->tmo.prev = NULL;
    }
    else
    {
        if (next == NULL)
            (*list)->tmo.prev = prev;
        prev->tmo.next = next;
        /* Mark as unlisted */
        thread->tmo.prev = NULL;
    }
}


#ifdef HAVE_PRIORITY_SCHEDULING
/*---------------------------------------------------------------------------
 * Priority distribution structure (one category for each possible priority):
 *
 *       +----+----+----+ ... +-----+
 * hist: | F0 | F1 | F2 |     | F31 |
 *       +----+----+----+ ... +-----+
 * mask: | b0 | b1 | b2 |     | b31 |
 *       +----+----+----+ ... +-----+
 *
 * F = count of threads at priority category n (frequency)
 * b = bitmask of non-zero priority categories (occupancy)
 *
 *        / if H[n] != 0 : 1
 * b[n] = |
 *        \ else         : 0 
 *
 *---------------------------------------------------------------------------
 * Basic priority inheritance priotocol (PIP):
 *
 * Mn = mutex n, Tn = thread n
 *
 * A lower priority thread inherits the priority of the highest priority
 * thread blocked waiting for it to complete an action (such as release a
 * mutex or respond to a message via queue_send):
 *
 * 1) T2->M1->T1
 *
 * T1 owns M1, T2 is waiting for M1 to realease M1. If T2 has a higher
 * priority than T1 then T1 inherits the priority of T2.
 *
 * 2) T3
 *    \/
 *    T2->M1->T1
 *
 * Situation is like 1) but T2 and T3 are both queued waiting for M1 and so
 * T1 inherits the higher of T2 and T3.
 *
 * 3) T3->M2->T2->M1->T1
 *
 * T1 owns M1, T2 owns M2. If T3 has a higher priority than both T1 and T2,
 * then T1 inherits the priority of T3 through T2.
 *
 * Blocking chains can grow arbitrarily complex (though it's best that they
 * not form at all very often :) and build-up from these units.
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * Increment frequency at category "priority"
 *---------------------------------------------------------------------------
 */
static inline unsigned int prio_add_entry(
    struct priority_distribution *pd, int priority)
{
    unsigned int count;
    /* Enough size/instruction count difference for ARM makes it worth it to
     * use different code (192 bytes for ARM). Only thing better is ASM. */
#ifdef CPU_ARM
    count = pd->hist[priority];
    if (++count == 1)
        pd->mask |= 1 << priority;
    pd->hist[priority] = count;
#else /* This one's better for Coldfire */
    if ((count = ++pd->hist[priority]) == 1)
        pd->mask |= 1 << priority;
#endif

    return count;
}

/*---------------------------------------------------------------------------
 * Decrement frequency at category "priority"
 *---------------------------------------------------------------------------
 */
static inline unsigned int prio_subtract_entry(
    struct priority_distribution *pd, int priority)
{
    unsigned int count;

#ifdef CPU_ARM
    count = pd->hist[priority];
    if (--count == 0)
        pd->mask &= ~(1 << priority);
    pd->hist[priority] = count;
#else
    if ((count = --pd->hist[priority]) == 0)
        pd->mask &= ~(1 << priority);
#endif

    return count;
}

/*---------------------------------------------------------------------------
 * Remove from one category and add to another
 *---------------------------------------------------------------------------
 */
static inline void prio_move_entry(
    struct priority_distribution *pd, int from, int to)
{
    uint32_t mask = pd->mask;

#ifdef CPU_ARM
    unsigned int count;

    count = pd->hist[from];
    if (--count == 0)
        mask &= ~(1 << from);
    pd->hist[from] = count;

    count = pd->hist[to];
    if (++count == 1)
        mask |= 1 << to;
    pd->hist[to] = count;
#else
    if (--pd->hist[from] == 0)
        mask &= ~(1 << from);

    if (++pd->hist[to] == 1)
        mask |= 1 << to;
#endif

    pd->mask = mask;
}

/*---------------------------------------------------------------------------
 * Change the priority and rtr entry for a running thread
 *---------------------------------------------------------------------------
 */
static inline void set_running_thread_priority(
    struct thread_entry *thread, int priority)
{
    const unsigned int core = IF_COP_CORE(thread->core);
    RTR_LOCK(core);
    rtr_move_entry(core, thread->priority, priority);
    thread->priority = priority;
    RTR_UNLOCK(core);
}

/*---------------------------------------------------------------------------
 * Finds the highest priority thread in a list of threads. If the list is
 * empty, the PRIORITY_IDLE is returned.
 *
 * It is possible to use the struct priority_distribution within an object
 * instead of scanning the remaining threads in the list but as a compromise,
 * the resulting per-object memory overhead is saved at a slight speed
 * penalty under high contention.
 *---------------------------------------------------------------------------
 */
static int find_highest_priority_in_list_l(
    struct thread_entry * const thread)
{
    if (LIKELY(thread != NULL))
    {
        /* Go though list until the ending up at the initial thread */
        int highest_priority = thread->priority;
        struct thread_entry *curr = thread;

        do
        {
            int priority = curr->priority;

            if (priority < highest_priority)
                highest_priority = priority;

            curr = curr->l.next;
        }
        while (curr != thread);

        return highest_priority;
    }

    return PRIORITY_IDLE;
}

/*---------------------------------------------------------------------------
 * Register priority with blocking system and bubble it down the chain if
 * any until we reach the end or something is already equal or higher.
 *
 * NOTE: A simultaneous circular wait could spin deadlock on multiprocessor
 * targets but that same action also guarantees a circular block anyway and
 * those are prevented, right? :-)
 *---------------------------------------------------------------------------
 */
static struct thread_entry *
    blocker_inherit_priority(struct thread_entry *current)
{
    const int priority = current->priority;
    struct blocker *bl = current->blocker;
    struct thread_entry * const tstart = current;
    struct thread_entry *bl_t = bl->thread;

    /* Blocker cannot change since the object protection is held */
    LOCK_THREAD(bl_t);

    for (;;)
    {
        struct thread_entry *next;
        int bl_pr = bl->priority;

        if (priority >= bl_pr)
            break; /* Object priority already high enough */

        bl->priority = priority;

        /* Add this one */
        prio_add_entry(&bl_t->pdist, priority);

        if (bl_pr < PRIORITY_IDLE)
        {
            /* Not first waiter - subtract old one */
            prio_subtract_entry(&bl_t->pdist, bl_pr);
        }

        if (priority >= bl_t->priority)
            break; /* Thread priority high enough */

        if (bl_t->state == STATE_RUNNING)
        {
            /* Blocking thread is a running thread therefore there are no
             * further blockers. Change the "run queue" on which it
             * resides. */
            set_running_thread_priority(bl_t, priority);
            break;
        }

        bl_t->priority = priority;

        /* If blocking thread has a blocker, apply transitive inheritance */
        bl = bl_t->blocker;

        if (bl == NULL)
            break; /* End of chain or object doesn't support inheritance */

        next = bl->thread;

        if (UNLIKELY(next == tstart))
            break; /* Full-circle - deadlock! */

        UNLOCK_THREAD(current);

#if NUM_CORES > 1
        for (;;)
        {
            LOCK_THREAD(next);

            /* Blocker could change - retest condition */
            if (LIKELY(bl->thread == next))
                break;

            UNLOCK_THREAD(next);
            next = bl->thread;
        }
#endif
        current = bl_t;
        bl_t = next;
    }

    UNLOCK_THREAD(bl_t);

    return current;
}

/*---------------------------------------------------------------------------
 * Readjust priorities when waking a thread blocked waiting for another
 * in essence "releasing" the thread's effect on the object owner. Can be
 * performed from any context.
 *---------------------------------------------------------------------------
 */
struct thread_entry *
    wakeup_priority_protocol_release(struct thread_entry *thread)
{
    const int priority = thread->priority;
    struct blocker *bl = thread->blocker;
    struct thread_entry * const tstart = thread;
    struct thread_entry *bl_t = bl->thread;

    /* Blocker cannot change since object will be locked */
    LOCK_THREAD(bl_t);

    thread->blocker = NULL; /* Thread not blocked */

    for (;;)
    {
        struct thread_entry *next;
        int bl_pr = bl->priority;

        if (priority > bl_pr)
            break; /* Object priority higher */

        next = *thread->bqp;

        if (next == NULL)
        {
            /* No more threads in queue */
            prio_subtract_entry(&bl_t->pdist, bl_pr);
            bl->priority = PRIORITY_IDLE;
        }
        else
        {
            /* Check list for highest remaining priority */
            int queue_pr = find_highest_priority_in_list_l(next);

            if (queue_pr == bl_pr)
                break; /* Object priority not changing */

            /* Change queue priority */
            prio_move_entry(&bl_t->pdist, bl_pr, queue_pr);
            bl->priority = queue_pr;
        }

        if (bl_pr > bl_t->priority)
            break; /* thread priority is higher */

        bl_pr = find_first_set_bit(bl_t->pdist.mask);

        if (bl_pr == bl_t->priority)
            break; /* Thread priority not changing */

        if (bl_t->state == STATE_RUNNING)
        {
            /* No further blockers */
            set_running_thread_priority(bl_t, bl_pr);
            break;
        }

        bl_t->priority = bl_pr;

        /* If blocking thread has a blocker, apply transitive inheritance */
        bl = bl_t->blocker;

        if (bl == NULL)
            break; /* End of chain or object doesn't support inheritance */

        next = bl->thread;

        if (UNLIKELY(next == tstart))
            break; /* Full-circle - deadlock! */

        UNLOCK_THREAD(thread);

#if NUM_CORES > 1
        for (;;)
        {
            LOCK_THREAD(next);

            /* Blocker could change - retest condition */
            if (LIKELY(bl->thread == next))
                break;

            UNLOCK_THREAD(next);
            next = bl->thread;
        }
#endif
        thread = bl_t;
        bl_t = next;
    }

    UNLOCK_THREAD(bl_t);

#if NUM_CORES > 1
    if (UNLIKELY(thread != tstart))
    {
        /* Relock original if it changed */
        LOCK_THREAD(tstart);
    }
#endif

    return cores[CURRENT_CORE].running;
}

/*---------------------------------------------------------------------------
 * Transfer ownership to a thread waiting for an objects and transfer
 * inherited priority boost from other waiters. This algorithm knows that
 * blocking chains may only unblock from the very end.
 *
 * Only the owning thread itself may call this and so the assumption that
 * it is the running thread is made.
 *---------------------------------------------------------------------------
 */
struct thread_entry *
    wakeup_priority_protocol_transfer(struct thread_entry *thread)
{
    /* Waking thread inherits priority boost from object owner */
    struct blocker *bl = thread->blocker;
    struct thread_entry *bl_t = bl->thread;
    struct thread_entry *next;
    int bl_pr;

    THREAD_ASSERT(cores[CURRENT_CORE].running == bl_t,
                  "UPPT->wrong thread", cores[CURRENT_CORE].running);

    LOCK_THREAD(bl_t);

    bl_pr = bl->priority;

    /* Remove the object's boost from the owning thread */
    if (prio_subtract_entry(&bl_t->pdist, bl_pr) == 0 &&
        bl_pr <= bl_t->priority)
    {
        /* No more threads at this priority are waiting and the old level is
         * at least the thread level */
        int priority = find_first_set_bit(bl_t->pdist.mask);

        if (priority != bl_t->priority)
        {
            /* Adjust this thread's priority */
            set_running_thread_priority(bl_t, priority);
        }
    }

    next = *thread->bqp;

    if (LIKELY(next == NULL))
    {
        /* Expected shortcut - no more waiters */
        bl_pr = PRIORITY_IDLE;
    }
    else
    {
        if (thread->priority <= bl_pr)
        {
            /* Need to scan threads remaining in queue */
            bl_pr = find_highest_priority_in_list_l(next);
        }

        if (prio_add_entry(&thread->pdist, bl_pr) == 1 &&
            bl_pr < thread->priority)
        {
            /* Thread priority must be raised */
            thread->priority = bl_pr;
        }
    }

    bl->thread = thread;    /* This thread pwns */
    bl->priority = bl_pr;   /* Save highest blocked priority */
    thread->blocker = NULL; /* Thread not blocked */

    UNLOCK_THREAD(bl_t);

    return bl_t;
}

/*---------------------------------------------------------------------------
 * No threads must be blocked waiting for this thread except for it to exit.
 * The alternative is more elaborate cleanup and object registration code.
 * Check this for risk of silent data corruption when objects with
 * inheritable blocking are abandoned by the owner - not precise but may
 * catch something.
 *---------------------------------------------------------------------------
 */
static void __attribute__((noinline)) check_for_obj_waiters(
    const char *function, struct thread_entry *thread)
{
    /* Only one bit in the mask should be set with a frequency on 1 which
     * represents the thread's own base priority */
    uint32_t mask = thread->pdist.mask;
    if ((mask & (mask - 1)) != 0 ||
        thread->pdist.hist[find_first_set_bit(mask)] > 1)
    {
        unsigned char name[32];
        thread_get_name(name, 32, thread);
        panicf("%s->%s with obj. waiters", function, name);
    }
}
#endif /* HAVE_PRIORITY_SCHEDULING */

/*---------------------------------------------------------------------------
 * Move a thread back to a running state on its core.
 *---------------------------------------------------------------------------
 */
static void core_schedule_wakeup(struct thread_entry *thread)
{
    const unsigned int core = IF_COP_CORE(thread->core);

    RTR_LOCK(core);

    thread->state = STATE_RUNNING;

    add_to_list_l(&cores[core].running, thread);
    rtr_add_entry(core, thread->priority);

    RTR_UNLOCK(core);

#if NUM_CORES > 1
    if (core != CURRENT_CORE)
        core_wake(core);
#endif
}

/*---------------------------------------------------------------------------
 * Check the core's timeout list when at least one thread is due to wake.
 * Filtering for the condition is done before making the call. Resets the
 * tick when the next check will occur.
 *---------------------------------------------------------------------------
 */
void check_tmo_threads(void)
{
    const unsigned int core = CURRENT_CORE;
    const long tick = current_tick; /* snapshot the current tick */
    long next_tmo_check = tick + 60*HZ; /* minimum duration: once/minute */
    struct thread_entry *next = cores[core].timeout;

    /* If there are no processes waiting for a timeout, just keep the check
       tick from falling into the past. */

    /* Break the loop once we have walked through the list of all
     * sleeping processes or have removed them all. */
    while (next != NULL)
    {
        /* Check sleeping threads. Allow interrupts between checks. */
        enable_irq();

        struct thread_entry *curr = next;

        next = curr->tmo.next;

        /* Lock thread slot against explicit wakeup */
        disable_irq();
        LOCK_THREAD(curr);

        unsigned state = curr->state;

        if (state < TIMEOUT_STATE_FIRST)
        {
            /* Cleanup threads no longer on a timeout but still on the
             * list. */
            remove_from_list_tmo(curr);
        }
        else if (LIKELY(TIME_BEFORE(tick, curr->tmo_tick)))
        {
            /* Timeout still pending - this will be the usual case */
            if (TIME_BEFORE(curr->tmo_tick, next_tmo_check))
            {
                /* Earliest timeout found so far - move the next check up
                   to its time */
                next_tmo_check = curr->tmo_tick;
            }
        }
        else
        {
            /* Sleep timeout has been reached so bring the thread back to
             * life again. */
            if (state == STATE_BLOCKED_W_TMO)
            {
#ifdef HAVE_CORELOCK_OBJECT
                /* Lock the waiting thread's kernel object */
                struct corelock *ocl = curr->obj_cl;

                if (UNLIKELY(corelock_try_lock(ocl) == 0))
                {
                    /* Need to retry in the correct order though the need is
                     * unlikely */
                    UNLOCK_THREAD(curr);
                    corelock_lock(ocl);
                    LOCK_THREAD(curr);

                    if (UNLIKELY(curr->state != STATE_BLOCKED_W_TMO))
                    {
                        /* Thread was woken or removed explicitely while slot
                         * was unlocked */
                        corelock_unlock(ocl);
                        remove_from_list_tmo(curr);
                        UNLOCK_THREAD(curr);
                        continue;
                    }
                }
#endif /* NUM_CORES */

                remove_from_list_l(curr->bqp, curr);

#ifdef HAVE_WAKEUP_EXT_CB
                if (curr->wakeup_ext_cb != NULL)
                    curr->wakeup_ext_cb(curr);
#endif

#ifdef HAVE_PRIORITY_SCHEDULING
                if (curr->blocker != NULL)
                    wakeup_priority_protocol_release(curr);
#endif
                corelock_unlock(ocl);
            }
            /* else state == STATE_SLEEPING */

            remove_from_list_tmo(curr);

            RTR_LOCK(core);

            curr->state = STATE_RUNNING;

            add_to_list_l(&cores[core].running, curr);
            rtr_add_entry(core, curr->priority);

            RTR_UNLOCK(core);
        }

        UNLOCK_THREAD(curr);
    }

    cores[core].next_tmo_check = next_tmo_check;
}

/*---------------------------------------------------------------------------
 * Performs operations that must be done before blocking a thread but after
 * the state is saved.
 *---------------------------------------------------------------------------
 */
#if NUM_CORES > 1
static inline void run_blocking_ops(
    unsigned int core, struct thread_entry *thread)
{
    struct thread_blk_ops *ops = &cores[core].blk_ops;
    const unsigned flags = ops->flags;

    if (LIKELY(flags == TBOP_CLEAR))
        return;

    switch (flags)
    {
    case TBOP_SWITCH_CORE:
        core_switch_blk_op(core, thread);
        /* Fall-through */
    case TBOP_UNLOCK_CORELOCK:
        corelock_unlock(ops->cl_p);
        break;
    }

    ops->flags = TBOP_CLEAR;
}
#endif /* NUM_CORES > 1 */

#ifdef RB_PROFILE
void profile_thread(void)
{
    profstart(cores[CURRENT_CORE].running - threads);
}
#endif

/*---------------------------------------------------------------------------
 * Prepares a thread to block on an object's list and/or for a specified
 * duration - expects object and slot to be appropriately locked if needed
 * and interrupts to be masked.
 *---------------------------------------------------------------------------
 */
static inline void block_thread_on_l(struct thread_entry *thread,
                                     unsigned state)
{
    /* If inlined, unreachable branches will be pruned with no size penalty
       because state is passed as a constant parameter. */
    const unsigned int core = IF_COP_CORE(thread->core);

    /* Remove the thread from the list of running threads. */
    RTR_LOCK(core);
    remove_from_list_l(&cores[core].running, thread);
    rtr_subtract_entry(core, thread->priority);
    RTR_UNLOCK(core);

    /* Add a timeout to the block if not infinite */
    switch (state)
    {
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        /* Put the thread into a new list of inactive threads. */
        add_to_list_l(thread->bqp, thread);

        if (state == STATE_BLOCKED)
            break;

        /* Fall-through */
    case STATE_SLEEPING:
        /* If this thread times out sooner than any other thread, update
           next_tmo_check to its timeout */
        if (TIME_BEFORE(thread->tmo_tick, cores[core].next_tmo_check))
        {
            cores[core].next_tmo_check = thread->tmo_tick;
        }

        if (thread->tmo.prev == NULL)
        {
            add_to_list_tmo(thread);
        }
        /* else thread was never removed from list - just keep it there */
        break;
    }

    /* Remember the the next thread about to block. */
    cores[core].block_task = thread;

    /* Report new state. */
    thread->state = state;
}

/*---------------------------------------------------------------------------
 * Switch thread in round robin fashion for any given priority. Any thread
 * that removed itself from the running list first must specify itself in
 * the paramter.
 *
 * INTERNAL: Intended for use by kernel and not for programs.
 *---------------------------------------------------------------------------
 */
void switch_thread(void)
{

    const unsigned int core = CURRENT_CORE;
    struct thread_entry *block = cores[core].block_task;
    struct thread_entry *thread = cores[core].running;

    /* Get context to save - next thread to run is unknown until all wakeups
     * are evaluated */
    if (block != NULL)
    {
        cores[core].block_task = NULL;

#if NUM_CORES > 1
        if (UNLIKELY(thread == block))
        {
            /* This was the last thread running and another core woke us before
             * reaching here. Force next thread selection to give tmo threads or
             * other threads woken before this block a first chance. */
            block = NULL;
        }
        else
#endif
        {
            /* Blocking task is the old one */
            thread = block;
        }
    }

#ifdef RB_PROFILE
#ifdef CPU_COLDFIRE
    _profile_thread_stopped(thread->id & THREAD_ID_SLOT_MASK);
#else
    profile_thread_stopped(thread->id & THREAD_ID_SLOT_MASK);
#endif
#endif

    /* Begin task switching by saving our current context so that we can
     * restore the state of the current thread later to the point prior
     * to this call. */
    store_context(&thread->context);

    /* Check if the current thread stack is overflown */
    if (UNLIKELY(thread->stack[0] != DEADBEEF) && thread->stack_size > 0)
        thread_stkov(thread);

#if NUM_CORES > 1
    /* Run any blocking operations requested before switching/sleeping */
    run_blocking_ops(core, thread);
#endif

#ifdef HAVE_PRIORITY_SCHEDULING
    IF_NO_SKIP_YIELD( if (thread->skip_count != -1) )
    /* Reset the value of thread's skip count */
        thread->skip_count = 0;
#endif

    for (;;)
    {
        /* If there are threads on a timeout and the earliest wakeup is due,
         * check the list and wake any threads that need to start running
         * again. */
        if (!TIME_BEFORE(current_tick, cores[core].next_tmo_check))
        {
            check_tmo_threads();
        }

        disable_irq();
        RTR_LOCK(core);

        thread = cores[core].running;

        if (UNLIKELY(thread == NULL))
        {
            /* Enter sleep mode to reduce power usage - woken up on interrupt
             * or wakeup request from another core - expected to enable
             * interrupts. */
            RTR_UNLOCK(core);
            core_sleep(IF_COP(core));
        }
        else
        {
#ifdef HAVE_PRIORITY_SCHEDULING
            /* Select the new task based on priorities and the last time a
             * process got CPU time relative to the highest priority runnable
             * task. */
            struct priority_distribution *pd = &cores[core].rtr;
            int max = find_first_set_bit(pd->mask);

            if (block == NULL)
            {
                /* Not switching on a block, tentatively select next thread */
                thread = thread->l.next;
            }

            for (;;)
            {
                int priority = thread->priority;
                int diff;

                /* This ridiculously simple method of aging seems to work
                 * suspiciously well. It does tend to reward CPU hogs (under
                 * yielding) but that's generally not desirable at all. On
                 * the plus side, it, relatively to other threads, penalizes
                 * excess yielding which is good if some high priority thread
                 * is performing no useful work such as polling for a device
                 * to be ready. Of course, aging is only employed when higher
                 * and lower priority threads are runnable. The highest
                 * priority runnable thread(s) are never skipped unless a
                 * lower-priority process has aged sufficiently. Priorities
                 * of REALTIME class are run strictly according to priority
                 * thus are not subject to switchout due to lower-priority
                 * processes aging; they must give up the processor by going
                 * off the run list. */
                if (LIKELY(priority <= max) ||
                    IF_NO_SKIP_YIELD( thread->skip_count == -1 || )
                    (priority > PRIORITY_REALTIME &&
                     (diff = priority - max,
                         ++thread->skip_count > diff*diff)))
                {
                    cores[core].running = thread;
                    break;
                }

                thread = thread->l.next;
            }
#else
            /* Without priority use a simple FCFS algorithm */
            if (block == NULL)
            {
                /* Not switching on a block, select next thread */
                thread = thread->l.next;
                cores[core].running = thread;
            }
#endif /* HAVE_PRIORITY_SCHEDULING */

            RTR_UNLOCK(core);
            enable_irq();
            break;
        }
    }

    /* And finally give control to the next thread. */
    load_context(&thread->context);

#ifdef RB_PROFILE
    profile_thread_started(thread->id & THREAD_ID_SLOT_MASK);
#endif

}

/*---------------------------------------------------------------------------
 * Sleeps a thread for at least a specified number of ticks with zero being
 * a wait until the next tick.
 *
 * INTERNAL: Intended for use by kernel and not for programs.
 *---------------------------------------------------------------------------
 */
void sleep_thread(int ticks)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;

    LOCK_THREAD(current);

    /* Set our timeout, remove from run list and join timeout list. */
    current->tmo_tick = current_tick + ticks + 1;
    block_thread_on_l(current, STATE_SLEEPING);

    UNLOCK_THREAD(current);
}

/*---------------------------------------------------------------------------
 * Indefinitely block a thread on a blocking queue for explicit wakeup.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
void block_thread(struct thread_entry *current)
{
    /* Set the state to blocked and take us off of the run queue until we
     * are explicitly woken */
    LOCK_THREAD(current);

    /* Set the list for explicit wakeup */
    block_thread_on_l(current, STATE_BLOCKED);

#ifdef HAVE_PRIORITY_SCHEDULING
    if (current->blocker != NULL)
    {
        /* Object supports PIP */
        current = blocker_inherit_priority(current);
    }
#endif

    UNLOCK_THREAD(current);
}

/*---------------------------------------------------------------------------
 * Block a thread on a blocking queue for a specified time interval or until
 * explicitly woken - whichever happens first.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
void block_thread_w_tmo(struct thread_entry *current, int timeout)
{
    /* Get the entry for the current running thread. */
    LOCK_THREAD(current);

    /* Set the state to blocked with the specified timeout */
    current->tmo_tick = current_tick + timeout;

    /* Set the list for explicit wakeup */
    block_thread_on_l(current, STATE_BLOCKED_W_TMO);

#ifdef HAVE_PRIORITY_SCHEDULING
    if (current->blocker != NULL)
    {
        /* Object supports PIP */
        current = blocker_inherit_priority(current);
    }
#endif

    UNLOCK_THREAD(current);
}

/*---------------------------------------------------------------------------
 * Explicitly wakeup a thread on a blocking queue. Only effects threads of
 * STATE_BLOCKED and STATE_BLOCKED_W_TMO.
 *
 * This code should be considered a critical section by the caller meaning
 * that the object's corelock should be held.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
unsigned int wakeup_thread(struct thread_entry **list)
{
    struct thread_entry *thread = *list;
    unsigned int result = THREAD_NONE;

    /* Check if there is a blocked thread at all. */
    if (thread == NULL)
        return result;

    LOCK_THREAD(thread);

    /* Determine thread's current state. */
    switch (thread->state)
    {
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        remove_from_list_l(list, thread);

        result = THREAD_OK;

#ifdef HAVE_PRIORITY_SCHEDULING
        struct thread_entry *current;
        struct blocker *bl = thread->blocker;

        if (bl == NULL)
        {
            /* No inheritance - just boost the thread by aging */
            IF_NO_SKIP_YIELD( if (thread->skip_count != -1) )
                thread->skip_count = thread->priority;
            current = cores[CURRENT_CORE].running;
        }
        else
        {
            /* Call the specified unblocking PIP */
            current = bl->wakeup_protocol(thread);
        }

        if (current != NULL &&
            find_first_set_bit(cores[IF_COP_CORE(current->core)].rtr.mask)
                < current->priority)
        {
            /* There is a thread ready to run of higher or same priority on
             * the same core as the current one; recommend a task switch.
             * Knowing if this is an interrupt call would be helpful here. */
            result |= THREAD_SWITCH;
        }
#endif /* HAVE_PRIORITY_SCHEDULING */

        core_schedule_wakeup(thread);
        break;

    /* Nothing to do. State is not blocked. */
#if THREAD_EXTRA_CHECKS
    default:
        THREAD_PANICF("wakeup_thread->block invalid", thread);
    case STATE_RUNNING:
    case STATE_KILLED:
        break;
#endif
    }

    UNLOCK_THREAD(thread);
    return result;
}

/*---------------------------------------------------------------------------
 * Wakeup an entire queue of threads - returns bitwise-or of return bitmask
 * from each operation or THREAD_NONE of nothing was awakened. Object owning
 * the queue must be locked first.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
unsigned int thread_queue_wake(struct thread_entry **list)
{
    unsigned result = THREAD_NONE;

    for (;;)
    {
        unsigned int rc = wakeup_thread(list);

        if (rc == THREAD_NONE)
            break; /* No more threads */

        result |= rc;
    }

    return result;
}

/*---------------------------------------------------------------------------
 * Assign the thread slot a new ID. Version is 1-255.
 *---------------------------------------------------------------------------
 */
static void new_thread_id(unsigned int slot_num,
                          struct thread_entry *thread)
{
    unsigned int version =
        (thread->id + (1u << THREAD_ID_VERSION_SHIFT))
                & THREAD_ID_VERSION_MASK;

    /* If wrapped to 0, make it 1 */
    if (version == 0)
        version = 1u << THREAD_ID_VERSION_SHIFT;

    thread->id = version | (slot_num & THREAD_ID_SLOT_MASK);
}

/*---------------------------------------------------------------------------
 * Find an empty thread slot or MAXTHREADS if none found. The slot returned
 * will be locked on multicore.
 *---------------------------------------------------------------------------
 */
static struct thread_entry * find_empty_thread_slot(void)
{
    /* Any slot could be on an interrupt-accessible list */
    IF_COP( int oldlevel = disable_irq_save(); )
    struct thread_entry *thread = NULL;
    int n;

    for (n = 0; n < MAXTHREADS; n++)
    {
        /* Obtain current slot state - lock it on multicore */
        struct thread_entry *t = &threads[n];
        LOCK_THREAD(t);

        if (t->state == STATE_KILLED IF_COP( && t->name != THREAD_DESTRUCT ))
        {
            /* Slot is empty - leave it locked and caller will unlock */
            thread = t;
            break;
        }

        /* Finished examining slot - no longer busy - unlock on multicore */
        UNLOCK_THREAD(t);
    }

    IF_COP( restore_irq(oldlevel); ) /* Reenable interrups - this slot is
                                          not accesible to them yet */
    return thread;
}

/*---------------------------------------------------------------------------
 * Return the thread_entry pointer for a thread_id. Return the current
 * thread if the ID is (unsigned int)-1 (alias for current).
 *---------------------------------------------------------------------------
 */
struct thread_entry * thread_id_entry(unsigned int thread_id)
{
    return &threads[thread_id & THREAD_ID_SLOT_MASK];
}

/*---------------------------------------------------------------------------
 * Return the thread id of the calling thread
 * --------------------------------------------------------------------------
 */
unsigned int thread_self(void)
{
    return cores[CURRENT_CORE].running->id;
}

/*---------------------------------------------------------------------------
 * Return the thread entry of the calling thread.
 *
 * INTERNAL: Intended for use by kernel and not for programs.
 *---------------------------------------------------------------------------
 */
struct thread_entry* thread_self_entry(void)
{
    return cores[CURRENT_CORE].running;
}

/*---------------------------------------------------------------------------
 * Place the current core in idle mode - woken up on interrupt or wake
 * request from another core.
 *---------------------------------------------------------------------------
 */
void core_idle(void)
{
    IF_COP( const unsigned int core = CURRENT_CORE; )
    disable_irq();
    core_sleep(IF_COP(core));
}

/*---------------------------------------------------------------------------
 * Create a thread. If using a dual core architecture, specify which core to
 * start the thread on.
 *
 * Return ID if context area could be allocated, else NULL.
 *---------------------------------------------------------------------------
 */
unsigned int create_thread(void (*function)(void),
                           void* stack, size_t stack_size,
                           unsigned flags, const char *name
                           IF_PRIO(, int priority)
                           IF_COP(, unsigned int core))
{
    unsigned int i;
    unsigned int stack_words;
    uintptr_t stackptr, stackend;
    struct thread_entry *thread;
    unsigned state;
    int oldlevel;

    thread = find_empty_thread_slot();
    if (thread == NULL)
    {
        return 0;
    }

    oldlevel = disable_irq_save();

    /* Munge the stack to make it easy to spot stack overflows */
    stackptr = ALIGN_UP((uintptr_t)stack, sizeof (uintptr_t));
    stackend = ALIGN_DOWN((uintptr_t)stack + stack_size, sizeof (uintptr_t));
    stack_size = stackend - stackptr;
    stack_words = stack_size / sizeof (uintptr_t);

    for (i = 0; i < stack_words; i++)
    {
        ((uintptr_t *)stackptr)[i] = DEADBEEF;
    }

    /* Store interesting information */
    thread->name = name;
    thread->stack = (uintptr_t *)stackptr;
    thread->stack_size = stack_size;
    thread->queue = NULL;
#ifdef HAVE_WAKEUP_EXT_CB
    thread->wakeup_ext_cb = NULL;
#endif
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    thread->cpu_boost = 0;
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
    memset(&thread->pdist, 0, sizeof(thread->pdist));
    thread->blocker = NULL;
    thread->base_priority = priority;
    thread->priority = priority;
    thread->skip_count = priority;
    prio_add_entry(&thread->pdist, priority);
#endif

#ifdef HAVE_IO_PRIORITY
    /* Default to high (foreground) priority */
    thread->io_priority = IO_PRIORITY_IMMEDIATE;
#endif

#if NUM_CORES > 1
    thread->core = core;

    /* Writeback stack munging or anything else before starting */
    if (core != CURRENT_CORE)
    {
        commit_discard_idcache();
    }
#endif

    /* Thread is not on any timeout list but be a bit paranoid */
    thread->tmo.prev = NULL;

    state = (flags & CREATE_THREAD_FROZEN) ?
        STATE_FROZEN : STATE_RUNNING;
    
    thread->context.sp = (typeof (thread->context.sp))stackend;

    /* Load the thread's context structure with needed startup information */
    THREAD_STARTUP_INIT(core, thread, function);

    thread->state = state;
    i = thread->id; /* Snapshot while locked */

    if (state == STATE_RUNNING)
        core_schedule_wakeup(thread);

    UNLOCK_THREAD(thread);
    restore_irq(oldlevel);

    return i;
}

#ifdef HAVE_SCHEDULER_BOOSTCTRL
/*---------------------------------------------------------------------------
 * Change the boost state of a thread boosting or unboosting the CPU
 * as required.
 *---------------------------------------------------------------------------
 */
static inline void boost_thread(struct thread_entry *thread, bool boost)
{
    if ((thread->cpu_boost != 0) != boost)
    {
        thread->cpu_boost = boost;
        cpu_boost(boost);
    }
}

void trigger_cpu_boost(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    boost_thread(current, true);
}

void cancel_cpu_boost(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    boost_thread(current, false);
}
#endif /* HAVE_SCHEDULER_BOOSTCTRL */

/*---------------------------------------------------------------------------
 * Block the current thread until another thread terminates. A thread may
 * wait on itself to terminate which prevents it from running again and it
 * will need to be killed externally.
 * Parameter is the ID as returned from create_thread().
 *---------------------------------------------------------------------------
 */
void thread_wait(unsigned int thread_id)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    struct thread_entry *thread = thread_id_entry(thread_id);

    /* Lock thread-as-waitable-object lock */
    corelock_lock(&thread->waiter_cl);

    /* Be sure it hasn't been killed yet */
    if (thread->id == thread_id && thread->state != STATE_KILLED)
    {
        IF_COP( current->obj_cl = &thread->waiter_cl; )
        current->bqp = &thread->queue;

        disable_irq();
        block_thread(current);

        corelock_unlock(&thread->waiter_cl);

        switch_thread();
        return;
    }

    corelock_unlock(&thread->waiter_cl);
}

/*---------------------------------------------------------------------------
 * Exit the current thread. The Right Way to Do Things (TM).
 *---------------------------------------------------------------------------
 */
/* This is done to foil optimizations that may require the current stack,
 * such as optimizing subexpressions that put variables on the stack that
 * get used after switching stacks. */
#if NUM_CORES > 1
/* Called by ASM stub */
static void thread_final_exit_do(struct thread_entry *current)
#else
/* No special procedure is required before calling */
static inline void thread_final_exit(struct thread_entry *current)
#endif
{
    /* At this point, this thread isn't using resources allocated for
     * execution except the slot itself. */

    /* Signal this thread */
    thread_queue_wake(&current->queue);
    corelock_unlock(&current->waiter_cl);
    switch_thread();
    /* This should never and must never be reached - if it is, the
     * state is corrupted */
    THREAD_PANICF("thread_exit->K:*R", current);
    while (1);
}

void thread_exit(void)
{
    register struct thread_entry * current = cores[CURRENT_CORE].running;

    /* Cancel CPU boost if any */
    cancel_cpu_boost();

    disable_irq();

    corelock_lock(&current->waiter_cl);
    LOCK_THREAD(current);

#if defined (ALLOW_REMOVE_THREAD) && NUM_CORES > 1
    if (current->name == THREAD_DESTRUCT)
    {
        /* Thread being killed - become a waiter */
        unsigned int id = current->id;
        UNLOCK_THREAD(current);
        corelock_unlock(&current->waiter_cl);
        thread_wait(id);
        THREAD_PANICF("thread_exit->WK:*R", current);
    }
#endif

#ifdef HAVE_PRIORITY_SCHEDULING
    check_for_obj_waiters("thread_exit", current);
#endif

    if (current->tmo.prev != NULL)
    {
        /* Cancel pending timeout list removal */
        remove_from_list_tmo(current);
    }

    /* Switch tasks and never return */
    block_thread_on_l(current, STATE_KILLED);

    /* Slot must be unusable until thread is really gone */
    UNLOCK_THREAD_AT_TASK_SWITCH(current);

    /* Update ID for this slot */
    new_thread_id(current->id, current);
    current->name = NULL;

    /* Do final cleanup and remove the thread */
    thread_final_exit(current);
}

#ifdef ALLOW_REMOVE_THREAD
/*---------------------------------------------------------------------------
 * Remove a thread from the scheduler. Not The Right Way to Do Things in
 * normal programs.
 *
 * Parameter is the ID as returned from create_thread().
 *
 * Use with care on threads that are not under careful control as this may
 * leave various objects in an undefined state.
 *---------------------------------------------------------------------------
 */
void remove_thread(unsigned int thread_id)
{
#ifdef HAVE_CORELOCK_OBJECT
    /* core is not constant here because of core switching */
    unsigned int core = CURRENT_CORE;
    unsigned int old_core = NUM_CORES;
    struct corelock *ocl = NULL;
#else
    const unsigned int core = CURRENT_CORE;
#endif
    struct thread_entry *current = cores[core].running;
    struct thread_entry *thread = thread_id_entry(thread_id);

    unsigned state;
    int oldlevel;

    if (thread == current)
        thread_exit(); /* Current thread - do normal exit */

    oldlevel = disable_irq_save();

    corelock_lock(&thread->waiter_cl);
    LOCK_THREAD(thread);

    state = thread->state;

    if (thread->id != thread_id || state == STATE_KILLED)
        goto thread_killed;

#if NUM_CORES > 1
    if (thread->name == THREAD_DESTRUCT)
    {
        /* Thread being killed - become a waiter */
        UNLOCK_THREAD(thread);
        corelock_unlock(&thread->waiter_cl);
        restore_irq(oldlevel);
        thread_wait(thread_id);
        return;
    }

    thread->name = THREAD_DESTRUCT; /* Slot can't be used for now */

#ifdef HAVE_PRIORITY_SCHEDULING
    check_for_obj_waiters("remove_thread", thread);
#endif

    if (thread->core != core)
    {
        /* Switch cores and safely extract the thread there */
        /* Slot HAS to be unlocked or a deadlock could occur which means other
         * threads have to be guided into becoming thread waiters if they
         * attempt to remove it. */
        unsigned int new_core = thread->core;

        corelock_unlock(&thread->waiter_cl);

        UNLOCK_THREAD(thread);
        restore_irq(oldlevel);

        old_core = switch_core(new_core);

        oldlevel = disable_irq_save();

        corelock_lock(&thread->waiter_cl);
        LOCK_THREAD(thread);

        state = thread->state;
        core = new_core;
        /* Perform the extraction and switch ourselves back to the original
           processor */
    }
#endif /* NUM_CORES > 1 */

    if (thread->tmo.prev != NULL)
    {
        /* Clean thread off the timeout list if a timeout check hasn't
         * run yet */
        remove_from_list_tmo(thread);
    }

#ifdef HAVE_SCHEDULER_BOOSTCTRL
    /* Cancel CPU boost if any */
    boost_thread(thread, false);
#endif

IF_COP( retry_state: )

    switch (state)
    {
    case STATE_RUNNING:
        RTR_LOCK(core);
        /* Remove thread from ready to run tasks */
        remove_from_list_l(&cores[core].running, thread);
        rtr_subtract_entry(core, thread->priority);
        RTR_UNLOCK(core);
        break;
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        /* Remove thread from the queue it's blocked on - including its
         * own if waiting there */
#if NUM_CORES > 1
        if (&thread->waiter_cl != thread->obj_cl)
        {
            ocl = thread->obj_cl;

            if (UNLIKELY(corelock_try_lock(ocl) == 0))
            {
                UNLOCK_THREAD(thread);
                corelock_lock(ocl);
                LOCK_THREAD(thread);

                if (UNLIKELY(thread->state != state))
                {
                    /* Something woke the thread */
                    state = thread->state;
                    corelock_unlock(ocl);
                    goto retry_state;
                }
            }
        }
#endif
        remove_from_list_l(thread->bqp, thread);

#ifdef HAVE_WAKEUP_EXT_CB
        if (thread->wakeup_ext_cb != NULL)
            thread->wakeup_ext_cb(thread);
#endif

#ifdef HAVE_PRIORITY_SCHEDULING
        if (thread->blocker != NULL)
        {
            /* Remove thread's priority influence from its chain */
            wakeup_priority_protocol_release(thread);
        }
#endif

#if NUM_CORES > 1
        if (ocl != NULL)
            corelock_unlock(ocl);
#endif
        break;
    /* Otherwise thread is frozen and hasn't run yet */
    }

    new_thread_id(thread_id, thread);
    thread->state = STATE_KILLED;

    /* If thread was waiting on itself, it will have been removed above.
     * The wrong order would result in waking the thread first and deadlocking
     * since the slot is already locked. */
    thread_queue_wake(&thread->queue);

    thread->name = NULL;

thread_killed: /* Thread was already killed */
    /* Removal complete - safe to unlock and reenable interrupts */
    corelock_unlock(&thread->waiter_cl);
    UNLOCK_THREAD(thread);
    restore_irq(oldlevel);

#if NUM_CORES > 1
    if (old_core < NUM_CORES)
    {
        /* Did a removal on another processor's thread - switch back to
           native core */
        switch_core(old_core);
    }
#endif
}
#endif /* ALLOW_REMOVE_THREAD */

#ifdef HAVE_PRIORITY_SCHEDULING
/*---------------------------------------------------------------------------
 * Sets the thread's relative base priority for the core it runs on. Any
 * needed inheritance changes also may happen.
 *---------------------------------------------------------------------------
 */
int thread_set_priority(unsigned int thread_id, int priority)
{
    int old_base_priority = -1;
    struct thread_entry *thread = thread_id_entry(thread_id);

    /* A little safety measure */
    if (priority < HIGHEST_PRIORITY || priority > LOWEST_PRIORITY)
        return -1;

    /* Thread could be on any list and therefore on an interrupt accessible
       one - disable interrupts */
    int oldlevel = disable_irq_save();

    LOCK_THREAD(thread);

    /* Make sure it's not killed */
    if (thread->id == thread_id && thread->state != STATE_KILLED)
    {
        int old_priority = thread->priority;

        old_base_priority = thread->base_priority;
        thread->base_priority = priority;

        prio_move_entry(&thread->pdist, old_base_priority, priority);
        priority = find_first_set_bit(thread->pdist.mask);

        if (old_priority == priority)
        {
            /* No priority change - do nothing */
        }
        else if (thread->state == STATE_RUNNING)
        {
            /* This thread is running - change location on the run
             * queue. No transitive inheritance needed. */
            set_running_thread_priority(thread, priority);
        }
        else
        {
            thread->priority = priority;

            if (thread->blocker != NULL)
            {
                /* Bubble new priority down the chain */
                struct blocker *bl = thread->blocker;   /* Blocker struct */
                struct thread_entry *bl_t = bl->thread; /* Blocking thread */
                struct thread_entry * const tstart = thread;   /* Initial thread */
                const int highest = MIN(priority, old_priority); /* Higher of new or old */

                for (;;)
                {
                    struct thread_entry *next; /* Next thread to check */
                    int bl_pr;    /* Highest blocked thread */
                    int queue_pr; /* New highest blocked thread */
#if NUM_CORES > 1
                    /* Owner can change but thread cannot be dislodged - thread
                     * may not be the first in the queue which allows other
                     * threads ahead in the list to be given ownership during the
                     * operation. If thread is next then the waker will have to
                     * wait for us and the owner of the object will remain fixed.
                     * If we successfully grab the owner -- which at some point
                     * is guaranteed -- then the queue remains fixed until we
                     * pass by. */
                    for (;;)
                    {
                        LOCK_THREAD(bl_t);

                        /* Double-check the owner - retry if it changed */
                        if (LIKELY(bl->thread == bl_t))
                            break;

                        UNLOCK_THREAD(bl_t);
                        bl_t = bl->thread;
                    }
#endif
                    bl_pr = bl->priority;

                    if (highest > bl_pr)
                        break; /* Object priority won't change */

                    /* This will include the thread being set */
                    queue_pr = find_highest_priority_in_list_l(*thread->bqp);

                    if (queue_pr == bl_pr)
                        break; /* Object priority not changing */

                    /* Update thread boost for this object */
                    bl->priority = queue_pr;
                    prio_move_entry(&bl_t->pdist, bl_pr, queue_pr);
                    bl_pr = find_first_set_bit(bl_t->pdist.mask);

                    if (bl_t->priority == bl_pr)
                        break; /* Blocking thread priority not changing */

                    if (bl_t->state == STATE_RUNNING)
                    {
                        /* Thread not blocked - we're done */
                        set_running_thread_priority(bl_t, bl_pr);
                        break;
                    }

                    bl_t->priority = bl_pr;
                    bl = bl_t->blocker; /* Blocking thread has a blocker? */

                    if (bl == NULL)
                        break; /* End of chain */

                    next = bl->thread;

                    if (UNLIKELY(next == tstart))
                        break; /* Full-circle */

                    UNLOCK_THREAD(thread);

                    thread = bl_t;
                    bl_t = next;
                } /* for (;;) */

                UNLOCK_THREAD(bl_t);
            }
        }
    }

    UNLOCK_THREAD(thread);

    restore_irq(oldlevel);

    return old_base_priority;
}

/*---------------------------------------------------------------------------
 * Returns the current base priority for a thread.
 *---------------------------------------------------------------------------
 */
int thread_get_priority(unsigned int thread_id)
{
    struct thread_entry *thread = thread_id_entry(thread_id);
    int base_priority = thread->base_priority;

    /* Simply check without locking slot. It may or may not be valid by the
     * time the function returns anyway. If all tests pass, it is the
     * correct value for when it was valid. */
    if (thread->id != thread_id || thread->state == STATE_KILLED)
        base_priority = -1;

    return base_priority;
}
#endif /* HAVE_PRIORITY_SCHEDULING */

#ifdef HAVE_IO_PRIORITY
int thread_get_io_priority(unsigned int thread_id)
{
    struct thread_entry *thread = thread_id_entry(thread_id);
    return thread->io_priority;
}

void thread_set_io_priority(unsigned int thread_id,int io_priority)
{
    struct thread_entry *thread = thread_id_entry(thread_id);
    thread->io_priority = io_priority;
}
#endif

/*---------------------------------------------------------------------------
 * Starts a frozen thread - similar semantics to wakeup_thread except that
 * the thread is on no scheduler or wakeup queue at all. It exists simply by
 * virtue of the slot having a state of STATE_FROZEN.
 *---------------------------------------------------------------------------
 */
void thread_thaw(unsigned int thread_id)
{
    struct thread_entry *thread = thread_id_entry(thread_id);
    int oldlevel = disable_irq_save();

    LOCK_THREAD(thread);

    /* If thread is the current one, it cannot be frozen, therefore
     * there is no need to check that. */
    if (thread->id == thread_id && thread->state == STATE_FROZEN)
        core_schedule_wakeup(thread);

    UNLOCK_THREAD(thread);
    restore_irq(oldlevel);
}

#if NUM_CORES > 1
/*---------------------------------------------------------------------------
 * Switch the processor that the currently executing thread runs on.
 *---------------------------------------------------------------------------
 */
unsigned int switch_core(unsigned int new_core)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *current = cores[core].running;

    if (core == new_core)
    {
        /* No change - just return same core */
        return core;
    }

    int oldlevel = disable_irq_save();
    LOCK_THREAD(current);

    if (current->name == THREAD_DESTRUCT)
    {
        /* Thread being killed - deactivate and let process complete */
        unsigned int id = current->id;
        UNLOCK_THREAD(current);
        restore_irq(oldlevel);
        thread_wait(id);
        /* Should never be reached */
        THREAD_PANICF("switch_core->D:*R", current);
    }

    /* Get us off the running list for the current core */
    RTR_LOCK(core);
    remove_from_list_l(&cores[core].running, current);
    rtr_subtract_entry(core, current->priority);
    RTR_UNLOCK(core);

    /* Stash return value (old core) in a safe place */
    current->retval = core;

    /* If a timeout hadn't yet been cleaned-up it must be removed now or
     * the other core will likely attempt a removal from the wrong list! */
    if (current->tmo.prev != NULL)
    {
        remove_from_list_tmo(current);
    }

    /* Change the core number for this thread slot */
    current->core = new_core;

    /* Do not use core_schedule_wakeup here since this will result in
     * the thread starting to run on the other core before being finished on
     * this one. Delay the  list unlock to keep the other core stuck
     * until this thread is ready. */
    RTR_LOCK(new_core);

    rtr_add_entry(new_core, current->priority);
    add_to_list_l(&cores[new_core].running, current);

    /* Make a callback into device-specific code, unlock the wakeup list so
     * that execution may resume on the new core, unlock our slot and finally
     * restore the interrupt level */
    cores[core].blk_ops.flags = TBOP_SWITCH_CORE;
    cores[core].blk_ops.cl_p  = &cores[new_core].rtr_cl;
    cores[core].block_task    = current;

    UNLOCK_THREAD(current);

    /* Alert other core to activity */
    core_wake(new_core);

    /* Do the stack switching, cache_maintenence and switch_thread call -
       requires native code */
    switch_thread_core(core, current);

    /* Finally return the old core to caller */
    return current->retval;
}
#endif /* NUM_CORES > 1 */

/*---------------------------------------------------------------------------
 * Initialize threading API. This assumes interrupts are not yet enabled. On
 * multicore setups, no core is allowed to proceed until create_thread calls
 * are safe to perform.
 *---------------------------------------------------------------------------
 */
void init_threads(void)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *thread;

    if (core == CPU)
    {
        /* Initialize core locks and IDs in all slots */
        int n;
        for (n = 0; n < MAXTHREADS; n++)
        {
            thread = &threads[n];
            corelock_init(&thread->waiter_cl);
            corelock_init(&thread->slot_cl);
            thread->id = THREAD_ID_INIT(n);
        }
    }

    /* CPU will initialize first and then sleep */
    thread = find_empty_thread_slot();

    if (thread == NULL)
    {
        /* WTF? There really must be a slot available at this stage.
         * This can fail if, for example, .bss isn't zero'ed out by the loader
         * or threads is in the wrong section. */
        THREAD_PANICF("init_threads->no slot", NULL);
    }

    /* Initialize initially non-zero members of core */
    cores[core].next_tmo_check = current_tick; /* Something not in the past */

    /* Initialize initially non-zero members of slot */
    UNLOCK_THREAD(thread); /* No sync worries yet */
    thread->name = main_thread_name;
    thread->state = STATE_RUNNING;
    IF_COP( thread->core = core; )
#ifdef HAVE_PRIORITY_SCHEDULING
    corelock_init(&cores[core].rtr_cl);
    thread->base_priority = PRIORITY_USER_INTERFACE;
    prio_add_entry(&thread->pdist, PRIORITY_USER_INTERFACE);
    thread->priority = PRIORITY_USER_INTERFACE;
    rtr_add_entry(core, PRIORITY_USER_INTERFACE);
#endif

    add_to_list_l(&cores[core].running, thread);

    if (core == CPU)
    {
        thread->stack = stackbegin;
        thread->stack_size = (uintptr_t)stackend - (uintptr_t)stackbegin;
#if NUM_CORES > 1  /* This code path will not be run on single core targets */
        /* Wait for other processors to finish their inits since create_thread
         * isn't safe to call until the kernel inits are done. The first
         * threads created in the system must of course be created by CPU.
         * Another possible approach is to initialize all cores and slots
         * for each core by CPU, let the remainder proceed in parallel and
         * signal CPU when all are finished. */
        core_thread_init(CPU);
    } 
    else
    {
        /* Initial stack is the idle stack */
        thread->stack = idle_stacks[core];
        thread->stack_size = IDLE_STACK_SIZE;
        /* After last processor completes, it should signal all others to
         * proceed or may signal the next and call thread_exit(). The last one
         * to finish will signal CPU. */
        core_thread_init(core);
        /* Other cores do not have a main thread - go idle inside switch_thread
         * until a thread can run on the core. */
        thread_exit();
#endif /* NUM_CORES */
    }
#ifdef INIT_MAIN_THREAD
    init_main_thread(&thread->context);
#endif
}

/* Shared stack scan helper for thread_stack_usage and idle_stack_usage */
#if NUM_CORES == 1
static inline int stack_usage(uintptr_t *stackptr, size_t stack_size)
#else
static int stack_usage(uintptr_t *stackptr, size_t stack_size)
#endif
{
    unsigned int stack_words = stack_size / sizeof (uintptr_t);
    unsigned int i;
    int usage = 0;

    for (i = 0; i < stack_words; i++)
    {
        if (stackptr[i] != DEADBEEF)
        {
            usage = ((stack_words - i) * 100) / stack_words;
            break;
        }
    }

    return usage;
}

/*---------------------------------------------------------------------------
 * Returns the maximum percentage of stack a thread ever used while running.
 * NOTE: Some large buffer allocations that don't use enough the buffer to
 * overwrite stackptr[0] will not be seen.
 *---------------------------------------------------------------------------
 */
int thread_stack_usage(const struct thread_entry *thread)
{
    if (LIKELY(thread->stack_size > 0))
        return stack_usage(thread->stack, thread->stack_size);
    return 0;
}

#if NUM_CORES > 1
/*---------------------------------------------------------------------------
 * Returns the maximum percentage of the core's idle stack ever used during
 * runtime.
 *---------------------------------------------------------------------------
 */
int idle_stack_usage(unsigned int core)
{
    return stack_usage(idle_stacks[core], IDLE_STACK_SIZE);
}
#endif

/*---------------------------------------------------------------------------
 * Fills in the buffer with the specified thread's name. If the name is NULL,
 * empty, or the thread is in destruct state a formatted ID is written
 * instead.
 *---------------------------------------------------------------------------
 */
void thread_get_name(char *buffer, int size,
                     struct thread_entry *thread)
{
    if (size <= 0)
        return;

    *buffer = '\0';

    if (thread)
    {
        /* Display thread name if one or ID if none */
        const char *name = thread->name;
        const char *fmt = "%s";
        if (name == NULL IF_COP(|| name == THREAD_DESTRUCT) || *name == '\0')
        {
            name = (const char *)(uintptr_t)thread->id;
            fmt = "%04lX";
        }
        snprintf(buffer, size, fmt, name);
    }
}
