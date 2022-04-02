/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefPtr.h>
#include <AK/Try.h>
#include <Kernel/Graphics/Bochs/DisplayConnector.h>
#include <Kernel/Graphics/Console/GenericFramebufferConsole.h>
#include <Kernel/Locking/Spinlock.h>
#include <Kernel/Memory/TypedMapping.h>

namespace Kernel {

struct BochsDisplayMMIORegisters;
class QEMUDisplayConnector final
    : public BochsDisplayConnector {
    friend class BochsGraphicsAdapter;
    friend class DeviceManagement;

public:
    TYPEDEF_DISTINCT_ORDERED_ID(u16, IndexID);

    static NonnullRefPtr<QEMUDisplayConnector> must_create(PhysicalAddress framebuffer_address, Memory::TypedMapping<BochsDisplayMMIORegisters volatile>);

    virtual IndexID index_id() const override;

private:
    QEMUDisplayConnector(PhysicalAddress framebuffer_address, Memory::TypedMapping<BochsDisplayMMIORegisters volatile>);

    virtual bool double_framebuffering_capable() const override { return true; }
    virtual ErrorOr<ByteBuffer> get_edid() const override;
    virtual ErrorOr<void> set_resolution(Resolution const&) override;
    virtual ErrorOr<Resolution> get_resolution() override;
    virtual ErrorOr<void> set_y_offset(size_t y) override;
    virtual ErrorOr<void> unblank() override;

    void set_framebuffer_to_big_endian_format();
    void set_framebuffer_to_little_endian_format();

    Memory::TypedMapping<BochsDisplayMMIORegisters volatile> m_registers;
};
}
