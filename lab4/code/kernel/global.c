
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"

PUBLIC PROCESS proc_table[NR_TASKS];

PUBLIC char task_stack[STACK_SIZE_TOTAL];

PUBLIC TASK task_table[NR_TASKS] = {{ReaderA, STACK_SIZE_READER_A, "ReaderA"},
                                    {ReaderB, STACK_SIZE_READER_B, "ReaderB"},
                                    {ReaderC, STACK_SIZE_READER_C, "ReaderC"},
                                    {WriterD, STACK_SIZE_WRITER_D, "WriterD"},
                                    {WriterE, STACK_SIZE_WRITER_E, "WriterE"},
                                    {NormalF, STACK_SIZE_NORMAL_F, "NormalF"}};

PUBLIC irq_handler irq_table[NR_IRQ];

PUBLIC system_call sys_call_table[NR_SYS_CALL] = {sys_get_ticks,
                                                  sys_sleep,
                                                  sys_print,
                                                  sys_P,
                                                  sys_V};

PUBLIC SEMAPHORE readerLimit = {MAX_READERS, 0, 0};
PUBLIC SEMAPHORE writeBlock = {1, 0, 0};
PUBLIC SEMAPHORE readBlock = {1, 0, 0};
PUBLIC SEMAPHORE mutex_readerNum = {1, 0, 0};
PUBLIC SEMAPHORE mutex_writerNum = {1, 0, 0};
PUBLIC SEMAPHORE mutex_fair = {1, 0, 0};
