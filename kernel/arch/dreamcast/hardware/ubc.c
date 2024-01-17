/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/ubc.c
   Copyright (C) 2024 Falco Girgis
*/

#include <dc/ubc.h>

#include <arch/types.h>
#include <arch/memory.h>
#include <arch/irq.h>

#include <kos/dbglog.h>

#include <string.h>
#include <assert.h>

#include <stdio.h>

#define BAR(o)  (*((vuint32 *)(uintptr_t)(SH4_REG_UBC_BARA  + (unsigned)o * 0xc)))
#define BASR(o) (*((vuint8  *)(uintptr_t)(SH4_REG_UBC_BASRA + (unsigned)o * 0x4)))
#define BAMR(o) (*((vuint8  *)(uintptr_t)(SH4_REG_UBC_BAMRA + (unsigned)o * 0xc)))
#define BBR(o)  (*((vuint16 *)(uintptr_t)(SH4_REG_UBC_BBRA  + (unsigned)o * 0xc)))

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

// BASR
#define BASM            (1 << 2)  /* Use ASID (0) or not (1) */
#define BAM_BIT_HIGH    3
#define BAM_BITS        3
#define BAM_HIGH        (1 << BAM_BIT_HIGH)
#define BAM_LOW         0x3
#define BAM             (BAM_HIGH | BAM_LOW)     /* Which bits of BARA/BARB get used */

// BBR
#define ID_BIT          4
#define ID              (3 << ID_BIT) // Instruction vs Data (or Either or Neither?)
#define RW_BIT          2
#define RW              (3 << RW_BIT) // Access Type (Read, Write, Both, Neither?) 
#define SZ_BIT_HIGH     6
#define SZ_BITS         3
#define SZ_HIGH         (1 << SZ_BIT_HIGH)
#define SZ_LOW          0x3
#define SZ              (SZ_HIGH | SZ_LOW)   // Data size (Not used, byte, word, longword, quadword)

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
    ubc_channel_count
} ubc_channel_t;

static struct ubc_channel_state {
    const ubc_breakpoint_t *bp;
    ubc_break_func_t        cb;
    void                   *ud;
} channel_state[ubc_channel_count] = { 0 };

static ubc_break_func_t break_cb = NULL;
static void *break_ud = NULL;

extern uintptr_t dbr_get(void);
extern void dbr_set(uintptr_t address);

static void disable_breakpoint(ubc_channel_t ch) {
    /* Clear our state for the given channel. */
    memset(&channel_state[ch], 0, sizeof(struct ubc_channel_state));
    /* Clear UBC conditions for the given channel. */
    BBR(ch) = 0;
    BAMR(ch) = 0;
    BASR(ch) = 0;
    BAR(ch) = 0;
}

static void dbr_handler(int evt) {

    bool serviced = false;
    irq_context_t *irq_ctx = NULL;

    if(BRCR & CMFA) {
        bool disable = false;

        if(channel_state[ubc_channel_a].cb)
            disable = channel_state[ubc_channel_a].cb(channel_state[ubc_channel_a].bp,
                                                      irq_ctx,
                                                      channel_state[ubc_channel_a].ud);
        if(disable) {
            disable_breakpoint(ubc_channel_a);
            if(BRCR & SEQ)
                disable_breakpoint(ubc_channel_b);
        }

        BRCR &= ~CMFA;

        serviced = true;
    }

    if(BRCR & CMFB) {
        if(!(BRCR & SEQ))
            if(channel_state[ubc_channel_b].cb)
                if(channel_state[ubc_channel_b].cb(channel_state[ubc_channel_b].bp,
                                                   irq_ctx,
                                                   channel_state[ubc_channel_b].ud))
                    disable_breakpoint(ubc_channel_b);

        BRCR &= ~CMFB;

        serviced = true;
    }

    if(!serviced) {
        if(break_cb)
            break_cb(NULL, irq_ctx, break_ud);
        else
            dbglog(DBG_CRITICAL, "Unhandled UBC break request!\n");
    }

}

