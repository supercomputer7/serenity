#include <Kernel/ACPI/DMIDecoder.h>
#include <Kernel/StdLib.h>
#include <Kernel/VM/MemoryManager.h>

static DMIDecoder* s_dmi_decoder;

//#define SMBIOS_DEBUG

#define SMBIOS_BASE_SEARCH_ADDR 0xf0000
#define SMBIOS_END_SEARCH_ADDR 0xfffff
#define SMBIOS_SEARCH_AREA_SIZE (SMBIOS_END_SEARCH_ADDR - SMBIOS_BASE_SEARCH_ADDR)

DMIDecoder& DMIDecoder::the()
{
    if (s_dmi_decoder == nullptr) {
        s_dmi_decoder = new DMIDecoder(true);
    }
    return *s_dmi_decoder;
}

void DMIDecoder::initialize()
{
    if (s_dmi_decoder == nullptr) {
        s_dmi_decoder = new DMIDecoder(true);
    }
}

void DMIDecoder::initialize_untrusted()
{
    if (s_dmi_decoder == nullptr) {
        s_dmi_decoder = new DMIDecoder(false);
    }
}

void DMIDecoder::initialize_parser()
{
    if (m_entry32bit_point != nullptr || m_entry64bit_point != nullptr) {
        m_operable = true;
        kprintf("DMI Decoder is enabled\n");
        if (m_entry64bit_point != nullptr) {
            kprintf("DMIDecoder: SMBIOS 64bit Entry point @ P 0x%x\n", m_entry64bit_point.ptr());
            m_use_64bit_entry = true;
            m_structure_table = OwnPtr<SMBIOS::TableHeader>((SMBIOS::TableHeader*)m_entry64bit_point->table_ptr);
            m_structures_count = m_entry64bit_point->table_maximum_size;
            m_table_length = m_entry64bit_point->table_maximum_size;
        } else if (m_entry32bit_point != nullptr) {
            kprintf("DMIDecoder: SMBIOS 32bit Entry point @ P 0x%x\n", m_entry32bit_point.ptr());
            m_use_64bit_entry = false;
            m_structure_table = OwnPtr<SMBIOS::TableHeader>((SMBIOS::TableHeader*)m_entry32bit_point->legacy_structure.smbios_table_ptr);
            m_structures_count = m_entry32bit_point->legacy_structure.smbios_tables_count;
            m_table_length = m_entry32bit_point->legacy_structure.smboios_table_length;
        }
        kprintf("DMIDecoder: Data table @ P 0x%x\n", m_structure_table.ptr());
        enumerate_smbios_tables();
    } else {
        m_operable = false;
        kprintf("DMI Decoder is disabled. Cannot find SMBIOS tables.\n");
    }
}

void DMIDecoder::enumerate_smbios_tables()
{
    u32 table_length = m_table_length;
    SMBIOS::TableHeader* physical_table = m_structure_table.ptr();

    auto region = MM.allocate_kernel_region(PAGE_ROUND_UP(table_length), "DMI Decoder Entry Point Finding", Region::Access::Read);
    PhysicalAddress paddr = PhysicalAddress((u32)physical_table & PAGE_MASK);
    mmap_region(*region, paddr);

    auto* v_smbios_table = (SMBIOS::TableHeader*)(region->vaddr().get() + ((u32)physical_table & (~PAGE_MASK)));

    u32 structures_count = 0;
    while (table_length > 0) {
#ifdef SMBIOS_DEBUG
        dbgprintf("DMIDecoder: Examining table @ P 0x%x\n", physical_table);
#endif
        structures_count++;
        if (v_smbios_table->type == (u32)SMBIOS::TableType::EndOfTable) {
            kprintf("DMIDecoder: Detected table with type 127, End of SMBIOS data.\n");
            break;
        } else {
            m_smbios_tables.append(physical_table);
            table_length -= v_smbios_table->length;
            kprintf("DMIDecoder: Detected table with type %u\n", v_smbios_table->type);
        }
        SMBIOS::TableHeader* physical_next_table = get_next_physical_table(*physical_table);
#ifdef SMBIOS_DEBUG
        dbgprintf("DMIDecoder: Next table @ P 0x%x\n", physical_next_table);
#endif
        if (physical_next_table == nullptr) {
            break;
        } else {
            v_smbios_table = (SMBIOS::TableHeader*)(region->vaddr().get() + (u32)physical_next_table - paddr.get());
            physical_table = physical_next_table;
        }
    }
    m_structures_count = structures_count;
}

