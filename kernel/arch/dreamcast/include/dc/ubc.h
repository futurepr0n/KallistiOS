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
#define BARB    (*((vuint32 *)0xff20000c))   /**< \brief Break Address B */
#define BASRB   (*((vuint8  *)0xff000018))   /**< \brief Break ASID */
#define BAMRB   (*((vuint8  *)0xff200010))   /**< \brief Break Address Mask */
#define BBRB    (*((vuint16 *)0xff200014))   /**< \brief Break Bus Cycle */
#define BDRB    (*((vuint32 *)0xff200018))   /**< \brief Break Data */
#define BDMRB   (*((vuint32 *)0xff20001c))   /**< \brief Break Data Mask */
#define BRCR    (*((vuint16 *)0xff200020))   /**< \brief Break Control */
/** @} */

// BASRA/BASRB
#define BASMA  (1 << 2)  /* Use ASID (0) or not (1) */
#define BAMA   (0xb)     /* Which bits of BARA/BARB get used */

// BBRA/BBRB
#define IDA     (3 << 4) // Instruction vs Data (or Either or Neither?)
#define RWA     (3 << 2) // Access Type (Read, Write, Both, Neither?) 
#define SZA     (0x43)   // Data size (Not used, byte, word, longword, quadword)

// BRCR
#define CMFA    (1 << 15) // Set when break condition A is met (not cleared)
#define CMFB    (1 << 14) // Set when break condition B is met (not cleared)
#define PCBA    (1 << 10) // Instruction break condition A is before or after instruction execution
#define DBEB    (1 << 7)  // Include Data Bus condition for channel B
#define PCBB    (1 << 6)  // Instruction break condition B is before or after instruction execution
#define SEQ     (1 << 3)  // A, B are independent (0) or sequential (1)
#define UBDE    (1 << 0)  // Use debug function in DBR register

typedef enum ubc_channel {
    ubc_channel_a,
    ubc_channel_b,
    ubc_channel_invalid
} ubc_channel_t;

typedef enum ubc_address_mask {
    ubc_address_mask_disable,
    ubc_address_mask_10,
    ubc_address_mask_12,
    ubc_address_mask_all,
    ubc_address_mask_16,
    ubc_address_mask_20
} ubc_address_mask_t;

typedef enum ubc_cond_access {
    ubc_cond_access_disable,
    ubc_cond_access_instruction,
    ubc_cond_access_operand,
    ubc_cond_access_both
} ubc_cond_access_t;

typedef enum ubc_cond_rw {
    ubc_cond_rw_disable,
    ubc_cond_rw_read,
    ubc_cond_rw_write,
    ubc_cond_rw_both
} ubc_cond_rw_t;

typedef enum ubc_cond_size {
    ubc_cond_size_disable,
    ubc_cond_size_byte,
    ubc_cond_size_word,
    ubc_cond_size_longword,
    ubc_cond_size_quadword
} ubc_cond_size_t;

typedef struct ubc_breakpoint {
    uintptr_t          address;
    ubc_address_mask_t address_mask;
    uint8_t            asid;
    bool               use_asid;
    ubc_cond_access_t  access_cond;
    ubc_cond_rw_t      rw_cond;
    ubc_cond_size_t    size_cond;
    bool               post_instruction;
    uintptr_t          data;
    uintptr_t          data_mask;
    bool               use_data;
} ubc_breakpoint_t;

static inline ubc_breakpoint_t* ubc_breakpoint_instruction(ubc_breakpoint_t* breakpoint,
                                                           uintptr_t address);

static inline ubc_breakpoint_t* ubc_breakpoint_data_write(ubc_breakpoint_t* breakpoint,
                                                          uintptr_t address);

ubc_breakpoint_t bp;
ubc_enable_breakpoint(ubc_available_channel(),
                      ubc_breakpoint_data_write(&bp, 0xdeadbeef));


static inline bool ubc_breakpoint_enable(ubc_channel_t channel,
                                         const ubc_breakpoint* breakpoint);



typedef (*ubc_debug_func_t)(...);



static inline bool ubc_enable_break_cond(ubc_channel_t channel,
                                         uintptr_t address,
                                         ubc_address_mask_t address_mask,
                                         ubc_cond_access_t access_type,
                                         ubc_cond_rw_t rw_type,
                                         ubc_cond_size_t size_type);
static inline bool ubc_disable_break(ubc_channel_t channel);

static inline bool ubc_enable_break_data(ubc_channel_t channel,
                                         uintptr_t address,
                                         ubc_address_mask_t address_mask,
                                         ubc_cond_rw_t rw_type,
                                         ubc_cond_size_t size_type);

static inline bool ubc_enable_break_instruction(ubc_channel_t channel,
                                                uintptr_t address, 
                                                ubc_address_mask_t address_mask,
                                                bool before_execution);

static inline bool ubc_set_break_asid(ubc_channel_t channel, uint8_t asid);
static inline bool ubc_set_break_data(uintptr_t data, uintptr_t mask);  // requires operand size specified first
static inline void ubc_set_break_sequential(bool enabled);

static void ubc_set_debug_function(ubc_debug_func_t *callback);

static bool ubc_break_active(ubc_channel_t channel);
static bool ubc_break_clear(ubc_channel_t channel);


static inline bool          ubc_breakpoint_set(ubc_channel_t channel, 
                                               uint8_t       asid, 
                                               uintptr_t     address, 
                                               uint8_t       address_mask,
                                               uint16_t      bus_cycle,
                                               uintptr_t     data,
                                               uintptr_t     data_mask,
                                               uint8_t       control);


static inline void          ubc_delay(void) {}

static inline void          ubc_init(void) {}
static inline void          ubc_shutdown(void) {}

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
    BAMRA = 4;      /* Disable the ASID */
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
    BAMRA = 4;      /* Disable the ASID */

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

