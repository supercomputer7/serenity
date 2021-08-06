/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Atomic.h>
#include <AK/CircularQueue.h>
#include <AK/NonnullRefPtrVector.h>
#include <AK/RefPtr.h>
#include <AK/Time.h>
#include <AK/Types.h>
#include <Kernel/API/KeyCode.h>
#include <Kernel/API/MousePacket.h>
#include <Kernel/KResult.h>
#include <Kernel/SpinLock.h>
#include <Kernel/UnixTypes.h>
#include <LibKeyboard/CharacterMap.h>

namespace Kernel::USB {

class HostController;
class Management {
    AK_MAKE_ETERNAL;

public:
    Management();
    static void initialize();
    static Management& the();

    void enumerate();

private:
    RefPtr<HostController> determine_host_controller_type(PCI::Address address) const;
    NonnullRefPtrVector<HostController> m_host_controllers;
};

}
