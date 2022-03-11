/*
 * Copyright (c) 2021, Sahan Fernando <sahan.h.fernando@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/API/VirGL.h>
#include <Kernel/Graphics/GraphicsManagement.h>
#include <Kernel/Graphics/VirtIOGPU/Console.h>
#include <Kernel/Graphics/VirtIOGPU/DisplayConnector.h>
#include <Kernel/Graphics/VirtIOGPU/GraphicsAdapter.h>
#include <Kernel/Graphics/VirtIOGPU/Protocol.h>
#include <Kernel/Random.h>
#include <LibC/sys/ioctl_numbers.h>

namespace Kernel::Graphics::VirtIOGPU {

NonnullRefPtr<VirtIODisplayConnector> VirtIODisplayConnector::must_create(VirtIOGPU::GraphicsAdapter& graphics_adapter)
{
    // Setup memory transfer region
    auto virtio_virgl3d_uploader_buffer_region = TRY(MM.allocate_kernel_region(
        NUM_TRANSFER_REGION_PAGES * PAGE_SIZE,
        "VIRGL3D upload buffer",
        Memory::Region::Access::ReadWrite,
        AllocationStrategy::AllocateNow));
    auto connector = adopt_ref_if_nonnull(new (nothrow) VirtIODisplayConnector(graphics_adapter, move(virtio_virgl3d_uploader_buffer_region))).release_nonnull();
    return connector;
}

VirtIODisplayConnector::VirtIODisplayConnector(VirtIOGPU::GraphicsAdapter& graphics_adapter, NonnullOwnPtr<Memory::Region> virtio_virgl3d_uploader_buffer_region)
    : DisplayConnector()
    , m_graphics_adapter(graphics_adapter)
{
    m_kernel_context_id = m_graphics_adapter.create_context();
}

}
