/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullRefPtr.h>
#include <AK/NonnullRefPtrVector.h>
#include <AK/OwnPtr.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Interrupts/IRQHandler.h>
#include <Kernel/Locking/Spinlock.h>
#include <Kernel/Memory/MemoryManager.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Storage/NVMe/NVMeDefinitions.h>

namespace Kernel {

class AsyncBlockDeviceRequest;
class SDMMCQueue
    : public RefCounted<SDMMCQueue>
    , public IRQHandler {
public:
    static ErrorOr<NonnullRefPtr<SDMMCQueue>> try_create();
    void read(AsyncBlockDeviceRequest& request, u16 nsid, u64 index, u32 count);
    void write(AsyncBlockDeviceRequest& request, u16 nsid, u64 index, u32 count);
    virtual ~SDMMCQueue() { }

private:
};
}
