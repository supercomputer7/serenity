/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/HashTable.h>
#include <AK/Singleton.h>
#include <Kernel/FileSystem/InodeMetadata.h>
#include <Kernel/Sound/AudioInterfaceDevice.h>
#include <Kernel/Sections.h>
#include <Kernel/Sound/Management.h>

namespace Kernel {

static Singleton<SoundManagement> s_the;

UNMAP_AFTER_INIT SoundManagement::SoundManagement()
{
}

UNMAP_AFTER_INIT void SoundManagement::initialize_and_enumerate()
{
    m_interface_audio_device = AudioInterfaceDevice::must_create({});
    auto device = SB16::try_detect({});
    if (device) {
        m_sound_devices.append(*device);
        m_default_sound_device = device;
    }
        
    VERIFY(m_sound_devices.size_slow() == 1);
}

//KResultOr<size_t> SoundManagement::try_get_sample_rate(size_t sound_device_index)
KResultOr<size_t> SoundManagement::try_get_sample_rate(size_t)
{
    dbgln("SoundManagement::try_get_sample_rate");
    return KResult(ENODEV);
}

KResult SoundManagement::try_set_sample_rate(size_t, size_t)
//KResult SoundManagement::try_set_sample_rate(size_t sound_device_index, size_t)
{
    dbgln("SoundManagement::try_set_sample_rate");
    return KResult(ENODEV);
}

SoundManagement& SoundManagement::the()
{
    return *s_the;
}

}
