/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Memory.h>
#include <AK/StringView.h>
#include <Kernel/Debug.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Sections.h>
#include <Kernel/Sound/AudioInterfaceDevice.h>
#include <Kernel/Sound/Management.h>
#include <Kernel/Thread.h>
#include <LibC/sys/ioctl_numbers.h>

namespace Kernel {

UNMAP_AFTER_INIT AudioInterfaceDevice::AudioInterfaceDevice()
    : CharacterDevice(42, 42)
{
}

UNMAP_AFTER_INIT AudioInterfaceDevice::~AudioInterfaceDevice()
{
}

UNMAP_AFTER_INIT NonnullRefPtr<AudioInterfaceDevice> AudioInterfaceDevice::must_create(Badge<SoundManagement>)
{
    auto device_or_error = DeviceManagement::try_create_device<AudioInterfaceDevice>();
    VERIFY(!device_or_error.is_error());
    return device_or_error.release_value();
}

KResult AudioInterfaceDevice::ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg)
{
    switch (request) {
    case SOUNDCARD_IOCTL_GET_SAMPLE_RATE: {
        AudioDeviceSampleRate input;
        TRY(copy_from_user(&input, static_ptr_cast<AudioDeviceSampleRate*>(arg)));
        if (input.index < 0)
            return EINVAL;
        size_t current_rate = TRY(SoundManagement::the().try_get_sample_rate(static_cast<size_t>(input.index)));
        input.rate = current_rate;
        auto output = static_ptr_cast<AudioDeviceSampleRate*>(arg);
        return copy_to_user(output, &input);
    }
    case SOUNDCARD_IOCTL_SET_SAMPLE_RATE: {
        AudioDeviceSampleRate input;
        TRY(copy_from_user(&input, static_ptr_cast<AudioDeviceSampleRate*>(arg)));
        if (input.index < 0)
            return EINVAL;
        if (input.rate < 0)
            return EINVAL;
        return SoundManagement::the().try_set_sample_rate(static_cast<size_t>(input.index), static_cast<size_t>(input.rate));
    }
    default:
        return EINVAL;
    }
}

bool AudioInterfaceDevice::can_read(OpenFileDescription const&, size_t) const
{
    return false;
}

KResultOr<size_t> AudioInterfaceDevice::read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t)
{
    return 0;
}

//KResultOr<size_t> AudioInterfaceDevice::write(OpenFileDescription&, u64, UserOrKernelBuffer const& data, size_t length)
KResultOr<size_t> AudioInterfaceDevice::write(OpenFileDescription&, u64, UserOrKernelBuffer const&, size_t)
{
    return KResult(ENOSPC);
}

}
