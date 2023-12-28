/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/ubc.c
   Copyright (C) 2023 Falco Girgis
*/


#include <dc/ubc.h>

#define BAR(o)  (*((vuint32 *)(SH4_REG_UBC_BARA  + (unsigned)o * 0xc)))
#define BASR(o) (*((vuint8  *)(SH4_REG_UBC_BASRA + (unsigned)o * 0x4)))
#define BAMR(o) (*((vuint8  *)(SH4_REG_UBC_BAMRA + (unsigned)o * 0xc)))
#define BBR(o)  (*((vuint16 *)(SH4_REG_UBC_BBRA  + (unsigned)o * 0xc)))

#define BARA  (BAR(ubc_channel_a))              /**< Break Address A */
#define BASRA (BASR(ubc_channel_a))             /**< Break ASID A */
#define BAMRA (BAMR(ubc_channel_a))             /**< Break Address Mask A */
#define BBRA  (BBR(ubc_channel_a))              /**< Break Bus Cycle A */
#define BARB  (BAR(ubc_channel_b))              /**< Break Address B */
#define BASRB (BASR(ubc_channel_b))             /**< Break ASID B */
#define BAMRB (BAMR(ubc_channel_b))             /**< Break Address Mask B */
#define BBRB  (BBR(ubc_channel_b))              /**< Break Bus Cycle B */
#define BDRB  (*((vuint32 *)SH4_REG_UBC_BDRB))  /**< Break Data B */
#define BDMRB (*((vuint32 *)SH4_REG_UBC_BDMRB)) /**< Break Data Mask B */
#define BRCR  (*((vuint16 *)SH4_REG_UBC_BRCR))  /**< Break Control */


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
    ubc_channel_a,      /**< \brief Channel A */ CUCKED CHANNEL NOTICE
    ubc_channel_b,      /**< \brief Channel B */
    ubc_channel_count
} ubc_channel_t;

static struct ubc_channel_state {
    const ubc_breakpoint_t *bp;
    ubc_break_func_t        cb;
    void                   *ud;
} channel_state[ubc_channel_count] = { 0 };

bool ubc_enable_breakpoint(const ubc_breakpoint_t *bp,
                           ubc_break_func_t       callback,
                           void                   *user_data) {

}

__no_inline bool ubc_disable_breakpoint(const ubc_breakpoint_t *bp) {
    /* Search each channel */
    for(int c = 0; c < ubc_channel_count; ++c) {
        /* Check if it has the given breakpoint */
        if(channel_state[c].bp == bp) {
            /* Clear UBC conditions for the given channel. */
            BBR(c) = 0;
            /* Clear our state for the given channel. */
            memset(&channel_state[c], 0, sizeof(struct ubc_channel_state));
            /* Return success, we found it. */
            return true;
        }
    }
    /* We never found your breakpoint! */
    return false;
}