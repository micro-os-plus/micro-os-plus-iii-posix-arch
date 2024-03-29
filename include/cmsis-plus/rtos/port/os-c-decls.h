/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2016-2023 Liviu Ionescu. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose is hereby granted, under the terms of the MIT license.
 *
 * If a copy of the license was not distributed with this file, it can
 * be obtained from https://opensource.org/licenses/mit/.
 */

/*
 * The initial CMSIS++ RTOS API was inspired by CMSIS RTOS API v1.x,
 * Copyright (c) 2013 ARM LIMITED
 */

/*
 * This file is part of the CMSIS++ proposal, intended as a CMSIS
 * replacement for C++ applications.
 *
 * It is included in `cmsis-plus/rtos/os-c-decls.h` to customise
 * it with port specific declarations.
 *
 * These structures (which basically contain handlers)
 * are conditionally included in the system objects
 * when they are implemented using the port native objects.
 */

#ifndef CMSIS_PLUS_RTOS_PORT_OS_C_DECLS_H_
#define CMSIS_PLUS_RTOS_PORT_OS_C_DECLS_H_

#include <stdint.h>

#if !defined (_XOPEN_SOURCE)
#error This port requires defining _XOPEN_SOURCE=600L or 700L globally
#endif

#if defined(OS_INCLUDE_LIBUCONTEXT)

#include <libucontext/libucontext.h>

#define os_impl_ucontext_t libucontext_ucontext_t
#define os_impl_getcontext libucontext_posix_getcontext
#define os_impl_makecontext libucontext_makecontext
#define os_impl_setcontext libucontext_posix_setcontext
#define os_impl_swapcontext libucontext_posix_swapcontext

#else

#include <ucontext.h>

#define os_impl_ucontext_t ucontext_t
#define os_impl_getcontext getcontext
#define os_impl_makecontext makecontext
#define os_impl_setcontext setcontext
#define os_impl_swapcontext swapcontext

#endif // defined(OS_INCLUDE_LIBUCONTEXT)

#include <signal.h>
#include <stdbool.h>

// Must match port::clock::timestamp_t
typedef uint64_t os_port_clock_timestamp_t;

// Must match port::clock::duration_t
typedef uint32_t os_port_clock_duration_t;

// Must match port::clock::offset_t
typedef uint64_t os_port_clock_offset_t;

typedef struct
{
  os_impl_ucontext_t ucontext; //
} os_port_thread_context_t;

typedef bool os_port_scheduler_state_t;

// Signal set (true if signal blocked)
typedef bool os_port_irq_state_t;

typedef uint64_t os_port_thread_stack_element_t;
typedef uint64_t os_port_thread_stack_allocation_element_t;

#endif /* CMSIS_PLUS_RTOS_PORT_OS_C_DECLS_H_ */
