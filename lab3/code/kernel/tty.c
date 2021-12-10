/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

#define TTY_FIRST (tty_table)
#define TTY_END (tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY *p_tty);
PRIVATE void tty_do_read(TTY *p_tty);
PRIVATE void tty_do_write(TTY *p_tty);
PRIVATE void put_key(TTY *p_tty, u32 key);

PRIVATE int searchStartCursor = 0;
PRIVATE char searchStr[50] = {'\0'};
PRIVATE int searchLen = 0;
PUBLIC int STATUS = NORMAL_MODE;

PUBLIC void __stack_chk_fail()
{
}

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
    TTY *p_tty;
    STATUS = NORMAL_MODE; // 正常情况下都处于正常输入状态
    searchStartCursor = 0;
    searchLen = 0;

    init_keyboard();

    for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++)
    {
        init_tty(p_tty);
    }
    select_console(0);

    while (1)
    {
        for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++)
        {
            tty_do_read(p_tty);
            tty_do_write(p_tty);
        }
    }
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY *p_tty)
{
    p_tty->inbuf_count = 0;
    p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

    init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY *p_tty, u32 key)
{
    char output[2] = {'\0', '\0'};

    if (!(key & FLAG_EXT))
    {
        put_key(p_tty, key);
    }
    else
    {
        int raw_code = key & MASK_RAW;
        switch (raw_code)
        {
        case TAB:
            put_key(p_tty, '\t');
            break;
        case ESC:
            if (STATUS == NORMAL_MODE)
            {
                STATUS = ESC_MODE;
                // 记录搜索前的光标位置
                searchStartCursor = p_tty->p_console->cursor;
            }
            else if (STATUS == SEARCH_MODE)
            {
                STATUS = NORMAL_MODE;
                exitESC(p_tty->p_console);
            }
            break;
        case ENTER:
            put_key(p_tty, '\n');
            break;
        case BACKSPACE:
            put_key(p_tty, '\b');
            break;
        case UP:
            if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
            {
                scroll_screen(p_tty->p_console, SCR_DN);
            }
            break;
        case DOWN:
            if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
            {
                scroll_screen(p_tty->p_console, SCR_UP);
            }
            break;
        case F1:
        case F2:
        case F3:
        case F4:
        case F5:
        case F6:
        case F7:
        case F8:
        case F9:
        case F10:
        case F11:
        case F12:
            /* Alt + F1~F12 */
            if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R))
            {
                select_console(raw_code - F1);
            }
            break;
        default:
            break;
        }
    }
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY *p_tty, u32 key)
{
    if (p_tty->inbuf_count < TTY_IN_BYTES)
    {
        *(p_tty->p_inbuf_head) = key;
        p_tty->p_inbuf_head++;
        if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES)
        {
            p_tty->p_inbuf_head = p_tty->in_buf;
        }
        p_tty->inbuf_count++;
    }
}

/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY *p_tty)
{
    if (is_current_console(p_tty->p_console))
    {
        keyboard_read(p_tty);
    }
}

/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY *p_tty)
{
    if (p_tty->inbuf_count)
    {
        char ch = *(p_tty->p_inbuf_tail);
        p_tty->p_inbuf_tail++;
        if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES)
        {
            p_tty->p_inbuf_tail = p_tty->in_buf;
        }
        p_tty->inbuf_count--;

        // 判断模式
        if (STATUS == NORMAL_MODE)
        {
            // 正常输入模式
            out_char(p_tty->p_console, ch);
        }
        else if (STATUS == ESC_MODE)
        {
            // 输入查找函数的时候
            switch (ch)
            {
            case '\b':
                if (p_tty->p_console->cursor <= searchStartCursor)
                    // 搜索越界
                    return;
                else
                {
                    out_char(p_tty->p_console, ch);
                    searchLen--;
                }
                break;
            case '\n':
                STATUS = SEARCH_MODE;
                showSearchResult(p_tty->p_console);
                break;
            case '\t':
                break;
            default:
                searchStr[searchLen++] = ch;
                out_char(p_tty->p_console, ch);
                setColor(p_tty->p_console, p_tty->p_console->cursor - 1, SEARCH_CHAR_COLOR);
                break;
            }
        }
    }
}

PUBLIC void exitESC(CONSOLE *p_con)
{
    // 删除搜索的字符串
    STATUS = NORMAL_MODE; // 退出查找模式
    for (int i = 0; i < searchLen; i++)
    {
        out_char(p_con, '\b');
    }
    // 重置颜色
    for (int i = 0; i < searchStartCursor - p_con->original_addr; i++)
    {
        setColor(p_con, i + p_con->original_addr, DEFAULT_CHAR_COLOR);
    }
    // 清空搜索字符串
    for (int i = 0; i < searchLen; i++)
    {
        searchStr[i] = '?';
    }
    searchLen = 0;
}

PUBLIC void showSearchResult(CONSOLE *p_con)
{
    // 将匹配到的值设置为黑底红色
    int start = p_con->original_addr;
    int end = searchStartCursor;
    u8 *ptr;
    for (int i = start; i < end; i++)
    {
        ptr = (u8 *)V_MEM_BASE + 2 * i;
        if (judge(ptr))
        {
            for (int j = 0; j < searchLen; j++)
            {
                setColor(p_con, i + j, SEARCH_CHAR_COLOR);
            }
        }
    }
}

PUBLIC int judge(u8 *str)
{
    // 匹配从str开始的字符串和searchStr是否相同
    u8 *p_str = str;
    // 输入文本的终点
    u8 *end = (u8 *)(V_MEM_BASE + searchStartCursor * 2);
    for (int i = 0;
         i < searchLen && p_str < end;
         i++, p_str += 2)
    {
        if (*(p_str) != searchStr[i])
            return 0;
    }
    return 1;
}
