/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/IntrusiveList.h>
#include <AK/RefCounted.h>
#include <AK/Types.h>
#include <Kernel/API/KResult.h>
#include <Kernel/UserOrKernelBuffer.h>

namespace Kernel {

class SoundDevice : public RefCounted<SoundDevice> {
    friend class SoundManagement;

public:
    virtual ~SoundDevice() {};

    virtual bool can_read() const = 0;
    virtual KResultOr<size_t> read(UserOrKernelBuffer&, size_t) = 0;
    virtual KResultOr<size_t> write(const UserOrKernelBuffer&, size_t) = 0;
    virtual bool can_write(size_t) const { return true; }

protected:
    SoundDevice() = default;

    virtual StringView class_name() const = 0;
    u16 m_sample_rate { 44100 };

    IntrusiveListNode<SoundDevice, RefPtr<SoundDevice>> m_node;
};
}
