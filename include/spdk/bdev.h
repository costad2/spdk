/*-
 *   BSD LICENSE
 *
 *   Copyright (C) 2008-2012 Daisuke Aoyama <aoyama@peach.ne.jp>.
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file
 * Block device abstraction layer
 */

#ifndef SPDK_BDEV_H_
#define SPDK_BDEV_H_

#include "spdk/stdinc.h"

#include "spdk/event.h"
#include "spdk/scsi_spec.h"
#include "spdk/nvme_spec.h"

#define SPDK_BDEV_SMALL_BUF_MAX_SIZE 8192
#define SPDK_BDEV_LARGE_BUF_MAX_SIZE (64 * 1024)

typedef void (*spdk_bdev_remove_cb_t)(void *remove_ctx);

/**
 * Block device I/O
 *
 * This is an I/O that is passed to an spdk_bdev.
 */
struct spdk_bdev_io;

struct spdk_bdev_fn_table;
struct spdk_io_channel;
struct spdk_json_write_ctx;

/** Blockdev status */
enum spdk_bdev_status {
	SPDK_BDEV_STATUS_INVALID,
	SPDK_BDEV_STATUS_UNCLAIMED,
	SPDK_BDEV_STATUS_CLAIMED,
	SPDK_BDEV_STATUS_REMOVING,
};

/**
 * \brief SPDK block device.
 *
 * This is a virtual representation of a block device that is exported by the backend.
 */
struct spdk_bdev;

/** Blockdev I/O type */
enum spdk_bdev_io_type {
	SPDK_BDEV_IO_TYPE_READ = 1,
	SPDK_BDEV_IO_TYPE_WRITE,
	SPDK_BDEV_IO_TYPE_UNMAP,
	SPDK_BDEV_IO_TYPE_FLUSH,
	SPDK_BDEV_IO_TYPE_RESET,
	SPDK_BDEV_IO_TYPE_NVME_ADMIN,
	SPDK_BDEV_IO_TYPE_NVME_IO,
};

/**
 * Block device completion callback
 *
 * \param bdev_io Block device I/O that has completed.
 * \param success true if I/O completed successfully or false if it failed; additional error
 *                information may be retrieved from bdev_io by calling
 *                spdk_bdev_io_get_nvme_status() or spdk_bdev_io_get_scsi_status().
 * \param cb_arg Callback argument specified when bdev_io was submitted.
 */
typedef void (*spdk_bdev_io_completion_cb)(struct spdk_bdev_io *bdev_io,
		bool success,
		void *cb_arg);

struct spdk_bdev_io_stat {
	uint64_t bytes_read;
	uint64_t num_read_ops;
	uint64_t bytes_written;
	uint64_t num_write_ops;
};

struct spdk_bdev *spdk_bdev_get_by_name(const char *bdev_name);

struct spdk_bdev *spdk_bdev_first(void);
struct spdk_bdev *spdk_bdev_next(struct spdk_bdev *prev);

/**
 * Claim ownership of a block device.
 *
 * User applications and virtual blockdevs may use this to mediate access to bdevs.
 *
 * When the ownership of the bdev is no longer needed, the user should call spdk_bdev_unclaim().
 *
 * \param bdev Block device to claim.
 * \param remove_cb callback function for hot remove the device.
 * \param remove_ctx param for hot removal callback function.
 * \return true if the caller claimed the bdev, or false if it was already claimed by another user.
 */
bool spdk_bdev_claim(struct spdk_bdev *bdev, spdk_bdev_remove_cb_t remove_cb, void *remove_ctx);

/**
 * Release claim of ownership of a block device.
 *
 * When a bdev reference acquired with spdk_bdev_claim() is no longer needed, the user should
 * release the claim using spdk_bdev_unclaim().
 *
 * \param bdev Block device to release.
 */
void spdk_bdev_unclaim(struct spdk_bdev *bdev);

bool spdk_bdev_io_type_supported(struct spdk_bdev *bdev, enum spdk_bdev_io_type io_type);

int spdk_bdev_dump_config_json(struct spdk_bdev *bdev, struct spdk_json_write_ctx *w);

/**
 * Get block device name.
 *
 * \param bdev Block device to query.
 * \return Name of bdev as a null-terminated string.
 */
const char *spdk_bdev_get_name(const struct spdk_bdev *bdev);

/**
 * Get block device product name.
 *
 * \param bdev Block device to query.
 * \return Product name of bdev as a null-terminated string.
 */
