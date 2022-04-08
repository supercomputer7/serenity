/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefPtr.h>
#include <AK/Try.h>
#include <AK/Types.h>
#include <Kernel/Graphics/Intel/Definitions.h>
#include <Kernel/Graphics/Intel/VirtualMemory/GPUVirtualAddress.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/PhysicalAddress.h>

namespace Kernel::IntelGraphics {

class AddressSpace {
public:
    static ErrorOr<NonnullOwnPtr<AddressSpace>> create(Generation, Memory::TypedMapping<volatile u32>);

    ErrorOr<GPUVirtualAddress> map_aperture_with_physical_address(PhysicalAddress);

private:
    AddressSpace(Generation, Memory::TypedMapping<volatile u32>);

    const Generation m_generation;
    Memory::TypedMapping<volatile u32> m_address_space_page_table_entries;
};
}
