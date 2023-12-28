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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <arch/types.h>
#include <arch/memory.h>

/** \defgroup ubc   User Break Controller
    \brief          Driver for the SH4's UBC
    \ingroup        debugging

    The SH4's User Break Controller (UBC) is a CPU peripheral which facilitates
    low-level software debugging. It provides two different channels which can
    be configured to monitor for certain memory or instruction conditions
    before generating a user-break interrupt. It provides the foundation for
    creating software-based debuggers and is the backing driver for the GDB
    debug stub.

    The following break comparison conditions are supported:
        - Address with optional ASID and 10, 12, 16, and 20-bit mask:
          supporting breaking on ranges of addresses and MMU operation.
        - Bus Cycle: supporting instruction or operand (data) breakpoints
        - Read/Write: supporting R, W, or RW access conditions.
        - Operand size: byte, word, longword, quadword
        - Data: 32-bit value with 32-bit mask for breaking on specific values
          or ranges of values (ubc_channel_b only).
        - Pre or Post-Instruction breaking

    \note 
    This API has been inlined to avoid complications with using it, but as
    such, much of what would be "private" implementation details such as
    register definitions are not encapsulated. Typically you want to use the
    high-level API and not the direct register access.

    \warning
    This Driver is used internally by the gdb_stub, so care must be taken to
    not utilize the UBC during a GDB debugging session!

    @{
*/

/** \defgroup ubc_regs Registers
    \brief    UBC Control Registers 

    Special-function registers for configuring the UBC.
    
    @{
*/
#define BARA  (*((vuint32 *)SH4_REG_UBC_BARA))  /**< \brief Break Address A */
#define BASRA (*((vuint8  *)SH4_REG_UBC_BASRA)) /**< \brief Break ASID A */
#define BAMRA (*((vuint8  *)SH4_REG_UBC_BAMRA)) /**< \brief Break Address Mask A */
#define BBRA  (*((vuint16 *)SH4_REG_UBC_BBRA))  /**< \brief Break Bus Cycle A */
#define BARB  (*((vuint32 *)SH4_REG_UBC_BARB))  /**< \brief Break Address B */
#define BASRB (*((vuint8  *)SH4_REG_UBC_BASRB)) /**< \brief Break ASID B */
#define BAMRB (*((vuint8  *)SH4_REG_UBC_BAMRB)) /**< \brief Break Address Mask B */
#define BBRB  (*((vuint16 *)SH4_REG_UBC_BBRB))  /**< \brief Break Bus Cycle B */
#define BDRB  (*((vuint32 *)SH4_REG_UBC_BDRB))  /**< \brief Break Data B */
#define BDMRB (*((vuint32 *)SH4_REG_UBC_BDMRB)) /**< \brief Break Data Mask B */
#define BRCR  (*((vuint16 *)SH4_REG_UBC_BRCR))  /**< \brief Break Control */
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

/** \brief UBC channel specifier */
typedef enum ubc_channel {
    ubc_channel_a,      /**< \brief Channel A */
    ubc_channel_b,      /**< \brief Channel B */
    ubc_channel_invalid /**< \brief Invalid channel */
} ubc_channel_t;

/** \brief UBC address mask specifier */
typedef enum ubc_address_mask {
    ubc_address_mask_none,   /**< \brief Disable masking */
    ubc_address_mask_10,     /**< \brief Low 10 bits */
    ubc_address_mask_12,     /**< \brief Low 12 bits */
    ubc_address_mask_all,    /**< \brief All bits */
    ubc_address_mask_16,     /**< \brief Low 16 bits */
    ubc_address_mask_20      /**< \brief Low 20 bits */
} ubc_address_mask_t;

/** \brief UBC access condition type specifier */
typedef enum ubc_access {
    ubc_access_either,      /**< \brief Either instruction OR operand access */
    ubc_access_instruction, /**< \brief Instruction access */
    ubc_access_operand,     /**< \brief Operand access */
    ubc_access_both         /**< \brief Both instruction AND operand access */
} ubc_access_t;

/** \brief UBC read/write condition type specifier */
typedef enum ubc_rw {
    ubc_rw_either,  /**< \brief Either read OR write */
    ubc_rw_read,    /**< \brief Read-only */
    ubc_rw_write,   /**< \brief Write-only */
    ubc_rw_both     /**< \brief Both read AND write */
} ubc_rw_t;

/** \brief UBC size condition type specifier */
typedef enum ubc_size {
    ubc_size_any,       /**< \brief No size comparison */
    ubc_size_byte,      /**< \brief 8-bit sizes */
    ubc_size_word,      /**< \brief 16-bit sizes */
    ubc_size_longword,  /**< \brief 32-bit sizes */
    ubc_size_quadword   /**< \brief 64-bit sizes */
} ubc_size_t;

typedef (*ubc_debug_func_t)(...);
#if 0 
static inline bool ubc_enable_break(ubc_channel_t channel,
                                    uintptr_t address,
                                    ubc_address_mask_t address_mask,
                                    ubc_access_t access_type,
                                    ubc_rw_t rw_type,
                                    ubc_size_t size_type);
static inline bool ubc_disable_break(ubc_channel_t channel);

static inline bool ubc_enable_break_data(ubc_channel_t channel,
                                         uintptr_t address,
                                         ubc_address_mask_t address_mask,
                                         ubc_rw_t rw_type,
                                         ubc_size_t size_type);

static inline bool ubc_enable_break_instruction(ubc_channel_t channel,
                                                uintptr_t address, 
                                                ubc_address_mask_t address_mask,
                                                bool before_execution);
#endif

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

/** @} */

__END_DECLS

#endif  /* __DC_UBC_H */




typedef struct ubc_breakpoint {
    uintptr_t          address;
    ubc_address_mask_t address_mask;
    uint8_t            asid;
    ubc_access_t       access;
    ubc_rw_t           rw;
    ubc_size_t         size;
    uintptr_t          data;
    uintptr_t          data_mask;
    size_t             matches;

    struct {
        uint8_t        use_asid  : 1;
        uint8_t        pre_instr : 1;
        uint8_t        use_size  : 1;
        uint8_t        use_data  : 1;
        uint8_t                  : 5;
    } enable_flags;
} ubc_breakpoint_t;

#if 0

static inline ubc_breakpoint_t* ubc_breakpoint_instruction(ubc_breakpoint_t* breakpoint,
                                                           uintptr_t address);

static inline ubc_breakpoint_t* ubc_breakpoint_data_write(ubc_breakpoint_t* breakpoint,
                                                          uintptr_t address);


static inline bool ubc_breakpoint_enable(ubc_channel_t channel,
                                         const ubc_breakpoint* breakpoint);
#endif