const char *spdk_bdev_get_product_name(const struct spdk_bdev *bdev);

/**
 * Get block device logical block size.
 *
 * \param bdev Block device to query.
 * \return Size of logical block for this bdev in bytes.
 */
uint32_t spdk_bdev_get_block_size(const struct spdk_bdev *bdev);

/**
 * Get size of block device in logical blocks.
 *
 * \param bdev Block device to query.
 * \return Size of bdev in logical blocks.
 *
 * Logical blocks are numbered from 0 to spdk_bdev_get_num_blocks(bdev) - 1, inclusive.
 */
uint64_t spdk_bdev_get_num_blocks(const struct spdk_bdev *bdev);

/**
 * Get maximum number of descriptors per unmap request.
 *
 * \param bdev Block device to query.
 * \return Maximum number of unmap descriptors per request.
 */
uint32_t spdk_bdev_get_max_unmap_descriptors(const struct spdk_bdev *bdev);

/**
 * Get minimum I/O buffer address alignment for a bdev.
 *
 * \param bdev Block device to query.
 * \return Required alignment of I/O buffers in bytes.
 */
size_t spdk_bdev_get_buf_align(const struct spdk_bdev *bdev);

/**
 * Query whether block device has an enabled write cache.
 *
 * \param bdev Block device to query.
 * \return true if block device has a volatile write cache enabled.
 *
 * If this function returns true, written data may not be persistent until a flush command
 * is issued.
 */
bool spdk_bdev_has_write_cache(const struct spdk_bdev *bdev);

/**
 * Obtain an I/O channel for this block device. I/O channels are bound to threads,
 * so the resulting I/O channel may only be used from the thread it was originally
 * obtained from.
 *
 * \param bdev Block device
 *
 * \return A handle to the I/O channel or NULL on failure.
 */
struct spdk_io_channel *spdk_bdev_get_io_channel(struct spdk_bdev *bdev);

/**
 * Submit a read request to the bdev on the given channel.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param buf Data buffer to read into.
 * \param offset The offset, in bytes, from the start of the block device.
 * \param nbytes The number of bytes to read.
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_read(struct spdk_bdev *bdev, struct spdk_io_channel *ch,
		   void *buf, uint64_t offset, uint64_t nbytes,
		   spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Submit a read request to the bdev on the given channel. This differs from
 * spdk_bdev_read by allowing the data buffer to be described in a scatter
 * gather list. Some physical devices place memory alignment requirements on
 * data and may not be able to directly transfer into the buffers provided. In
 * this case, the request may fail.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param iov A scatter gather list of buffers to be read into.
 * \param iovcnt The number of elements in iov.
 * \param offset The offset, in bytes, from the start of the block device.
 * \param nbytes The number of bytes to read.
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_readv(struct spdk_bdev *bdev, struct spdk_io_channel *ch,
		    struct iovec *iov, int iovcnt,
		    uint64_t offset, uint64_t nbytes,
		    spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Submit a write request to the bdev on the given channel.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param buf Data buffer to written from.
 * \param offset The offset, in bytes, from the start of the block device.
 * \param nbytes The number of bytes to write. buf must be greater than or equal to this size.
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_write(struct spdk_bdev *bdev, struct spdk_io_channel *ch,
		    void *buf, uint64_t offset, uint64_t nbytes,
		    spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Submit a write request to the bdev on the given channel. This differs from
 * spdk_bdev_write by allowing the data buffer to be described in a scatter
 * gather list. Some physical devices place memory alignment requirements on
 * data and may not be able to directly transfer out of the buffers provided. In
 * this case, the request may fail.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param iov A scatter gather list of buffers to be written from.
 * \param iovcnt The number of elements in iov.
 * \param offset The offset, in bytes, from the start of the block device.
 * \param nbytes The number of bytes to read. buf must be greater than or equal to this size.
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_writev(struct spdk_bdev *bdev, struct spdk_io_channel *ch,
		     struct iovec *iov, int iovcnt,
		     uint64_t offset, uint64_t len,
		     spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Submit an unmap request to the block device. Unmap is sometimes also called trim or
 * deallocate. This notifies the device that the data in the blocks described is no
 * longer valid. Reading blocks that have been unmapped results in indeterminate data.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param unmap_d An array of unmap descriptors.
 * \param bdesc_count The number of elements in unmap_d.
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_unmap(struct spdk_bdev *bdev, struct spdk_io_channel *ch,
		    struct spdk_scsi_unmap_bdesc *unmap_d,
		    uint16_t bdesc_count,
		    spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Submit a flush request to the bdev on the given channel. For devices with volatile
 * caches, data is not guaranteed to be persistent until the completion of a flush
 * request. Call spdk_bdev_has_write_cache() to check if the bdev has a volatile cache.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param offset The offset, in bytes, from the start of the block device.
 * \param length The number of bytes.
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_flush(struct spdk_bdev *bdev, struct spdk_io_channel *ch,
		    uint64_t offset, uint64_t length,
		    spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Submit a reset request to the bdev on the given channel.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_reset(struct spdk_bdev *bdev, struct spdk_io_channel *ch,
		    spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Submit an NVMe Admin command to the bdev. This passes directly through
 * the block layer to the device. Support for NVMe passthru is optional,
 * indicated by calling spdk_bdev_io_type_supported().
 *
 * The SGL/PRP will be automated generated based on the given buffer,
 * so that portion of the command may be left empty.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param cmd The raw NVMe command. Must be an admin command.
 * \param buf Data buffer to written from.
 * \param nbytes The number of bytes to transfer. buf must be greater than or equal to this size.
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_nvme_admin_passthru(struct spdk_bdev *bdev,
				  struct spdk_io_channel *ch,
				  const struct spdk_nvme_cmd *cmd,
				  void *buf, size_t nbytes,
				  spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Submit an NVMe I/O command to the bdev. This passes directly through
 * the block layer to the device. Support for NVMe passthru is optional,
 * indicated by calling spdk_bdev_io_type_supported().
 *
 * The SGL/PRP will be automated generated based on the given buffer,
 * so that portion of the command may be left empty. Also, the namespace
 * id (nsid) will be populated automatically.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param cmd The raw NVMe command. Must be in the NVM command set.
 * \param buf Data buffer to written from.
 * \param nbytes The number of bytes to transfer. buf must be greater than or equal to this size.
 * \param cb Called when the request is complete.
 * \param cb_arg Argument passed to cb.
 *
 * \return 0 on success. On success, the callback will always
 * be called (even if the request ultimately failed). Return
 * negated errno on failure, in which case the callback will not be called.
 */
