#pragma once

#include <ACPI/ACPIParser.h>

class ACPIStaticParser : ACPIParser {
public:
    static void initialize(ACPI_RAW::RSDPDescriptor20& rsdp);
    static void initialize_without_rsdp();
    static bool is_initialized();

    virtual ACPI_RAW::SDTHeader* find_table(const char* sig) override;
    virtual void do_acpi_reboot() override;
    virtual void do_acpi_shutdown() override;
    virtual bool is_operable() override { return m_operable; }

protected:
    ACPIStaticParser();
    explicit ACPIStaticParser(ACPI_RAW::RSDPDescriptor20&);

    virtual void mmap(VirtualAddress preferred_vaddr, PhysicalAddress paddr, u32) override;
    virtual void mmap_region(Region& region, PhysicalAddress paddr) override;

private:
    void locate_static_data();
    void locate_all_aml_tables();
    void locate_main_system_description_table();
    void initialize_main_system_description_table();
    ACPI_RAW::SDTHeader& find_fadt();
    void init_fadt();
    OwnPtr<ACPI_RAW::RSDPDescriptor20> search_rsdp();

    // Early pointers that are needed really for initializtion only...
    OwnPtr<ACPI_RAW::RSDPDescriptor20> m_rsdp;
    OwnPtr<ACPI_RAW::SDTHeader> m_main_system_description_table;

    RefPtr<ACPI::MainSystemDescriptionTable> m_main_sdt;
    RefPtr<ACPI::FixedACPIData> m_fadt;
    RefPtr<ACPI::MCFG> m_mcfg;
    RefPtr<ACPI::MADT> m_madt;

    Vector<ACPI_RAW::SDTHeader*>& m_aml_tables_ptrs;
    bool m_xsdt_supported;
};