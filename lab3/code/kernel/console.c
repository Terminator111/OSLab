
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE *p_con);

PRIVATE char restore_pos[V_MEM_SIZE / 2];	 //在对应的pos索引存储操作
PRIVATE char restore_recall[V_MEM_SIZE / 2]; //存储需要撤销的操作（普通操作就存\b）
PRIVATE int recall_count = 0;

PRIVATE int isRecall = 0;

/*======================================================================*
			   undo
 *======================================================================*/
PUBLIC void undo(CONSOLE *p_con)
{
	if (recall_count == 0)
		return;
	isRecall = 1;
	out_char(p_con, restore_recall[--recall_count]);
	isRecall = 0;
}

/*======================================================================*
			   setColor
 *======================================================================*/
PUBLIC void setColor(CONSOLE *p_con, int index, char color)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + 2 * index);
	*(p_vmem + 1) = color;
}

/*======================================================================*
			   clean_screen
*======================================================================*/
PUBLIC void clean_screen(CONSOLE *p_con)
{
	isRecall = 0;
	u8 *p_vmem;
	while (p_con->cursor > p_con->original_addr)
	{
		p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
		*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
		*(p_vmem - 2) = '\0';
		p_con->cursor--;
	}
	recall_count = 0;
	flush(p_con);
}

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

	int con_v_mem_size = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0)
	{
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else
	{
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
	clean_screen(p_tty->p_console);
}

/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);

	switch (ch)
	{
	case '\t': // 将cursor往后移动TAB_LEN
		if (p_con->cursor < p_con->original_addr +
								p_con->v_mem_limit - TAB_LEN)
		{
			int pos = (int)(p_con->cursor - p_con->original_addr);
			for (int i = 0; i < TAB_LEN; i++)
			{
				*p_vmem++ = '\0'; // 区别Tab和空格
				*p_vmem++ = DEFAULT_CHAR_COLOR;
				restore_pos[pos++] = '\t';
			}
			p_con->cursor += TAB_LEN; // 调整光标
			if (!isRecall)
				restore_recall[recall_count++] = '\b';
		}
		break;
	case '\n':
		if (p_con->cursor < p_con->original_addr +
								p_con->v_mem_limit - SCREEN_WIDTH)
		{
			// 将回车的起始地点和终止地点都存储为\n
			int start = (int)(p_con->cursor - p_con->original_addr);
			restore_pos[start] = '\n';
			// 进入一个新行
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
													   ((p_con->cursor - p_con->original_addr) /
															SCREEN_WIDTH +
														1);
			int end = (int)(p_con->cursor - p_con->original_addr) - 1;
			// 在新行之前一个的位置存入一个\n
			restore_pos[end] = '\n';
			for (int i = start + 1; i < end; i++)
			{
				restore_pos[i] = '\0';
			}
			if (!isRecall)
				restore_recall[recall_count++] = '\b';
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr)
		{
			int pos = (int)(p_con->cursor - p_con->original_addr) - 1;
			switch (restore_pos[pos])
			{
			case '\t':
				for (int i = 0; i < TAB_LEN; i++)
				{
					restore_pos[pos--] = '\0'; // 区别于空格
					p_con->cursor--;
					*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
					*(p_vmem - 2) = '\0';
					p_vmem -= 2;
				}
				if (!isRecall)
					restore_recall[recall_count++] = '\t';
				break;
			case '\n':
				do
				{
					restore_pos[pos--] = '\0';
					*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
					*(p_vmem - 2) = '\0';
					p_con->cursor--;
					p_vmem -= 2;
				} while (restore_pos[pos] != '\n');
				p_con->cursor--;
				restore_pos[pos] = '\0';
				if (!isRecall)
					restore_recall[recall_count++] = '\n';
				break;
			default:
				if (!isRecall)
					restore_recall[recall_count++] = *(p_vmem - 2);
				p_con->cursor--;
				*(p_vmem - 2) = '\0';
				*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
				break;
			}
		}
		break;
	case 'z':
	case 'Z':
		if (control && !isRecall)
		{
			undo(p_con);
			return;
		}
	default:
		if (p_con->cursor <
			p_con->original_addr + p_con->v_mem_limit - 1)
		{
			*p_vmem++ = ch;
			*p_vmem++ = DEFAULT_CHAR_COLOR;
			p_con->cursor++;
			if (!isRecall)
				restore_recall[recall_count++] = '\b';
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con)
{
	set_cursor(p_con->cursor);
	set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES))
	{
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction)
{
	if (direction == SCR_UP)
	{
		if (p_con->current_start_addr > p_con->original_addr)
		{
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN)
	{
		if (p_con->current_start_addr + SCREEN_SIZE <
			p_con->original_addr + p_con->v_mem_limit)
		{
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else
	{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}
