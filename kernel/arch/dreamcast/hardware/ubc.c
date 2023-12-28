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
#define BASM  (1 << 2)  /* Use ASID (0) or not (1) */
#define BAM   (0xb)     /* Which bits of BARA/BARB get used */

// BBRA/BBRB
#define ID_BIT 4
#define ID     (3 << ID_BIT) // Instruction vs Data (or Either or Neither?)
#define RW_BIT 2
#define RW     (3 << RW_BIT) // Access Type (Read, Write, Both, Neither?) 
#define SZ_BIT_HIGH 6
#define SZ     (0x43)   // Data size (Not used, byte, word, longword, quadword)

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
    ubc_channel_count
} ubc_channel_t;

static struct ubc_channel_state {
    const ubc_breakpoint_t *bp;
    ubc_break_func_t        cb;
    void                   *ud;
} channel_state[ubc_channel_count] = { 0 };

static void enable_breakpoint(ubc_channel_t           ch,
                              const ubc_breakpoint_t *bp,
                              ubc_break_func_t        cb,
                              void                   *ud) {
    /* Set state variables. */
    channel_state[ch].bp = bp;
    channel_state[ch].cb = cb;
    channel_state[ch].ub = ub;

    /* Configure registers. */
    BAR(ch) = bp->address;

    // SET ADDRESS MASK
    BBR(ch) |= bp->cond.access << ID_BIT; /* Set Access Type */
    BBR(ch) |= bp->cond.rw << RW_BIT;
    BBR(ch) |= /* Set size */

    if(bp->asid.enabled) {
        // set ASID
        BASR(ch) &= ~BASM;
    }
    else {
        BASR(ch) |= BASM;
    }

    if(bp->data.enabled) {
        BRCR |= DBEB;
        BDRB = bp->data.value;
        BDMRB = bp->data.mask;
    }
    else {
        BRCR &= ~DBEB;
    }

    if(bp->instr.break_after) 
        BRCR |= (ch == ubc_channel_a)? PCBA : PBCBB;
    else 
        BRCR &= (ch == ubc_channel_a)? ~PCBA : ~PCBB;
}

__no_inline bool ubc_enable_breakpoint(const ubc_breakpoint_t *bp,
                                       ubc_break_func_t       callback,
                                       void                   *user_data) {

    /* Check if we're dealing with a combined sequential breakpoint */
    if(bp->next) {
        /* Basic sanity checks for debug builds */
        assert(!bp->next->next);   /* Can only chain two */
        assert(!bp->data.enabled); /* Channel B only for data */

        /* Ensure we have both channels free */
        if(channel_state[ubc_channel_a].bp || channel_state[ubc_channel_b].bp)
            return false;

        enable_breakpoint(ubc_channel_a, bp, callback, user_data);
        enable_breakpoint(ubc_channel_b, bp->next, callback, user_data);

        BRCR |= SEQ;
    }
    /* Handle single-channel */ 
    else {
        /* Check if we require channel B */
        if(bp->data.enabled) {
            /* Check if the channel is free */
            if(channel_state[ubc_channel_b].bp)
                return false;

            enable_breakpoint(ubc_channel_b, bp, callback, user_data);

        /* We can take either channel */ 
        } 
        else {
            /* Take whichever channel is free. */
            if(!channel_state[ubc_channel_a].bp)
                enable_breakpoint(ubc_channel_a, bp, callback, user_data);
            else if(!channel_state[ubc_channel_b].bp)
                enable_breakpoint(ubc_channel_b, bp, callback, user_data);
            else
                return false;
        }

        /* Configure both channels to run independently */
        BRCR &= ~SEQ;
    }

    return true;
}

static void disable_breakpoint(ubc_channel_t ch) {
    /* Clear UBC conditions for the given channel. */
    BBR(ch) = 0;
    /* Clear our state for the given channel. */
    memset(&channel_state[ch], 0, sizeof(struct ubc_channel_state));
}

// Need to handle multi-breakpoint
__no_inline bool ubc_disable_breakpoint(const ubc_breakpoint_t *bp) {
    /* Disabling a sequential breakpoint pair */
    if(bp->next) {
        if(channel_state[ubc_channel_a].bp == bp &&
           channel_state[ubc_channel_b].bp == bp->next) {
            /* Clear both channels */
            disable_breakpoint(ubc_channel_a);
            disable_breakpoint(ubc_channel_b);
            /* Return success, we found it. */
            return true;
        }
    }
    /* Disable single, non-sequential breakpoint */
    else {
        /* Search each channel */
        for(unsigned ch = 0; ch < ubc_channel_count; ++ch) {
            /* Check if it has the given breakpoint */
            if(channel_state[ch].bp == bp) {
                disable_breakpoint(ch);
                /* Return success, we found it. */
                return true;
            }
        }
    }

    /* We never found your breakpoint! */
    return false;
}