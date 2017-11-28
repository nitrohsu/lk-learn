/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <compiler.h>
#include <debug.h>
#include <string.h>
#include <app.h>
#include <arch.h>
#include <platform.h>
#include <target.h>
#include <lib/heap.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/dpc.h>

extern void *__ctor_list;
extern void *__ctor_end;
extern int __bss_start;
extern int _end;

static int bootstrap2(void *arg);

static void call_constructors(void)
{
	void **ctor;
   
	ctor = &__ctor_list;
	while(ctor != &__ctor_end) {
		void (*func)(void);

		func = (void (*)())*ctor;

		func();
		ctor++;
	}
}

/* called from crt0.S */
void kmain(void) __NO_RETURN __EXTERNALLY_VISIBLE;
void kmain(void)
{
	// get us into some sort of thread context
	// 初始化化lk线程上下文,arm平台详见arch/arm/arch.c
	thread_init_early();

	// early arch stuff
	// 架构初始化，如关闭cache，使能mmu
	arch_early_init();

	// do any super early platform initialization
	// 平台早期初始化,详见platform/***/platform.c
	platform_early_init();

	// do any super early target initialization
	//目标设备早期初始化
	target_early_init();

	dprintf(INFO, "welcome to lk\n\n");
	
	// deal with any static constructors
	// 静态构造函数初始化
	dprintf(SPEW, "calling constructors\n");
	call_constructors();

	// bring up the kernel heap
    // 堆初始化
	dprintf(SPEW, "initializing heap\n");
	heap_init();

	// initialize the threading system
    // 初始化线程
	dprintf(SPEW, "initializing threads\n");
	thread_init();

	// initialize the dpc system
    // lk系统控制器初始化
	dprintf(SPEW, "initializing dpc\n");
	dpc_init();

	// initialize kernel timers
    // kernel时钟初始化
	dprintf(SPEW, "initializing timers\n");
	timer_init();

	// create a thread to complete system initialization
	dprintf(SPEW, "creating bootstrap completion thread\n");
	thread_resume(thread_create("bootstrap2", &bootstrap2, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));

	// enable interrupts
    // 使能中断
	exit_critical_section();

	// become the idle thread
    // 本线程切换为idle线程
	thread_become_idle();
}

int main(void);

static int bootstrap2(void *arg)
{
	dprintf(SPEW, "top of bootstrap2()\n");

	arch_init();

	// initialize the rest of the platform
    // 平台初始化, 主要初始化系统时钟,超频等
	dprintf(SPEW, "initializing platform\n");
	platform_init();
	
	// initialize the target
    // 目标设备初始化,主要初始化Flash,整合分区表
	dprintf(SPEW, "initializing target\n");
	target_init();

    // 应用功能初始化,调用aboot_init,加载kernel
	dprintf(SPEW, "calling apps_init()\n");
	apps_init();

	return 0;
}

