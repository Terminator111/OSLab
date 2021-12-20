
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							   proc.c
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

char stR[19] = " starts reading.  ";
char isR[15] = " is reading.  ";
char fiR[21] = " finishes reading.  ";

/*======================================================================*
							  schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS *p;
	int greatest_ticks = 0;

	// 让所有在休眠的进程等待时间-1
	for (p = proc_table; p < proc_table + NR_TASKS; p++)
	{
		if (p->sleep_ticks > 0)
			p->sleep_ticks--;
	}

	while (!greatest_ticks)
	{ // 找到当前等待最久的进程，此进程不能处于睡眠状态，也不能在被阻塞状态
		for (p = proc_table; p < proc_table + NR_TASKS; p++)
		{
			// 正在睡眠or阻塞的进程不会被执行（不会被分配时间片）
			if (p->sleep_ticks > 0 || p->isBlocked == TRUE)
				continue;
			if (p->ticks > greatest_ticks)
			{
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		// 如果都是0，则重设ticks
		if (!greatest_ticks)
		{
			for (p = proc_table; p < proc_table + NR_TASKS; p++)
			{
				if (p->ticks > 0) // 说明被阻塞了
					continue;
				p->ticks = p->priority;
			}
		}
	}
}

/*======================================================================*
						   sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC int sys_sleep(int milli_seconds)
{
	p_proc_ready->sleep_ticks = milli_seconds / (1000 / HZ * 500); // 最后的乘500是为了修正系统的时钟中断错误（maybe?）
	schedule();
}

PUBLIC int sys_print(char *str)
{
	if (disp_pos >= 80 * 25 * 2)
	{
		memset(0xB8000, 0, 80 * 25 * 2);
		disp_pos = 0;
	}
	int offset = p_proc_ready - proc_table;
	int colors[] = {RED, GREEN, BLUE, YELLOW, PURPLE, WHITE};
	disp_color_str(str, BRIGHT | MAKE_COLOR(BLACK, colors[offset]));
}

PUBLIC int sys_P(void *sem)
{
	disable_irq(CLOCK_IRQ); // 保证原语
	SEMAPHORE *s = (SEMAPHORE *)sem;
	s->value--;
	if (s->value < 0)
	{ // 将进程加入队列尾
		p_proc_ready->isBlocked = TRUE;
		s->pQueue[s->tail] = p_proc_ready;
		s->tail = (s->tail + 1) % NR_TASKS;
		schedule();
	}
	enable_irq(CLOCK_IRQ);
}

PUBLIC int sys_V(void *sem)
{
	disable_irq(CLOCK_IRQ); // 保证原语
	SEMAPHORE *s = (SEMAPHORE *)sem;
	s->value++;
	if (s->value <= 0)
	{ // 释放队列头的进程
		PROCESS *proc = s->pQueue[s->head];
		proc->isBlocked = FALSE;
		s->head = (s->head + 1) % NR_TASKS;
	}
	enable_irq(CLOCK_IRQ);
}

PUBLIC void READER(char *name, int time_slice)
{
	print("Reader ");
	print(name);
	print(" arrives.  ");
	switch (STRATEGY)
	{
	case 0:
		READER_rf(name, time_slice);
		break;
	case 1:
		READER_wf(name, time_slice);
		break;
	default:
		READER_fair(name, time_slice);
		break;
	}
}

PUBLIC void WRITER(char *name, int time_slice)
{
	print("Writer ");
	print(name);
	print(" arrives.  ");
	switch (STRATEGY)
	{
	case 0:
		WRITER_rf(name, time_slice);
		break;
	case 1:
		WRITER_wf(name, time_slice);
		break;
	default:
		WRITER_fair(name, time_slice);
		break;
	}
}

/* 读优先 */
void READER_rf(char *name, int time_slice)
{
	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock); // 有读者，则禁止写
	readerNum++;
	V(&mutex_readerNum);

	P(&readerLimit);
	// 进行读，对写操作加锁
	readerNum_rf++;
	print(name);
	print(stR);
	print(name);
	print(isR);
	sleep(time_slice * TIME_SLICE);

	// 完成读
	print(name);
	print(fiR);
	readerNum_rf--;

	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock); // 无读者，可写
	V(&mutex_readerNum);

	V(&readerLimit);
}

void WRITER_rf(char *name, int time_slice)
{
	char stW[19] = " starts writing.  ";
	char isW[15] = " is writing.  ";
	char fiW[21] = " finishes writing.  ";

	P(&writeBlock);

	print(name);
	print(stW);
	writerNum++;
	print(name);
	print(isW);
	sleep(time_slice * TIME_SLICE);
	print(name);
	print(fiW);
	writerNum--;

	V(&writeBlock);
}

/* 写优先 */
void READER_wf(char *name, int time_slice)
{
	P(&readerLimit);

	P(&readBlock);

	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock); // 有读者，则禁止写
	readerNum++;
	V(&mutex_readerNum);

	V(&readBlock);

	// 进行读，对写操作加锁
	print(name);
	print(stR);
	print(name);
	print(isR);
	sleep(time_slice * TIME_SLICE);

	// 完成读
	print(name);
	print(fiR);
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock); // 无读者，可写
	V(&mutex_readerNum);

	V(&readerLimit);
}

void WRITER_wf(char *name, int time_slice)
{
	char stW[19] = " starts writing.  ";
	char isW[15] = " is writing.  ";
	char fiW[21] = " finishes writing.  ";

	P(&mutex_writerNum);
	writerNum++;
	if (writerNum == 1)
		P(&readBlock); // 有写者，则禁止读
	V(&mutex_writerNum);

	// 开始写
	P(&writeBlock);
	print(name);
	print(stW);
	print(name);
	print(isW);
	sleep(time_slice * TIME_SLICE);

	// 完成写
	P(&mutex_writerNum);
	writerNum--;
	if (writerNum == 0)
		V(&readBlock); // 无写者，可读
	V(&mutex_writerNum);

	print(name);
	print(fiW);

	V(&writeBlock);
}

/* 读写公平 */
void READER_fair(char *name, int time_slice)
{
	// 开始读
	P(&mutex_fair);

	P(&readerLimit);
	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock);
	V(&mutex_fair);

	readerNum++;
	V(&mutex_readerNum);

	// 进行读，对写操作加锁
	print(name);
	print(stR);
	print(name);
	print(isR);
	sleep(time_slice * TIME_SLICE);

	// 完成读
	print(name);
	print(fiR);
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock);
	V(&mutex_readerNum);

	V(&readerLimit);
}

void WRITER_fair(char *name, int time_slice)
{
	char stW[19] = " starts writing.  ";
	char isW[15] = " is writing.  ";
	char fiW[21] = " finishes writing.  ";

	P(&mutex_fair);
	P(&writeBlock);
	V(&mutex_fair);

	// 开始写
	print(name);
	print(stW);
	writerNum++;
	print(name);
	print(isW);
	sleep(time_slice * TIME_SLICE);
	// 完成写
	print(name);
	print(fiW);
	writerNum--;

	V(&writeBlock);
}
