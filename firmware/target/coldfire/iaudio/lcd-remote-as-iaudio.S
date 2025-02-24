/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-remote-as-iaudio.S 17847 2008-06-28 18:10:04Z bagder $
 *
 * Copyright (C) 2008 by Jens Arnold
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#define CLOCK_MASK     0x00004000
#define DATA_MASK      0x00002000
#define RS_MASK        0x00008000
#define GPIO_OUT_ADDR  0x80000004

#define CS_MASK        0x00000020 /* used in moveq.l */
#define GPIO1_OUT_ADDR 0x800000b4

    .extern cpu_frequency       /* Global variable from system.c */

    .section    .icode,"ax",@progbits

    /* Output 8 bits to the LCD. Instruction order is devised to maximize the
     * delay between changing the data line and the CLK L->H transition, which
     * makes the LCD controller sample DATA.
     * Requires CLK = 1 on entry.
     *
     * Custom calling convention:
     *   %a0 - GPIO_OUT_ADDR
     *   %d3 - data byte
     *   %d6 - DATA_MASK
     *   %d7 - CLOCK_MASK
     * Clobbers:
     *   %d0..%d3 
     */
.write_byte:
    move.w  %sr, %d2
    move.w  #0x2700, %sr

    move.l  (%a0), %d0          /* Get current state of data port */
    move.l  %d0, %d1
    and.l   %d6, %d1            /* Check current state of data line */
    beq.s   1f                  /*   and set it as previous-state bit */
    bset    #8, %d3
1:
    move.l  %d3, %d1            /* Compute the 'bit derivative', i.e. a value */
    lsr.l   #1, %d1             /*   with 1's where the data changes from the */
    eor.l   %d1, %d3            /*   previous state, and 0's where it doesn't */
    swap    %d3                 /* Shift data to upper byte */
    lsl.l   #8, %d3

    move.l  %d0, %d1            /* precalculate opposite state of clock line */
    eor.l   %d7, %d1

    lsl.l   #1, %d3             /* Shift out MSB */
    bcc.s   1f
    eor.l   %d6, %d0            /* 1: Flip data bit */
    eor.l   %d6, %d1
1:
    move.l  %d1, (%a0)          /* Output new state and set CLK = 0*/
    bra.w   .wr_bit7


    /* Output 16 bits to the LCD. Instruction order is devised to maximize the
     * delay between changing the data line and the CLK L->H transition, which
     * makes the LCD controller sample DATA.
     * Requires CLK = 1 on entry.
     *
     * Custom calling convention:
     *   %a0 - GPIO_OUT_ADDR
     *   %d3 - data word
     *   %d6 - DATA_MASK
     *   %d7 - CLOCK_MASK
     * Clobbers:
     *   %d0..%d3 
     */
.write_word:
    move.w  %sr, %d2
    move.w  #0x2700, %sr

    move.l  (%a0), %d0          /* Get current state of data port */
    move.l  %d0, %d1
    and.l   %d6, %d1            /* Check current state of data line */
    beq.s   1f                  /*   and set it as previous-state bit */
    bset    #16, %d3
1:
    move.l  %d3, %d1            /* Compute the 'bit derivative', i.e. a value */
    lsr.l   #1, %d1             /*   with 1's where the data changes from the */
    eor.l   %d1, %d3            /*   previous state, and 0's where it doesn't */
    swap    %d3                 /* Shift data to upper word */

    move.l  %d0, %d1            /* precalculate opposite state of clock line */
    eor.l   %d7, %d1

    lsl.l   #1, %d3             /* Shift out MSB */
    bcc.s   1f
    eor.l   %d6, %d0            /* 1: Flip data bit */
    eor.l   %d6, %d1
1:
    move.l  %d1, (%a0)          /* Output new state and set CLK = 0*/

.macro bit_out
    move.l  %d0, (%a0)          /* Set CLK = 1 */
    lsl.l   #1, %d3
    bcc.s   1f
    eor.l   %d6, %d0
    eor.l   %d6, %d1
1:
    move.l  %d1, (%a0)
.endm

    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out

    nop
.wr_bit7:
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop
    bit_out
    nop
    nop

    move.l  %d0, (%a0)          /* Set CLK = 1 */
    move.w  %d2, %sr
    rts


    /* Output 16 bits to the LCD as fast as possible. Use only at < 60MHz.
     *
     * Custom calling convention:
     *   %a0 - GPIO_OUT_ADDR
     *   %d3 - data word
     *   %d6 - DATA_MASK
     *   %d7 - CLOCK_MASK
     * Clobbers:
     *   %d0..%d3 
     */
