/* KallistiOS ##version##

   arch/dreamcast/include/arch/byteorder.h
   Copyright (C) 2015 Lawrence Sebald
   Copyright (C) 2023 Donald Haase

*/

/** \file   arch/byteorder.h
    \brief  Byte-order related macros.

    This file contains architecture-specific byte-order related macros and/or
    functions. Each platform should define six macros/functions in this file:
    arch_swap16, arch_swap32, arch_ntohs, arch_ntohl, arch_htons, and
    arch_htonl. The first two of these swap the byte order of 16-bit and 32-bit
    integers, respectively. The other four macros will be used by the kernel to
    implement the network-related byte order functions.

    \author Lawrence Sebald
*/

#ifndef __ARCH_BYTEORDER_H
#define __ARCH_BYTEORDER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/_types.h>

#ifdef BYTE_ORDER
/* If we've included <arch/types.h>, this might already be defined... */
#undef BYTE_ORDER
#endif

/** \brief  Define the byte-order of the platform in use. */
#define BYTE_ORDER      LITTLE_ENDIAN

/** \brief  Swap the byte order of a 16-bit integer.

    This macro swaps the byte order of a 16-bit integer in an architecture-
    defined manner.

    \param  x           The value to be byte-swapped. This should be a uint16,
                        or equivalent.
    \return             The swapped value.
*/
#define arch_swap16(x) ((((x) & 0xff00)>>8) | (((x) & 0x00ff)<<8))

/** \brief  Swap the byte order of a 32-bit integer.

    This macro swaps the byte order of a 32-bit integer in an architecture-
    defined manner.

    \param  x           The value to be byte-swapped. This should be a uint32,
                        or equivalent.
    \return             The swapped value.
*/
#define arch_swap32(x) ((((x) & 0xff000000)>>24) | (((x) & 0x00ff0000)>>16) | \
                        (((x) & 0x0000ff00)<<16) | (((x) & 0x000000ff)<<24))

/** \brief  Convert network-to-host short.

    This macro converts a network byte order (big endian) value to the host's
    native byte order. On a little endian system (like the Dreamcast), this
    should just call arch_swap16(). On a big endian system, this should be a
    no-op.

    \param  x           The value to be converted. This should be a uint16,
                        or equivalent.
    \return             The converted value.
*/
#define arch_ntohs(x) arch_swap16(x)

/** \brief  Convert network-to-host long.

    This macro converts a network byte order (big endian) value to the host's
    native byte order. On a little endian system (like the Dreamcast), this
    should just call arch_swap32(). On a big endian system, this should be a
    no-op.

    \param  x           The value to be converted. This should be a uint32,
                        or equivalent.
    \return             The converted value.
*/
#define arch_ntohl(x) arch_swap32(x)

/** \brief  Convert host-to-network short.

    This macro converts a value in the host's native byte order to network byte
    order (big endian). On a little endian system (like the Dreamcast), this
    should just call arch_swap16(). On a big endian system, this should be a
    no-op.

    \param  x           The value to be converted. This should be a uint16,
                        or equivalent.
    \return             The converted value.
*/
#define arch_htons(x) arch_swap16(x)

/** \brief  Convert host-to-network long.

    This macro converts a value in the host's native byte order to network byte
    order (big endian). On a little endian system (like the Dreamcast), this
    should just call arch_swap32(). On a big endian system, this should be a
    no-op.

    \param  x           The value to be converted. This should be a uint32,
                        or equivalent.
    \return             The converted value.
*/
#define arch_htonl(x) arch_swap32(x)

__END_DECLS

#endif /* !__ARCH_BYTEORDER_H */
