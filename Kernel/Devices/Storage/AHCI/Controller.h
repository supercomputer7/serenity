/*
 * Copyright (c) 2021-2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/OwnPtr.h>
#include <AK/Types.h>
#include <Kernel/Devices/Storage/AHCI/Definitions.h>
#include <Kernel/Devices/Storage/StorageController.h>
#include <Kernel/Devices/Storage/StorageDevice.h>
#include <Kernel/Library/Driver.h>
#include <Kernel/Library/LockRefPtr.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Sections.h>

namespace Kernel {

class AsyncBlockDeviceRequest;
class AHCIInterruptHandler;
class AHCIPort;
class AHCIController final : public StorageController {
    friend class AHCIInterruptHandler;

    KERNEL_MAKE_DRIVER_LISTABLE(AHCIController)
public:
    static ErrorOr<NonnullRefPtr<AHCIController>> initialize(PCI::Device&);
    virtual ~AHCIController() override;

    virtual void start_request(ATA::Address address, AsyncBlockDeviceRequest&) override;
    virtual void complete_current_request(AsyncDeviceRequest::RequestResult) override;

    void start_request(ATA::Address, AsyncBlockDeviceRequest&);

    void handle_interrupt_for_port(Badge<AHCIInterruptHandler>, u32 port_index) const;

    PCI::Device& pci_device() { return *m_pci_device; }

private:
    ErrorOr<void> reset();

    void disable_global_interrupts() const;
    void enable_global_interrupts() const;

    explicit AHCIController(PCI::Device&);
    ErrorOr<void> initialize_hba();

    AHCI::HBADefinedCapabilities capabilities() const;

    volatile AHCI::PortRegisters& port(size_t port_number) const;
    ErrorOr<Memory::TypedMapping<AHCI::HBA volatile>> map_default_hba_region();
    volatile AHCI::HBA& hba() const;

    Array<LockRefPtr<AHCIPort>, 32> m_ports;
    Memory::TypedMapping<AHCI::HBA volatile> m_hba_mapping;
    AHCI::HBADefinedCapabilities m_hba_capabilities;

    NonnullRefPtr<PCI::Device> const m_pci_device;

    // FIXME: There could be multiple IRQ (MSI) handlers for AHCI. Find a way to use all of them.
    OwnPtr<AHCIInterruptHandler> m_irq_handler;

    // Note: This lock is intended to be locked when doing changes to HBA registers
    // that affect its core functionality in a manner that controls all attached storage devices
    // to the HBA SATA ports.
    mutable Spinlock<LockRank::None> m_hba_control_lock {};
};
}
