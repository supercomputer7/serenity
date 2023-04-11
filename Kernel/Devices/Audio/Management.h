/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Badge.h>
#include <AK/Error.h>
#include <AK/IntrusiveList.h>
#include <AK/OwnPtr.h>
#include <AK/Types.h>
#include <Kernel/Devices/Audio/Controller.h>
#include <Kernel/Library/LockRefPtr.h>

namespace Kernel {

class AudioManagement {

public:
    AudioManagement();
    static AudioManagement& the();

    static MajorNumber audio_type_major_number();
    static MinorNumber generate_storage_minor_number();

    bool initialize();

private:
    ErrorOr<NonnullRefPtr<AudioController>> determine_audio_device(PCI::DeviceIdentifier const& device_identifier) const;

    void enumerate_hardware_controllers();
    SpinlockProtected<IntrusiveList<&AudioController::m_node>, LockRank::None> m_controllers_list;
};

}
