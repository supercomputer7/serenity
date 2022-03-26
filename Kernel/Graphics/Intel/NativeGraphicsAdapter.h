/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Graphics/Intel/DisplayConnectorGroup.h>
#include <Kernel/Graphics/VGA/PCIGenericAdapter.h>
#include <Kernel/PhysicalAddress.h>
#include <LibEDID/EDID.h>

namespace Kernel {

class IntelNativeGraphicsAdapter final
    : public PCIVGAGenericAdapter {

public:
    static RefPtr<IntelNativeGraphicsAdapter> initialize(PCI::DeviceIdentifier const&);

private:
    ErrorOr<void> initialize_adapter();

    IntelNativeGraphicsAdapter(PCI::Address, PCI::HardwareID);

    const PCI::HardwareID m_hardware_id;
    RefPtr<IntelDisplayConnectorGroup> m_connector_group;
};
}
