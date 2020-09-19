#pragma once

#include <AK/OwnPtr.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <AK/Vector.h>
#include <Kernel/Devices/BlockDevice.h>
#include <Kernel/Disk/Devices/ATADevice.h>
#include <Kernel/Disk/StorageController.h>
#include <Kernel/IO.h>
#include <Kernel/Lock.h>
#include <Kernel/PhysicalAddress.h>
#include <Kernel/Random.h>
#include <Kernel/VM/PhysicalPage.h>
#include <Kernel/WaitQueue.h>

namespace Kernel {

struct PhysicalRegionDescriptor {
    PhysicalAddress offset;
    u16 size { 0 };
    u16 end_of_table { 0 };
};

class IDEController : public StorageController {
public:
    static NonnullRefPtr<IDEController> create(PCI::Address address);

    virtual void initialize() override;

    virtual bool write(const StorageDevice&, TransferMode, ProtocolMode, u64 block, u16 count, const UserOrKernelBuffer& inbuf) const override;
    virtual bool read(const StorageDevice&, TransferMode, ProtocolMode, u64 block, u16 count, UserOrKernelBuffer& outbuf) const override;
    virtual bool eject(const StorageDevice&) const override;
    virtual bool reset_device(const StorageDevice&) const override;

    virtual bool reset() override;
    virtual bool shutdown() override;
    virtual bool is_operational() const override;

protected:
    // FIXME: I assume that all disk controllers are enumerated over PCI and we
    // use the PIC for now. This is very possible to find old computers with ISA
    // IDE controllers that are not enumerated with PCI. We could use the ACPI
    // namespace to find those controllers, or to try to probe them. Once we
    // have SMP working (with AML being parsed + getting IRQ redirections), we
    // will need to change this a bit again, to ensure that IRQs are handled
    // properly.

    explicit IDEController(PCI::Address address)
        : StorageController(address)
    {
    }

    void detect_disks();
    void detect_disks(u8 minor_base, IOAddress io_base);

    void wait_for_irq();
    bool ata_read_sectors_with_dma(u32, u16, u8*, bool);
    bool ata_write_sectors_with_dma(u32, u16, const u8*, bool);
    bool ata_read_sectors(u32, u16, u8*, bool);
    bool ata_write_sectors(u32, u16, const u8*, bool);

    inline void prepare_for_irq();

    Vector<RefPtr<StorageDevice>> m_connected_storage;
    IOAddress m_primary_io_base;
    IOAddress m_primary_control_base;
    IOAddress m_secondary_io_base;
    IOAddress m_secondary_control_base;    
    IOAddress m_bus_master_base;
    volatile u8 m_device_error { 0 };

    WaitQueue m_irq_queue;

    PhysicalRegionDescriptor& prdt() { return *reinterpret_cast<PhysicalRegionDescriptor*>(m_prdt_page->paddr().offset(0xc0000000).as_ptr()); }
    RefPtr<PhysicalPage> m_prdt_page;
    RefPtr<PhysicalPage> m_dma_buffer_page;
    Lockable<bool> m_dma_enabled;
    EntropySource m_entropy_source;
};
}
