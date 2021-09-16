/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Badge.h>
#include <Kernel/Devices/CharacterDevice.h>

namespace Kernel {

class SoundManagement;
class AudioInterfaceDevice final : public CharacterDevice {
    friend class DeviceManagement;
    friend class SoundManagement;

public:
    virtual ~AudioInterfaceDevice() override;

    static NonnullRefPtr<AudioInterfaceDevice> must_create(Badge<SoundManagement>);

    // ^CharacterDevice
    virtual bool can_read(const OpenFileDescription&, size_t) const override;
    virtual KResultOr<size_t> read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t) override;
    virtual KResultOr<size_t> write(OpenFileDescription&, u64, const UserOrKernelBuffer&, size_t) override;
    virtual bool can_write(const OpenFileDescription&, size_t) const override { return true; }

    virtual KResult ioctl(OpenFileDescription&, unsigned, Userspace<void*>) override;

private:
    AudioInterfaceDevice();

    // ^CharacterDevice
    virtual StringView class_name() const override { return "AudioInterfaceDevice"; }
};
}
