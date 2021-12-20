
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
													Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

PUBLIC void init()
{
	// 清屏
	disp_pos = 0;
	for (int i = 0; i < 80 * 25; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;

	// 初始化变量
	readerNum = 0;
	writerNum = 0;
	readerNum_rf = 0;
}

/*======================================================================*
							kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++)
	{
		strcpy(p_proc->p_name, p_task->name); // name of the process
		p_proc->pid = i;					  // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	for (int i = 0; i < NR_TASKS; i++)
	{
		proc_table[i].ticks = proc_table[i].priority = 1;
		proc_table[i].isBlocked = proc_table[i].sleep_ticks = 0;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	/* 初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
	out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);					   /* 让8259A可以接收时钟中断 */

	init();

	restart();

	while (1)
	{
	}
}

void ReaderA()
{
	// milli_delay(TIME_SLICE);
	while (TRUE)
	{
		READER("A\0", 2);
		milli_delay(TIME_SLICE / 2);
	}
}

void ReaderB()
{
	// milli_delay(10);
	while (TRUE)
	{
		READER("B\0", 3);
		milli_delay(TIME_SLICE / 2);
	}
}

void ReaderC()
{
	// milli_delay(5);
	while (TRUE)
	{
		READER("C\0", 3);
		milli_delay(TIME_SLICE / 2);
	}
}

void WriterD()
{
	// milli_delay(20);
	while (TRUE)
	{
		WRITER("D\0", 3);
		milli_delay(TIME_SLICE / 2);
	}
}

void WriterE()
{
	// milli_delay(20);
	while (TRUE)
	{
		WRITER("E\0", 4);
		milli_delay(TIME_SLICE / 2);
	}
}

void NormalF()
{
	// sleep(TIME_SLICE);
	char isR[23] = " process is reading>  ";
	char isW[15] = "<is writing>  ";
	while (TRUE)
	{
		if (readerNum > 0)
		{
			if (STRATEGY == 0)
			{ // 读优先
				print("<");
				char tmp[2] = {readerNum_rf + '0', '\0'};
				print(tmp);
				print(isR);
			}
			else
			{
				print("<");
				char tmp[2] = {readerNum + '0', '\0'};
				print(tmp);
				print(isR);
			}
		}
		else if (writerNum > 0)
		{
			print(isW);
		}
		else
		{
			continue;
		}
		sleep(TIME_SLICE);
	}
}
