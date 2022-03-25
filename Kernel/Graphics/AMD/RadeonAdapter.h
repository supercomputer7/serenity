/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Memory/MappedROM.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Graphics/GenericGraphicsAdapter.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/PhysicalAddress.h>
#include <Kernel/Graphics/AMD/ATOMBIOS.h>

namespace Kernel {

class GraphicsManagement;

class RadeonDisplayConnector;
class RadeonGraphicsAdapter final : public GenericGraphicsAdapter
    , public PCI::Device {
    friend class GraphicsManagement;

public:
    static NonnullRefPtr<RadeonGraphicsAdapter> initialize(PCI::DeviceIdentifier const&);
    virtual ~RadeonGraphicsAdapter() = default;

    virtual bool vga_compatible() const override;

private:
    ErrorOr<void> initialize_adapter(PCI::DeviceIdentifier const&);

    explicit RadeonGraphicsAdapter(PCI::DeviceIdentifier const&);

    ErrorOr<Memory::MappedROM> map_atombios();

    //RefPtr<RadeonDisplayConnector> m_display_connector;
    OwnPtr<ATOMBIOSParser> m_atombios_parser;
    bool m_is_vga_capable { false };
};
}
