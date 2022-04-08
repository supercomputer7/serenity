/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Graphics/Intel/VirtualMemory/AddressSpace.h>

namespace Kernel::IntelGraphics {

ErrorOr<NonnullOwnPtr<AddressSpace>> AddressSpace::create(Generation generation, Memory::TypedMapping<volatile u32> address_space_page_table_entries_mapping)
{
    return adopt_nonnull_own_or_enomem(new (nothrow) AddressSpace(generation, move(address_space_page_table_entries_mapping)));
}

AddressSpace::AddressSpace(Generation generation, Memory::TypedMapping<volatile u32> address_space_page_table_entries_mapping)
    : m_generation(generation)
    , m_address_space_page_table_entries(move(address_space_page_table_entries_mapping))
{
}

ErrorOr<GPUVirtualAddress> AddressSpace::map_aperture_with_physical_address(PhysicalAddress paddr)
{
    // Note: Intel graphics Gen4 chipsets seem to not need our code to handle any sort
    // of virtual memory mappings for DMA and apertures, so tell everything is mapped 1:1.
    if (m_generation == IntelGraphics::Generation::Gen4) {
        return GPUVirtualAddress(paddr.get());
    }
    VERIFY_NOT_REACHED();
}

}
