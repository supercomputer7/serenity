/*
 * Copyright (c) 2020, Liav A. <liavalb@hotmail.co.il>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//#define STORAGE_DEVICE_DEBUG

#include <AK/Memory.h>
#include <AK/StringView.h>
#include <Kernel/Disk/StorageDevice.h>
#include <Kernel/FileSystem/FileDescription.h>

namespace Kernel {

bool StorageDevice::read_blocks(unsigned index, u16 count, UserOrKernelBuffer& out)
{
    return m_storage_controller->read(*this, TransferMode::NotSpecified, ProtocolMode::NotSpecified, index, count, out);
}

bool StorageDevice::write_blocks(unsigned index, u16 count, const UserOrKernelBuffer& data)
{
    return m_storage_controller->write(*this, TransferMode::NotSpecified, ProtocolMode::NotSpecified, index, count, data);
}

KResultOr<size_t> StorageDevice::read(FileDescription&, size_t offset, UserOrKernelBuffer& outbuf, size_t len)
{
    unsigned index = offset / block_size();
    u16 whole_blocks = len / block_size();
    ssize_t remaining = len % block_size();

    unsigned blocks_per_page = PAGE_SIZE / block_size();

    if (whole_blocks >= blocks_per_page) {
        whole_blocks = blocks_per_page;
        remaining = 0;
    }

#ifdef STORAGE_DEVICE_DEBUG
    klog() << "StorageDevice::read() index=" << index << " whole_blocks=" << whole_blocks << " remaining=" << remaining;
#endif

    if (whole_blocks > 0) {
        if (!read_blocks(index, whole_blocks, outbuf))
            return -1;
    }

    off_t pos = whole_blocks * block_size();

    if (remaining > 0) {
        auto data = ByteBuffer::create_uninitialized(block_size());
        auto data_buffer = UserOrKernelBuffer::for_kernel_buffer(data.data());
        if (!read_blocks(index + whole_blocks, 1, data_buffer))
            return pos;
        if (!outbuf.write(data.data(), pos, remaining))
            return KResult(-EFAULT);
    }

    return pos + remaining;
}

bool StorageDevice::can_read(const FileDescription&, size_t offset) const
{
    return offset < (max_block() * block_size());
}

KResultOr<size_t> StorageDevice::write(FileDescription&, size_t offset, const UserOrKernelBuffer& inbuf, size_t len)
{
    unsigned index = offset / block_size();
    u16 whole_blocks = len / block_size();
    ssize_t remaining = len % block_size();

    unsigned blocks_per_page = PAGE_SIZE / block_size();

    if (whole_blocks >= blocks_per_page) {
        whole_blocks = blocks_per_page;
        remaining = 0;
    }

#ifdef STORAGE_DEVICE_DEBUG
    klog() << "StorageDevice::write() index=" << index << " whole_blocks=" << whole_blocks << " remaining=" << remaining;
#endif

    if (whole_blocks > 0) {
        if (!write_blocks(index, whole_blocks, inbuf))
            return -1;
    }

    off_t pos = whole_blocks * block_size();

    // since we can only write in block_size() increments, if we want to do a
    // partial write, we have to read the block's content first, modify it,
    // then write the whole block back to the disk.
    if (remaining > 0) {
        auto data = ByteBuffer::create_zeroed(block_size());
        auto data_buffer = UserOrKernelBuffer::for_kernel_buffer(data.data());
        if (!read_blocks(index + whole_blocks, 1, data_buffer))
            return pos;
        if (!inbuf.read(data.data(), pos, remaining))
            return KResult(-EFAULT);
        if (!write_blocks(index + whole_blocks, 1, data_buffer))
            return pos;
    }

    return pos + remaining;
}

bool StorageDevice::can_write(const FileDescription&, size_t offset) const
{
    return offset < (max_block() * block_size());
}

}
