/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Devices/GPU/Bochs/Definitions.h>
#include <Kernel/Devices/GPU/Console/GenericFramebufferConsole.h>
#include <Kernel/Devices/GPU/Device.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/PhysicalAddress.h>

namespace Kernel {

class GPUManagement;
struct BochsDisplayMMIORegisters;

class BochsGPUAdapter final : public GPUDevice
    , public PCI::Device {
    friend class GPUManagement;

public:
    static ErrorOr<bool> probe(PCI::DeviceIdentifier const&);
    static ErrorOr<NonnullLockRefPtr<GPUDevice>> create(PCI::DeviceIdentifier const&);

    static bool probe_plain_vga_isa(Badge<GPUManagement>);
    static ErrorOr<NonnullLockRefPtr<GPUDevice>> try_create_for_plain_vga_isa(Badge<GPUManagement>);

    virtual ~BochsGPUAdapter() = default;
    virtual StringView device_name() const override { return "BochsGPUAdapter"sv; }

private:
    ErrorOr<void> initialize_adapter(PCI::DeviceIdentifier const&);

    ErrorOr<void> validate_framebuffer_mode_setting(ModeSetting const& mode_setting);
    void change_framebuffer_mode_setting_with_mmio(ModeSetting const& mode_setting);

    virtual ErrorOr<NonnullLockRefPtr<Memory::VMObject>> vmobject_for_mmap(Process&, Memory::VirtualRange const&, u64&, bool) override;

    explicit BochsGPUAdapter(PCI::DeviceIdentifier const&);

    OwnPtr<IOWindow> m_dispi_mmio_registers;
    OwnPtr<IOWindow> m_vga_mmio_registers;
};
}
