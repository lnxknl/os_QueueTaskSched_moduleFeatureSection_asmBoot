/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <kernel/thread.h>
#include <limits.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <stdint.h>

__BEGIN_CDECLS

typedef uint32_t mp_cpu_mask_t;

#define MP_CPU_ALL_BUT_LOCAL (UINT32_MAX)

/* by default, mp_mbx_reschedule does not signal to cpus that are running realtime
 * threads. Override this behavior.
 */
#define MP_RESCHEDULE_FLAG_REALTIME (0x1)

typedef enum {
    MP_IPI_GENERIC,
    MP_IPI_RESCHEDULE,
} mp_ipi_t;

#ifdef WITH_SMP
void mp_init(void);

void mp_reschedule(mp_cpu_mask_t target, uint flags);
void mp_set_curr_cpu_active(bool active);

/* called from arch code during reschedule irq */
enum handler_return mp_mbx_reschedule_irq(void);

/* global mp state to track what the cpus are up to */
struct mp_state {
    volatile mp_cpu_mask_t active_cpus;

    /* only safely accessible with thread lock held */
    mp_cpu_mask_t idle_cpus;
    mp_cpu_mask_t realtime_cpus;
};

extern struct mp_state mp;

static inline int mp_is_cpu_active(uint cpu) {
    return mp.active_cpus & (1 << cpu);
}

static inline int mp_is_cpu_idle(uint cpu) {
    return mp.idle_cpus & (1 << cpu);
}

/* must be called with the thread lock held */
static inline void mp_set_cpu_idle(uint cpu) {
    mp.idle_cpus |= 1UL << cpu;
}

static inline void mp_set_cpu_busy(uint cpu) {
    mp.idle_cpus &= ~(1UL << cpu);
}

static inline mp_cpu_mask_t mp_get_idle_mask(void) {
    return mp.idle_cpus;
}

static inline void mp_set_cpu_realtime(uint cpu) {
    mp.realtime_cpus |= 1UL << cpu;
}

static inline void mp_set_cpu_non_realtime(uint cpu) {
    mp.realtime_cpus &= ~(1UL << cpu);
}

static inline mp_cpu_mask_t mp_get_realtime_mask(void) {
    return mp.realtime_cpus;
}
#else
static inline void mp_init(void) {}
static inline void mp_reschedule(mp_cpu_mask_t target, uint flags) {}
static inline void mp_set_curr_cpu_active(bool active) {}

static inline enum handler_return mp_mbx_reschedule_irq(void) { return INT_NO_RESCHEDULE; }

// only one cpu exists in UP and if you're calling these functions, it's active...
static inline int mp_is_cpu_active(uint cpu) { return 1; }
static inline int mp_is_cpu_idle(uint cpu) { return (get_current_thread()->flags & THREAD_FLAG_IDLE) != 0; }

static inline void mp_set_cpu_idle(uint cpu) {}
static inline void mp_set_cpu_busy(uint cpu) {}

static inline mp_cpu_mask_t mp_get_idle_mask(void) { return 0; }

static inline void mp_set_cpu_realtime(uint cpu) {}
static inline void mp_set_cpu_non_realtime(uint cpu) {}

static inline mp_cpu_mask_t mp_get_realtime_mask(void) { return 0; }
#endif

__END_CDECLS
