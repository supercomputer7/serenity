/*
 * Copyright (c) 2021, Sahan Fernando <sahan.h.fernando@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/BinaryBufferWriter.h>
#include <AK/DistinctNumeric.h>
#include <Kernel/Bus/VirtIO/Device.h>
#include <Kernel/Bus/VirtIO/Queue.h>
#include <Kernel/Devices/BlockDevice.h>
#include <Kernel/Graphics/GenericGraphicsAdapter.h>
#include <Kernel/Graphics/VirtIOGPU/Console.h>
#include <Kernel/Graphics/VirtIOGPU/FramebufferDevice.h>
#include <Kernel/Graphics/VirtIOGPU/Protocol.h>
#include <LibEDID/EDID.h>

namespace Kernel {

#define VIRTIO_GPU_F_VIRGL (1 << 0)
#define VIRTIO_GPU_F_EDID (1 << 1)

#define VIRTIO_GPU_FLAG_FENCE (1 << 0)

#define CONTROLQ 0
#define CURSORQ 1

#define MAX_VIRTIOGPU_RESOLUTION_WIDTH 3840
#define MAX_VIRTIOGPU_RESOLUTION_HEIGHT 2160

#define VIRTIO_GPU_EVENT_DISPLAY (1 << 0)

class VirtIOGraphicsAdapter final
    : public GenericGraphicsAdapter
    , public VirtIO::Device {

public:
    static NonnullRefPtr<GraphicsAdapter> initialize(PCI::DeviceIdentifier const&);

    // FIXME: There's a VirtIO VGA GPU variant, so we should consider that
    virtual bool vga_compatible() const override { return false; }

    virtual void initialize() override;

private:
    ErrorOr<void> initialize_adapter();

    explicit GraphicsAdapter(PCI::DeviceIdentifier const&);

    virtual bool handle_device_config_change() override;
    virtual void handle_queue_update(u16 queue_index) override;

    RefPtr<VGAGenericDisplayConnector> m_display_connector;

    // Synchronous commands
    WaitQueue m_outstanding_request;
    Mutex m_operation_lock;
    OwnPtr<Memory::Region> m_scratch_space;
};
}
