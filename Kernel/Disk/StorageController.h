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

#pragma once

#include <AK/RefCounted.h>
#include <AK/Types.h>
#include <AK/Vector.h>
#include <Kernel/Devices/BlockDevice.h>
#include <Kernel/PCI/Device.h>
#include <Kernel/PhysicalAddress.h>

namespace Kernel {
class IRQHandler;

enum class TransferMode {
    NotSpecified = 0,
    PIO28 = 1,
    PIO48 = 2,
    DMA = 3,
    UDMA = 4
};

enum class ProtocolMode {
    NotSpecified = 0,
    PacketInterface = 1
};

enum class StorageDeviceType {
    PATA = 0,
    PATAPI = 1,
    SATA = 2,
    SATAPI = 3,
    NVMe = 4
};

class StorageDevice;
class StorageController : public PCI::Device {
public:
    virtual void initialize() = 0;

    virtual bool write(const StorageDevice&, TransferMode, ProtocolMode, u64 block, u16 count, const UserOrKernelBuffer& inbuf) const = 0;
    virtual bool read(const StorageDevice&, TransferMode, ProtocolMode, u64 block, u16 count, UserOrKernelBuffer& outbuf) const = 0;
    virtual bool eject(const StorageDevice&) const = 0;
    virtual bool reset_device(const StorageDevice&) const = 0;

    virtual bool reset() = 0;
    virtual bool shutdown() = 0;
    virtual bool is_operational() const = 0;

protected:
    // FIXME: I assume that all disk controllers are enumerated over PCI and we use the PIC for now.
    // FIXME: Once we have SMP working (with AML being parsed + getting IRQ redirections), we will need to change this a bit.
    explicit StorageController(PCI::Address address)
        : PCI::Device(address)
    {
    }
    Vector<RefPtr<StorageDevice>> m_connected_storage;
};
}