int spdk_bdev_nvme_io_passthru(struct spdk_bdev *bdev,
			       struct spdk_io_channel *ch,
			       const struct spdk_nvme_cmd *cmd,
			       void *buf, size_t nbytes,
			       spdk_bdev_io_completion_cb cb, void *cb_arg);

/**
 * Free an I/O request. This should be called after the callback for the I/O has
 * been called and notifies the bdev layer that memory may now be released.
 *
 * \param bdev_io I/O request
 *
 * \return -1 on failure, 0 on success.
 */
int spdk_bdev_free_io(struct spdk_bdev_io *bdev_io);

/**
 * Return I/O statistics for this channel. After returning stats, zero out
 * the current state of the statistics.
 *
 * \param bdev Block device
 * \param ch I/O channel. Obtained by calling spdk_bdev_get_io_channel().
 * \param stat The per-channel statistics.
 *
 */
void spdk_bdev_get_io_stat(struct spdk_bdev *bdev, struct spdk_io_channel *ch,
			   struct spdk_bdev_io_stat *stat);

/**
 * Get the status of bdev_io as an NVMe status code.
 *
 * \param bdev_io I/O to get the status from.
 * \param sct Status Code Type return value, as defined by the NVMe specification.
 * \param sc Status Code return value, as defined by the NVMe specification.
 */
void spdk_bdev_io_get_nvme_status(const struct spdk_bdev_io *bdev_io, int *sct, int *sc);

/**
 * Get the status of bdev_io as a SCSI status code.
 *
 * \param bdev_io I/O to get the status from.
 * \param sc SCSI Status Code.
 * \param sk SCSI Sense Key.
 * \param asc SCSI Additional Sense Code.
 * \param ascq SCSI Additional Sense Code Qualifier.
 */
void spdk_bdev_io_get_scsi_status(const struct spdk_bdev_io *bdev_io,
				  int *sc, int *sk, int *asc, int *ascq);

/**
 * Get the iovec describing the data buffer of a bdev_io.
 *
 * \param bdev_io I/O to describe with iovec.
 * \param iovp Pointer to be filled with iovec.
 * \param iovcntp Pointer to be filled with number of iovec entries.
 */
void spdk_bdev_io_get_iovec(struct spdk_bdev_io *bdev_io, struct iovec **iovp, int *iovcntp);

#endif /* SPDK_BDEV_H_ */
