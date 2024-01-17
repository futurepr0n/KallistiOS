/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/ubc.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Falco Girgis
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

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stdint.h>

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

    \warning
    This Driver is used internally by the gdb_stub, so care must be taken to
    not utilize the UBC during a GDB debugging session!

    @{
*/

/** \cond Forward declarations*/
struct irq_context;
/** \endcond */

#define UBC_BRK() __asm(    \
    /* "brk\n" */           \
    ".word 0x003B\n"        \
    "nop\n"                 \
) /* needs to be a constant, not known to the assembler */

/** \brief UBC address mask specifier

*/
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
    ubc_access_either,      /**< \brief Either Instruction or Operand */
    ubc_access_instruction, /**< \brief Instruction */
    ubc_access_operand      /**< \brief Operand */
} ubc_access_t;

/** \brief UBC read/write condition type specifier */
typedef enum ubc_rw {
    ubc_rw_either,  /**< \brief Either Read or Write */
    ubc_rw_read,    /**< \brief Read-only */
    ubc_rw_write    /**< \brief Write-only */
} ubc_rw_t;

/** \brief UBC size condition type specifier */
typedef enum ubc_size {
    ubc_size_any,       /**< \brief No size comparison */
    ubc_size_byte,      /**< \brief 8-bit sizes */
    ubc_size_word,      /**< \brief 16-bit sizes */
    ubc_size_longword,  /**< \brief 32-bit sizes */
    ubc_size_quadword   /**< \brief 64-bit sizes */
} ubc_size_t;

/** \brief UBC breakpoint structure

    This structure contains all of the information needed to configure a
    breakpoint using the SH4's UBC. It is meant to be zero-initialized,
    with the most commonly preferred, general values being the defaults,
    so that the only member that must be initialized to a non-zero value is
    ubc_breakpoint_t::address.

    \remark
    The default configuration will trigger a breakpoint with read, write, or PC
    access to ubc_breakpoint_t::address.

    \warning
    When using ubc_breakpoint_t::asid or ubc_breakpoint_t::data, do not forget
    to set their respective `enable` members!
*/
typedef struct ubc_breakpoint {
    void *address;

    struct { 
        ubc_address_mask_t address_mask;
        ubc_access_t       access;
        ubc_rw_t           rw;
        ubc_size_t         size;    
    } cond;

    struct { 
        bool    enabled;
        uint8_t value;
    } asid;

    struct {
        bool     enabled;
        uint32_t value;
        uint32_t mask;
    } data;

    struct {
        bool break_after;
    } instr;

    struct ubc_breakpoint *next;
} ubc_breakpoint_t;

/** \brief UBC breakpoint user callback */
typedef bool (*ubc_break_func_t)(const ubc_breakpoint_t   *bp, 
                                 const struct irq_context *ctx, 
                                 void                     *user_data);

bool __no_inline ubc_enable_breakpoint(const ubc_breakpoint_t *bp,
                                       ubc_break_func_t       callback,
                                       void                   *user_data);

bool __no_inline ubc_disable_breakpoint(const ubc_breakpoint_t *bp);


void ubc_set_break_handler(ubc_break_func_t callback,
                           void             *user_data);

void ubc_break(void);

void __no_inline ubc_init(void);

void __no_inline ubc_shutdown(void);

#if 0
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
#endif

/** @} */

__END_DECLS

#endif  /* __DC_UBC_H */
