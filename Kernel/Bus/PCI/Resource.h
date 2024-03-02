/*
 * Copyright (c) 2023, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Memory/PhysicalAddress.h>

namespace Kernel::PCI {

static constexpr u64 bar_address_mask = ~0xfull;

struct Resource {
    enum class SpaceType {
        IOSpace,
        Memory16BitSpace,
        Memory32BitSpace,
        Memory64BitSpace,
    };
    u64 bar_value;
    SpaceType type;
    size_t length;

    PhysicalAddress physical_memory_address() const
    {
        VERIFY(type != SpaceType::IOSpace);
        return PhysicalAddress(bar_value & PCI::bar_address_mask);
    }
};

}
