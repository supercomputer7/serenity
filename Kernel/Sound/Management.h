/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Badge.h>
#include <AK/NonnullRefPtrVector.h>
#include <AK/OwnPtr.h>
#include <AK/RefPtr.h>
#include <AK/Time.h>
#include <AK/Types.h>
#include <Kernel/API/KResult.h>
#include <Kernel/API/TimePage.h>
#include <Kernel/Arch/x86/RegisterState.h>
#include <Kernel/Devices/ConsoleDevice.h>
#include <Kernel/Devices/Device.h>
#include <Kernel/Sound/AudioInterfaceDevice.h>
#include <Kernel/Sound/SoundDevice.h>
#include <Kernel/Sound/SB16.h>
#include <Kernel/UnixTypes.h>

namespace Kernel {

class SoundManagement {
    AK_MAKE_ETERNAL;

public:
    SoundManagement();
    void initialize_and_enumerate();
    static SoundManagement& the();

    KResult try_set_sample_rate(size_t sound_device_index, size_t sample_rate);
    KResultOr<size_t> try_get_sample_rate(size_t sound_device_index);

private:

    // Note: this is the device that is exposed in /dev/audio
    RefPtr<AudioInterfaceDevice> m_interface_audio_device;

    RefPtr<SoundDevice> m_default_sound_device;
    IntrusiveList<&SoundDevice::m_node> m_sound_devices;
};

}