size_t DMIDecoder::get_table_size(SMBIOS::TableHeader& table)
{
    // FIXME: Make sure we have some mapping here so we don't rely on existing identity mapping...
    size_t i;
#ifdef SMBIOS_DEBUG
    dbgprintf("DMIDecoder: table legnth - 0x%x\n", table.length);
#endif
    const char* strtab = (char*)&table + table.length;
    for (i = 1; strtab[i - 1] != '\0' || strtab[i] != '\0'; i++)
        ;
#ifdef SMBIOS_DEBUG
    dbgprintf("DMIDecoder: table size - 0x%x\n", table.length + i + 1);
#endif
    return table.length + i + 1;
}

SMBIOS::TableHeader* DMIDecoder::get_next_physical_table(SMBIOS::TableHeader& p_table)
{
    return (SMBIOS::TableHeader*)((u32)&p_table + get_table_size(p_table));
}

SMBIOS::TableHeader* DMIDecoder::get_smbios_physical_table_by_handle(u16 handle)
{
    auto region = MM.allocate_kernel_region(PAGE_ROUND_UP(PAGE_SIZE * 2), "DMI Decoder Finding Table", Region::Access::Read);

    for (auto* table : m_smbios_tables) {
        if (!table)
            continue;
        PhysicalAddress paddr = PhysicalAddress((u32)table & PAGE_MASK);
        mmap_region(*region, paddr);
        SMBIOS::TableHeader* table_v_ptr = (SMBIOS::TableHeader*)(region->vaddr().get() + ((u32)table & (~PAGE_MASK)));
        if (table_v_ptr->handle == handle) {
            return table;
        }
    }
    return nullptr;
}
SMBIOS::TableHeader* DMIDecoder::get_smbios_physical_table_by_type(u8 table_type)
{
    auto region = MM.allocate_kernel_region(PAGE_ROUND_UP(PAGE_SIZE * 2), "DMI Decoder Finding Table", Region::Access::Read);

    for (auto* table : m_smbios_tables) {
        if (!table)
            continue;
        PhysicalAddress paddr = PhysicalAddress((u32)table & PAGE_MASK);
        mmap_region(*region, paddr);
        SMBIOS::TableHeader* table_v_ptr = (SMBIOS::TableHeader*)(region->vaddr().get() + ((u32)table & (~PAGE_MASK)));
        if (table_v_ptr->type == table_type) {
            return table;
        }
    }
    return nullptr;
}

DMIDecoder::DMIDecoder(bool trusted)
    : m_entry32bit_point(find_entry32bit_point())
    , m_entry64bit_point(find_entry64bit_point())
    , m_untrusted(!trusted)
{
    if (!trusted) {
        kprintf("DMI Decoder initialized as untrusted due to user request.\n");
    }
    initialize_parser();
}

OwnPtr<SMBIOS::EntryPoint64bit> DMIDecoder::find_entry64bit_point()
{
    auto region = MM.allocate_kernel_region(PAGE_ROUND_UP(SMBIOS_SEARCH_AREA_SIZE), "DMI Decoder Entry Point Finding", Region::Access::Read);
    PhysicalAddress paddr = PhysicalAddress(SMBIOS_BASE_SEARCH_ADDR);
    mmap_region(*region, paddr);

    char* tested_physical_ptr = (char*)paddr.get();
    for (char* entry_str = (char*)(region->vaddr().get()); entry_str < (char*)(region->vaddr().get() + (SMBIOS_SEARCH_AREA_SIZE)); entry_str += 16) {
#ifdef SMBIOS_DEBUG
        dbgprintf("DMI Decoder: Looking for 64 bit Entry point @ P 0x%x\n", entry_str, tested_physical_ptr);
#endif
        if (!strncmp("_SM3_", entry_str, strlen("_SM3_")))
            return OwnPtr<SMBIOS::EntryPoint64bit>((SMBIOS::EntryPoint64bit*)tested_physical_ptr);

        tested_physical_ptr += 16;
    }
    return nullptr;
}

