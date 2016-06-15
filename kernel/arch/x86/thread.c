// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2009 Corey Tabaka
// Copyright (c) 2014 Travis Geiselbrecht
// Copyright (c) 2015 Intel Corporation
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <arch/fpu.h>
#include <arch/x86.h>
#include <arch/x86/descriptor.h>
#include <arch/x86/mp.h>

void arch_thread_initialize(thread_t *t, vaddr_t entry_point)
{
    // create a default stack frame on the stack
    vaddr_t stack_top = (vaddr_t)t->stack + t->stack_size;

#if ARCH_X86_32
    // make sure the top of the stack is 8 byte aligned for ABI compliance
    stack_top = ROUNDDOWN(stack_top, 8);
    struct x86_32_context_switch_frame *frame = (struct x86_32_context_switch_frame *)(stack_top);
#endif
#if ARCH_X86_64
    // make sure the top of the stack is 16 byte aligned for ABI compliance
    stack_top = ROUNDDOWN(stack_top, 16);

    // make sure we start the frame 8 byte unaligned (relative to the 16 byte alignment) because
    // of the way the context switch will pop the return address off the stack. After the first
    // context switch, this leaves the stack in unaligned relative to how a called function expects it.
    stack_top -= 8;
    struct x86_64_context_switch_frame *frame = (struct x86_64_context_switch_frame *)(stack_top);
#endif

    // move down a frame size and zero it out
    frame--;
    memset(frame, 0, sizeof(*frame));

#if ARCH_X86_32
    frame->eip = entry_point;
    frame->eflags = 0x3002; // IF = 0, NT = 0, IOPL = 3
#endif

#if ARCH_X86_64
    frame->rip = entry_point;
    frame->rflags = 0x3002; /* IF = 0, NT = 0, IOPL = 3 */
#endif

    // initialize the saved fpu state
    fpu_init_thread_states(t);

    // set the stack pointer
    t->arch.sp = (vaddr_t)frame;

    // initialize the fs, gs and kernel bases to 0.
#if ARCH_X86_64
    t->arch.fs_base = 0;
    t->arch.gs_base = 0;
#endif
}

void arch_dump_thread(thread_t *t)
{
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
        dprintf(INFO, "sp 0x%lx\n", t->arch.sp);
    }
}

#if ARCH_X86_32

void arch_context_switch(thread_t *oldthread, thread_t *newthread)
{
    //dprintf(DEBUG, "arch_context_switch: old %p (%s), new %p (%s)\n", oldthread, oldthread->name, newthread, newthread->name);

    fpu_context_switch(oldthread, newthread);

    __asm__ __volatile__ (
        "pushl $1f			\n\t"
        "pushf				\n\t"
        "pusha				\n\t"
        "movl %%esp,(%%edx)	\n\t"
        "movl %%eax,%%esp	\n\t"
        "popa				\n\t"
        "popf				\n\t"
        "ret				\n\t"
        "1:					\n\t"

        :
        : "d" (&oldthread->arch.sp), "a" (newthread->arch.sp)
    );

    /*__asm__ __volatile__ (
        "pushf              \n\t"
        "pushl %%cs         \n\t"
        "pushl $1f          \n\t"
        "pushl %%gs         \n\t"
        "pushl %%fs         \n\t"
        "pushl %%es         \n\t"
        "pushl %%ds         \n\t"
        "pusha              \n\t"
        "movl %%esp,(%%edx) \n\t"
        "movl %%eax,%%esp   \n\t"
        "popa               \n\t"
        "popl %%ds          \n\t"
        "popl %%es          \n\t"
        "popl %%fs          \n\t"
        "popl %%gs          \n\t"
        "iret               \n\t"
        "1: "
        :
        : "d" (&oldthread->arch.sp), "a" (newthread->arch.sp)
    );*/
}
#endif

#if ARCH_X86_64

void arch_context_switch(thread_t *oldthread, thread_t *newthread)
{
    fpu_context_switch(oldthread, newthread);

    /* TODO: precompute the stack top and put in the thread structure directly */
    vaddr_t kstack_top = (vaddr_t)newthread->stack + newthread->stack_size;
    kstack_top = ROUNDDOWN(kstack_top, 16);

    //printf("cs 0x%llx\n", kstack_top);

    /* set the tss SP0 value to point at the top of our stack */
    x86_set_tss_sp(kstack_top);

    /* set the kernel sp in our per cpu gs: structure */
    x86_set_percpu_kernel_sp(kstack_top);

    /* user and kernel gs have been swapped, so unswap them when loading
     * from the msrs
     */
    oldthread->arch.fs_base = read_msr(X86_MSR_IA32_FS_BASE);
    oldthread->arch.gs_base = read_msr(X86_MSR_IA32_KERNEL_GS_BASE);

    write_msr(X86_MSR_IA32_FS_BASE, newthread->arch.fs_base);
    write_msr(X86_MSR_IA32_KERNEL_GS_BASE, newthread->arch.gs_base);

    x86_64_context_switch(&oldthread->arch.sp, newthread->arch.sp);
}
#endif
