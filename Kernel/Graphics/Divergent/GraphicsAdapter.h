/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Graphics/Divergent/Definitions.h>
#include <Kernel/Graphics/Console/GenericFramebufferConsole.h>
#include <Kernel/Graphics/Divergent/DisplayConnector.h>
#include <Kernel/Graphics/GenericGraphicsAdapter.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/PhysicalAddress.h>

namespace Kernel {

class GraphicsManagement;

class DivergentDisplayConnector;
class DivergentGraphicsAdapter final : public GenericGraphicsAdapter
    , public PCI::Device {
    friend class GraphicsManagement;

public:
    static NonnullRefPtr<DivergentGraphicsAdapter> initialize(PCI::DeviceIdentifier const&);
    virtual ~DivergentGraphicsAdapter() = default;

    virtual bool vga_compatible() const override { return false; }

    ErrorOr<void> apply_mode_setting_on_connector(Badge<DivergentDisplayConnector>, DivergentDisplayConnector const&, DisplayConnector::ModeSetting const&);

private:
    ErrorOr<void> initialize_adapter(PCI::DeviceIdentifier const&);

    DivergentGraphicsAdapter(PCI::DeviceIdentifier const&, Memory::TypedMapping<DivergentDisplayProgrammingInterface volatile>);

    Array<RefPtr<DivergentDisplayConnector>, 8> m_display_connectors;
    Memory::TypedMapping<DivergentDisplayProgrammingInterface volatile> m_registers_mapping;
};
}
