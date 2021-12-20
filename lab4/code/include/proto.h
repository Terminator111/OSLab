
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* klib.asm */
PUBLIC void out_byte(u16 port, u8 value);
PUBLIC u8 in_byte(u16 port);
PUBLIC void disp_str(char *info);
PUBLIC void disp_color_str(char *info, int color);

/* protect.c */
PUBLIC void init_prot();
PUBLIC u32 seg2phys(u16 seg);

/* klib.c */
PUBLIC void delay(int time);

/* kernel.asm */
void restart();

/* main.c */
void ReaderA();
void ReaderB();
void ReaderC();
void WriterD();
void WriterE();
void NormalF();
/* 读写接口函数 */
PUBLIC void WRITER(char *name, int time_slice);
PUBLIC void READER(char *name, int time_slice);
/* 读优先 */
PUBLIC void WRITER_rf(char *name, int time_slice);
PUBLIC void READER_rf(char *name, int time_slice);
/* 写优先 */
PUBLIC void WRITER_wf(char *name, int time_slice);
PUBLIC void READER_wf(char *name, int time_slice);
/* 读写公平 */
PUBLIC void WRITER_fair(char *name, int time_slice);
PUBLIC void READER_fair(char *name, int time_slice);

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);

/* 以下是系统调用相关 */

/* 系统调用 - 系统级 */
/* proc.c */
PUBLIC int sys_get_ticks(); /* sys_call */
// addition
PUBLIC int sys_sleep(int milli_seconds);
PUBLIC int sys_print(char *str);
PUBLIC int sys_P(void *s);
PUBLIC int sys_V(void *s);

/* syscall.asm */
PUBLIC void sys_call(); /* int_handler */

/* 系统调用 - 用户级 */
PUBLIC int get_ticks();
// addition
PUBLIC int sleep(int milli_seconds);
PUBLIC int print(char *str);
PUBLIC int P(void *s);
PUBLIC int V(void *s);
