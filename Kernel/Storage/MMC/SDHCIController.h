/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/OwnPtr.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Sections.h>
#include <Kernel/Storage/MMC/Definitions.h>
#include <Kernel/Storage/StorageController.h>
#include <Kernel/Storage/StorageDevice.h>

namespace Kernel {

class AsyncBlockDeviceRequest;
class MMCDevice;
class SDMMCQueue;
class SDHCIController final
    : public StorageController
    , public PCI::Device
    , public Weakable<SDHCIController> {

public:
    static NonnullRefPtr<SDHCIController> must_initialize(PCI::DeviceIdentifier const& pci_device_identifier);
    virtual ~SDHCIController() override;

    virtual RefPtr<StorageDevice> device(u32 index) const override;
    virtual bool reset() override;
    virtual bool shutdown() override;
    virtual size_t devices_count() const override;
    void start_request(MMCDevice const&, AsyncBlockDeviceRequest&);
    virtual void complete_current_request(AsyncDeviceRequest::RequestResult) override;

private:
    explicit SDHCIController(PCI::DeviceIdentifier const&, Memory::TypedMapping<SDHCHostRegisters volatile>);
    void initialize_hba();

    Memory::TypedMapping<SDHCHostRegisters volatile> m_host_controller_registers_mapping;
    PCI::InterruptLine m_interrupt_line;
    NonnullRefPtr<SDMMCQueue> m_queues;
};

}
