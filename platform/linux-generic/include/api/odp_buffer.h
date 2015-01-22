/* Copyright (c) 2013, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */


/**
 * @file
 *
 * ODP buffer descriptor
 */

#ifndef ODP_BUFFER_H_
#define ODP_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <odp_std_types.h>
#include <odp_platform_types.h>

/** @defgroup odp_buffer ODP BUFFER
 *  Operations on a buffer.
 *  @{
 */


/**
 * Buffer start address
 *
 * @param buf      Buffer handle
 *
 * @return Buffer start address
 */
void *odp_buffer_addr(odp_buffer_t buf);

/**
 * Buffer maximum data size
 *
 * @param buf      Buffer handle
 *
 * @return Buffer maximum data size
 */
size_t odp_buffer_size(odp_buffer_t buf);

/**
 * Buffer type
 *
 * @param buf      Buffer handle
 *
 * @return Buffer type
 */
int odp_buffer_type(odp_buffer_t buf);

#define ODP_BUFFER_TYPE_INVALID (-1) /**< Buffer type invalid */
#define ODP_BUFFER_TYPE_ANY       0  /**< Buffer that can hold any other
					  buffer type */
#define ODP_BUFFER_TYPE_RAW       1  /**< Raw buffer, no additional metadata */
#define ODP_BUFFER_TYPE_PACKET    2  /**< Packet buffer */
#define ODP_BUFFER_TYPE_TIMEOUT   3  /**< Timeout buffer */

/**
 * Tests if buffer is valid
 *
 * @param buf      Buffer handle
 *
 * @retval 1 Buffer handle represents a valid buffer.
 * @retval 0 Buffer handle does not represent a valid buffer.
 */
int odp_buffer_is_valid(odp_buffer_t buf);

/**
 * Buffer pool of the buffer
 *
 * @param buf       Buffer handle
 *
 * @return Handle of buffer pool buffer belongs to
 */
odp_buffer_pool_t odp_buffer_pool(odp_buffer_t buf);

/**
 * Print buffer metadata to STDOUT
 *
 * @param buf      Buffer handle
 *
 */
void odp_buffer_print(odp_buffer_t buf);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