.write_word_fast:
    move.w  %sr, %d2            /* Get current interrupt level */
    move.w  #0x2700, %sr        /* Disable interrupts */

    move.l  (%a0), %d0          /* Get current state of data port */
    move.l  %d0, %d1
    and.l   %d6, %d1            /* Check current state of data line */
    beq.s   1f                  /*   and set it as previous-state bit */
    bset    #16, %d3
1:
    move.l  %d3, %d1            /* Compute the 'bit derivative', i.e. a value */
    lsr.l   #1, %d1             /*   with 1's where the data changes from the */
    eor.l   %d1, %d3            /*   previous state, and 0's where it doesn't */
    swap    %d3                 /* Shift data to upper byte */

    move.l  %d0, %d1            /* precalculate opposite state of clock line */
    eor.l   %d7, %d1
    
.macro bit_out_fast
    lsl.l   #1,%d3              /* Shift out MSB */
    bcc.s   1f                  
    eor.l   %d6, %d0            /* 1: Flip data bit */
    eor.l   %d6, %d1            /*   for both clock states */
1:
    move.l  %d1, (%a0)          /* Output new state and set CLK = 0*/
    move.l  %d0, (%a0)          /* set CLK = 1 */
.endm

    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast

    move.w  %d2, %sr            /* Restore interrupt level */
    rts


    .global lcd_remote_write_command
    .type   lcd_remote_write_command, @function

lcd_remote_write_command:
    lea.l   (-4*4, %sp), %sp
    movem.l %d2-%d3/%d6-%d7, (%sp)
    
    move.l  (4*4+4, %sp), %d3   /* cmd */

    lea.l   GPIO_OUT_ADDR, %a0
    lea.l   GPIO1_OUT_ADDR, %a1
    move.l  #DATA_MASK, %d6
    move.l  #CLOCK_MASK, %d7

    move.l  #~RS_MASK, %d0
    and.l   %d0, (%a0)
    moveq.l #~CS_MASK, %d0
    and.l   %d0, (%a1)

    bsr.w   .write_byte

    moveq.l #CS_MASK, %d0
    or.l    %d0, (%a1)
    
    movem.l (%sp), %d2-%d3/%d6-%d7
    lea.l   (4*4, %sp), %sp
    rts


    .global lcd_remote_write_command_ex
    .type   lcd_remote_write_command_ex, @function

lcd_remote_write_command_ex:
    lea.l   (-4*4, %sp), %sp
    movem.l %d2-%d3/%d6-%d7, (%sp)
    
    movem.l (4*4+4, %sp), %d2-%d3   /* cmd, data */

    lea.l   GPIO_OUT_ADDR, %a0
    lea.l   GPIO1_OUT_ADDR, %a1
    move.l  #DATA_MASK, %d6
    move.l  #CLOCK_MASK, %d7

    move.l  #~RS_MASK, %d0
    and.l   %d0, (%a0)
    moveq.l #~CS_MASK, %d0
    and.l   %d0, (%a1)

    lsl.l   #8, %d2
    or.l    %d2, %d3
    bsr.w   .write_word

    moveq.l #CS_MASK, %d0
    or.l    %d0, (%a1)
    
    movem.l (%sp), %d2-%d3/%d6-%d7
    lea.l   (4*4, %sp), %sp
    rts


    .global lcd_remote_write_data
    .type   lcd_remote_write_data, @function

lcd_remote_write_data:
    lea.l   (-7*4, %sp), %sp
    movem.l %d2-%d4/%d6-%d7/%a2-%a3, (%sp)
    
    move.l  (7*4+4, %sp), %a2   /* p_words */
    move.l  (7*4+8, %sp), %d4   /* count */

    lea.l   GPIO_OUT_ADDR, %a0
    lea.l   GPIO1_OUT_ADDR, %a1
    move.l  #DATA_MASK, %d6
    move.l  #CLOCK_MASK, %d7

    lea.l   .write_word, %a3
    move.l  cpu_frequency, %d0
    cmp.l   #60000000, %d0
    bhi.b   1f
    lea.l   .write_word_fast, %a3
1:

    move.l  #RS_MASK, %d0
    or.l    %d0, (%a0)
    moveq.l #~CS_MASK, %d0
    and.l   %d0, (%a1)

.wd_loop:                
    clr.l   %d3
    move.w  (%a2)+, %d3
    jsr     (%a3)
    subq.l  #1, %d4
    bne.s   .wd_loop

    moveq.l #CS_MASK, %d0
    or.l    %d0, (%a1)
    
    movem.l (%sp), %d2-%d4/%d6-%d7/%a2-%a3
    lea.l   (7*4, %sp), %sp
    rts