static __noinline void set_dbr(uintptr_t address) {
    (void)address;
    __asm__("ldc    R4, DBR" : : );
}

static __noinline uintptr_t get_dbr(void) {
    uintptr_t dbr = 0;
    __asm__("stc    DBR, R0\n"
            "mov.l  R0, %0" : : "m"(dbr));
    return dbr;
}


inline static uint8_t get_access_mask(ubc_access_t access) {
    switch(access) {
    case ubc_access_either:
        return 0x3;
    default:
        return access;
    }
}

inline static uint8_t get_rw_mask(ubc_rw_t rw) {
    switch(rw) {
    case ubc_rw_either:
        return 0x3;
    default:
        return rw;
    }
}

static void enable_breakpoint(ubc_channel_t           ch,
                              const ubc_breakpoint_t *bp,
                              ubc_break_func_t        cb,
                              void                   *ud) {

    /* Set state variables. */
    channel_state[ch].bp = bp;
    channel_state[ch].cb = cb;
    channel_state[ch].ud = ud;

    /* Configure Registers. */

    /* Address */
    BAR(ch) = bp->address;

    /* Address mask */
    BAMR(ch) = ((bp->cond.address_mask << (BAM_BIT_HIGH - (BAM_BITS - 1))) & BAM_HIGH) |
                (bp->cond.address_mask & BAM_LOW);

    /* ASID */
    if(bp->asid.enabled) {
        /* ASID value */
        BASR(ch) = bp->asid.value;
        /* ASID enable */
        BAMR(ch) &= ~BASM;
    }
    else {
        /* ASID disable */
        BAMR(ch) |= BASM;
    }

    /* Data (channel B only) */
    if(bp->data.enabled) {
        /* Data value */
        BDRB = bp->data.value;
        /* Data mask */
        BDMRB = bp->data.mask;
        /* Data enable */
        BRCR |= DBEB;
    }
    else {
        /* Data disable */
        BRCR &= ~DBEB;
    }

    /* Instruction */
    if(bp->instr.break_after)
        /* Instruction break after */ 
        BRCR |= (ch == ubc_channel_a)? PCBA : PCBB;
    else 
        /* Instruction break before */
        BRCR &= (ch == ubc_channel_a)? (~PCBA) : (~PCBB);

    /* Set Access Type */
    BBR(ch) |= get_access_mask(bp->cond.access) << ID_BIT;
    /* Read/Write type */
    BBR(ch) |= get_rw_mask(bp->cond.rw) << RW_BIT;
    /* Size type */
    BBR(ch) |= ((bp->cond.size << (SZ_BIT_HIGH - (SZ_BITS - 1))) & SZ_HIGH) |
                (bp->cond.size & SZ_LOW);

                ubc_wait();

}

bool __no_inline ubc_enable_breakpoint(const ubc_breakpoint_t *bp,
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
        } 
        /* We can take either channel */ 
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

    ubc_wait();

    return true;
}

// Need to handle multi-breakpoint
bool __no_inline ubc_disable_breakpoint(const ubc_breakpoint_t *bp) {
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


static void handle_exception(irq_t code, irq_context_t *context) {
    dbr_handler(code);
}

void __no_inline ubc_init(void) {
    disable_breakpoint(ubc_channel_a);
    disable_breakpoint(ubc_channel_b);

#if 1
    irq_set_handler(EXC_USER_BREAK_PRE, handle_exception);
    irq_set_handler(EXC_USER_BREAK_POST, handle_exception);
#endif

    //set_dbr((uintptr_t)dbr_handler);
    //assert(get_dbr() == (uintptr_t)dbr_handler);

    //BRCR = UBDE;

    //printf("UBC INITTED!\n");
}

void __no_inline ubc_shutdown(void) {
    disable_breakpoint(ubc_channel_a);
    disable_breakpoint(ubc_channel_b);

    BRCR = 0;
}