OwnPtr<SMBIOS::EntryPoint32bit> DMIDecoder::find_entry32bit_point()
{
    auto region = MM.allocate_kernel_region(PAGE_ROUND_UP(SMBIOS_SEARCH_AREA_SIZE), "DMI Decoder Entry Point Finding", Region::Access::Read);
    PhysicalAddress paddr = PhysicalAddress(SMBIOS_BASE_SEARCH_ADDR);
    mmap_region(*region, paddr);

    char* tested_physical_ptr = (char*)paddr.get();
    for (char* entry_str = (char*)(region->vaddr().get()); entry_str < (char*)(region->vaddr().get() + (SMBIOS_SEARCH_AREA_SIZE)); entry_str += 16) {
#ifdef SMBIOS_DEBUG
        dbgprintf("DMI Decoder: Looking for 32 bit Entry point @ P 0x%x\n", tested_physical_ptr);
#endif
        if (!strncmp("_SM_", entry_str, strlen("_SM_")))
            return OwnPtr<SMBIOS::EntryPoint32bit>((SMBIOS::EntryPoint32bit*)tested_physical_ptr);

        tested_physical_ptr += 16;
    }
    return nullptr;
}

void DMIDecoder::mmap(VirtualAddress vaddr, PhysicalAddress paddr, u32 length)
{
    unsigned i = 0;
    while (length >= PAGE_SIZE) {
        MM.map_for_kernel(VirtualAddress(vaddr.offset(i * PAGE_SIZE).get()), PhysicalAddress(paddr.offset(i * PAGE_SIZE).get()));
#ifdef SMBIOS_DEBUG
        dbgprintf("DMI Decoder: map - V 0x%x -> P 0x%x\n", vaddr.offset(i * PAGE_SIZE).get(), paddr.offset(i * PAGE_SIZE).get());
#endif
        length -= PAGE_SIZE;
        i++;
    }
    if (length > 0) {
        MM.map_for_kernel(vaddr.offset(i * PAGE_SIZE), paddr.offset(i * PAGE_SIZE), true);
    }
#ifdef SMBIOS_DEBUG
    dbgprintf("DMI Decoder: Finished mapping\n");
#endif
}

void DMIDecoder::mmap_region(Region& region, PhysicalAddress paddr)
{
#ifdef SMBIOS_DEBUG
    dbgprintf("DMI Decoder: Mapping region, size - %u\n", region.size());
#endif
    mmap(region.vaddr(), paddr, region.size());
}

Vector<SMBIOS::PhysicalMemoryArray*>& DMIDecoder::get_physical_memory_areas()
{
    // FIXME: Implement it...
    kprintf("DMIDecoder::get_physical_memory_areas() is not implemented.\n");
    ASSERT_NOT_REACHED();
}
bool DMIDecoder::is_reliable()
{
    return !m_untrusted;
}
u64 DMIDecoder::get_bios_characteristics()
{
    // FIXME: Make sure we have some mapping here so we don't rely on existing identity mapping...
    ASSERT(m_operable == true);
    SMBIOS::BIOSInfo* bios_info = (SMBIOS::BIOSInfo*)get_smbios_physical_table_by_type(0);
    ASSERT(bios_info != nullptr);
    kprintf("DMIDecoder: BIOS info @ P 0x%x\n", bios_info);
    return bios_info->bios_characteristics;
}

char* DMIDecoder::get_smbios_string(SMBIOS::TableHeader&, u8)
{
    // FIXME: Implement it...
    // FIXME: Make sure we have some mapping here so we don't rely on existing identity mapping...
    kprintf("DMIDecoder::get_smbios_string() is not implemented.\n");
    ASSERT_NOT_REACHED();
    return nullptr;
}
