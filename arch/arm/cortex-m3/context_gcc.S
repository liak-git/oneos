/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with 
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on 
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 *
 * @file        context_gcc.S
 *
 * @brief       This file provides context switch functions related to the ARM M3 architecture.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-23   OneOS Team      First version.
 ***********************************************************************************************************************
 */
.cpu    cortex-m3
.fpu    softvfp
.syntax unified
.thumb
.text

.equ    SCB_VTOR,           0xE000ED08      /* Vector table offset register. */
.equ    NVIC_INT_CTRL,      0xE000ED04      /* Interrupt control state register. */
.equ    NVIC_SHPR3,         0xE000ED20      /* System priority register (3). */
.equ    NVIC_PENDSV_PRI,    0x00FF0000      /* PendSV priority value (lowest). */
.equ    NVIC_PENDSVSET,     0x10000000      /* Value to trigger PendSV exception. */

/**
 ***********************************************************************************************************************
 * @brief           Disable interrupt.
 *
 * @param           None.
 *
 * @return          The state before disable interrupt.
 ***********************************************************************************************************************
 */
.global os_hw_interrupt_disable
.type os_hw_interrupt_disable, %function
os_hw_interrupt_disable:
    MRS     r0, PRIMASK
    CPSID   I
    BX      LR

/**
 ***********************************************************************************************************************
 * @brief           Enable interrupt.
 *
 * @param[in]       State to restore.
 *
 * @return          None.
 ***********************************************************************************************************************
 */
.global os_hw_interrupt_enable
.type os_hw_interrupt_enable, %function
os_hw_interrupt_enable:
    MSR     PRIMASK, r0
    BX      LR

/**
 ***********************************************************************************************************************
 * @brief           Start the task swtich process by software trigger PendSV interrupt.
 *
 * @param[in]       from            SP of 'from' task. from-->r0.
 * @param[in]       to              SP of 'to' task.   to  -->r1.
 *
 * @return          None.
 ***********************************************************************************************************************
 */
.global os_hw_context_switch_interrupt
.type os_hw_context_switch_interrupt, %function
.global os_hw_context_switch
.type os_hw_context_switch, %function

os_hw_context_switch_interrupt:
os_hw_context_switch:
    /* Set os_task_switch_interrupt_flag to 1. */
    LDR     r2, =os_task_switch_interrupt_flag
    LDR     r3, [r2]
    CMP     r3, #1
    BEQ     _reswitch
    MOV     r3, #1
    STR     r3, [r2]

    LDR     r2, =os_interrupt_from_task   /* Set os_interrupt_from_task. */
    STR     r0, [r2]

_reswitch:
    LDR     r2, =os_interrupt_to_task     /* Set os_interrupt_to_task. */
    STR     r1, [r2]

    LDR r0, =NVIC_INT_CTRL                /* Trigger the PendSV exception (causes context switch). */
    LDR r1, =NVIC_PENDSVSET
    STR r1, [r0]
    BX  LR

/**
 ***********************************************************************************************************************
 * @brief           PendSV interrupt handler,switch the context of the task.
 *
 * @param           None.
 *
 * @return          None.
 ***********************************************************************************************************************
 */
.global PendSV_Handler
.type PendSV_Handler, %function
PendSV_Handler:
    /* Disable interrupt to protect context switch. */
    MRS r2, PRIMASK
    CPSID   I

    /* Get os_task_switch_interrupt_flag. */
    LDR r0, =os_task_switch_interrupt_flag
    LDR r1, [r0]
    CBZ r1, pendsv_exit         /* Pendsv already handled. */

    /* Clear os_task_switch_interrupt_flag to 0. */
    MOV r1, #0x00
    STR r1, [r0]

    LDR r0, =os_interrupt_from_task
    LDR r1, [r0]
    CBZ r1, switch_to_task      /* Skip register save at the first time. */

    MRS r1, psp                 /* Get from task stack pointer. */
    STMFD   r1!, {r4 - r11}     /* Push r4 - r11 register. */
    LDR r0, [r0]
    STR r1, [r0]                /* Update from task stack pointer. */

switch_to_task:
    LDR r1, =os_interrupt_to_task
    LDR r1, [r1]
    LDR r1, [r1]                /* Load task stack pointer. */

    LDMFD   r1!, {r4 - r11}     /* Pop r4 - r11 register. */
    MSR psp, r1                 /* Update stack pointer. */

pendsv_exit:
    /* Restore interrupt. */
    MSR PRIMASK, r2

    ORR lr, lr, #0x04
    BX  lr

/**
 ***********************************************************************************************************************
 * @brief           This function is called when the scheduler starts the first task, Only used once.
 *
 * @param[in]       to              SP of 'to' task.
 *
 * @return          None.
 ***********************************************************************************************************************
 */
.global os_hw_context_switch_to
.type os_hw_context_switch_to, %function
os_hw_context_switch_to:
    LDR r1, =os_interrupt_to_task
    STR r0, [r1]

    /* Set from task to 0. */
    LDR r1, =os_interrupt_from_task
    MOV r0, #0x0
    STR r0, [r1]

    /* Set interrupt flag to 1. */
    LDR     r1, =os_task_switch_interrupt_flag
    MOV     r0, #1
    STR     r0, [r1]

    /* Set the PendSV exception priority. */
    LDR r0, =NVIC_SHPR3
    LDR r1, =NVIC_PENDSV_PRI
    LDR.W   r2, [r0,#0x00]       /* Read. */
    ORR     r1,r1,r2             /* Modify. */
    STR     r1, [r0]             /* Write-back. */

    LDR r0, =NVIC_INT_CTRL       /* Trigger the PendSV exception (causes context switch). */
    LDR r1, =NVIC_PENDSVSET
    STR r1, [r0]

    /* Restore MSP. */
    LDR     r0, =SCB_VTOR
    LDR     r0, [r0]
    LDR     r0, [r0]
    NOP
    MSR     msp, r0

    /* Enable interrupts at processor level. */
    CPSIE   F
    CPSIE   I

    /* Never reach here! */

/**
 ***********************************************************************************************************************
 * @brief           HardFault interrupt handler.
 *
 * @param           None.
 *
 * @return          None.
 ***********************************************************************************************************************
 */
.global HardFault_Handler
.type HardFault_Handler, %function
HardFault_Handler:
    /* Get current context. */
    MRS     r0, msp                 /* Get fault context from handler. */
    TST     lr, #0x04               /* if(!EXC_RETURN[2]). */
    BEQ     _get_sp_done
    MRS     r0, psp                 /* Get fault context from task. */
_get_sp_done:

    STMFD   r0!, {r4 - r11}         /* Push r4 - r11 register. */
    STMFD   r0!, {lr}               /* Push exec_return register. */

    TST     lr, #0x04               /* if(!EXC_RETURN[2]). */
    BEQ     _update_msp
    MSR     psp, r0                 /* Update stack pointer to PSP. */
    B       _update_done
_update_msp:
    MSR     msp, r0                 /* Update stack pointer to MSP. */
_update_done:

    PUSH    {LR}
    BL      os_hw_hard_fault_exception
    POP     {LR}

    ORR     lr, lr, #0x04
    BX      lr

