/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/ubc.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2023 Falco Girgis
*/

/** \file    dc/ubc.h
    \brief   User Break Controller Driver 
    \ingroup ubc

    This file provides a driver and API around the SH4's UBC. 

    \sa arch/gdb.h

    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __DC_UBC_H
#define __DC_UBC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <arch/types.h>

/** \defgroup ubc   User Break Controller (UBC)

    \note 
    This API has been inlined to avoid complications with using it.
*/

/** \defgroup ubc_regs  Registers
    \brief    UBC Control Registers 
    \ingroup  ubc

    Special-function registers for configuring the UBC.
    
    @{
*/
#define BARA    (*((vuint32 *)0xff200000))   /**< \brief Break Address A */
#define BASRA   (*((vuint8  *)0xff000014))   /**< \brief Break ASID */
#define BAMRA   (*((vuint8  *)0xff200004))   /**< \brief Break Address Mask */
#define BBRA    (*((vuint16 *)0xff200008))   /**< \brief Break Bus Cycle */
#define BARB    (*((vuint32 *)0xff20000c))   /**< \brief Break Address */
#define BASRB   (*((vuint8  *)0xff000018))   /**< \brief Break ASID */
#define BAMRB   (*((vuint8  *)0xff200010))   /**< \brief Break Address Mask */
#define BBRB    (*((vuint16 *)0xff200014))   /**< \brief Break Bus Cycle */
#define BDRB    (*((vuint32 *)0xff200018))   /**< \brief Break Data */
#define BDMRB   (*((vuint16 *)0xff20001c))   /**< \brief Break Data Mask */
#define BRCR    (*((vuint16 *)0xff200020))   /**< \brief Break Control */
/** @} */


#define BASMA  (1 << 2)  /* Specifies whether all bits of the channel A break ASID (BASA7–BASA0) are to be masked. */
#define BAMA   (0xb)     /* */

#define IDA     (3 << 4)
#define RWA     (3 << 2)
#define SZA     (0x43)

/** \brief  Pause after setting UBC parameters. 
 
    Required delay after changing the UBC's configuration.

 */
static inline void ubc_pause(void) {
    __asm__ __volatile__("nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n");
}

/** \brief Disable all UBC breakpoints. */
static inline void ubc_disable_all(void) {
    BBRA = 0;
    BBRB = 0;
    ubc_pause();
}

/** \brief  Set a UBC data-write breakpoint at the given address.
    \param  address         The address to set the breakpoint at
*/
static inline void ubc_break_data_write(uintptr_t address) {
    BASRA = 0;      /* ASID = 0 */
    BARA = address; /* Break address */
    BAMRA = 4;      /* Mask the ASID */
    BRCR = 0;       /* Nothing special, clear all flags */
    BBRA = 0x28;    /* Operand write cycle, no size constraint */
    ubc_pause();
}

/** \brief  Set a UBC instruction access breakpoint at the given address.
    \param  address         The address to set the breakpoint at.
    \param  use_dbr         Use the DBR register as the base for the exception.
*/
static inline void ubc_break_inst(uintptr_t address, int use_dbr) {
    BASRA = 0;      /* ASID = 0 */
    BARA = address; /* Break address */
    BAMRA = 4;      /* Mask the ASID */

    if(use_dbr) {
        BRCR = 1;   /* Use the DBR as the base for the IRQ */
    }
    else {
        BRCR = 0;
    }

    BBRA = 0x1C;    /* Instruction cycle, no size constraint */
    ubc_pause();
}

/* More to come.... */

__END_DECLS

#endif  /* __DC_UBC_H */

