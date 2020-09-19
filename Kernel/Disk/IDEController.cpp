#include <Kernel/Disk/IDEController.h>
#include <Kernel/Disk/StorageDevice.h>

namespace Kernel {

#define PATA_PRIMARY_IRQ 14
#define PATA_SECONDARY_IRQ 15

//#define PATA_DEBUG

#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX 0x02
#define ATA_SR_ERR 0x01

#define ATA_ER_BBK 0x80
#define ATA_ER_UNC 0x40
#define ATA_ER_MC 0x20
#define ATA_ER_IDNF 0x10
#define ATA_ER_MCR 0x08
#define ATA_ER_ABRT 0x04
#define ATA_ER_TK0NF 0x02
#define ATA_ER_AMNF 0x01

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_READ_PIO_EXT 0x24
#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET 0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY 0xEC

#define ATAPI_CMD_READ 0xA8
#define ATAPI_CMD_EJECT 0x1B

#define ATA_IDENT_DEVICETYPE 0
#define ATA_IDENT_CYLINDERS 2
#define ATA_IDENT_HEADS 6
#define ATA_IDENT_SECTORS 12
#define ATA_IDENT_SERIAL 20
#define ATA_IDENT_MODEL 54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID 106
#define ATA_IDENT_MAX_LBA 120
#define ATA_IDENT_COMMANDSETS 164
#define ATA_IDENT_MAX_LBA_EXT 200

#define IDE_ATA 0x00
#define IDE_ATAPI 0x01

#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_FEATURES 0x01
#define ATA_REG_SECCOUNT0 0x02
#define ATA_REG_LBA0 0x03
#define ATA_REG_LBA1 0x04
#define ATA_REG_LBA2 0x05
#define ATA_REG_HDDEVSEL 0x06
#define ATA_REG_COMMAND 0x07
#define ATA_REG_STATUS 0x07
#define ATA_CTL_CONTROL 0x00
#define ATA_CTL_ALTSTATUS 0x00
#define ATA_CTL_DEVADDRESS 0x01

#define PCI_Mass_Storage_Class 0x1
#define PCI_IDE_Controller_Subclass 0x1

void IDEController::initialize()
{
    ASSERT(!is_operational());
}
bool IDEController::write(const StorageDevice& device, TransferMode, ProtocolMode, u64 block, u16 count, const u8* inbuf) const
{
    // Assert if the StorageDevice somehow doesn't belong to this controller.
    ASSERT(device.controller().ptr() == this);
    // Assert if the StorageDevice somehow is not an IDE attached storage device.
    ASSERT(device.type() == DiskDeviceType::PATA || device.type() == DiskDeviceType::PATAPI);
}
bool IDEController::read(const StorageDevice&, TransferMode, ProtocolMode, u64 block, u16 count, u8* outbuf) const
{
}
bool IDEController::eject(const StorageDevice&) const
{
}
bool IDEController::reset_device(const StorageDevice&) const
{
}
bool IDEController::reset()
{
    ASSERT(!is_operational());
    shutdown();
    initialize();
}
bool IDEController::shutdown()
{
    m_connected_storage.clear();
}
bool IDEController::is_operational() const
{
    return !m_connected_storage.is_empty();
}

void IDEController::detect_disks(u8 minor_base, IOAddress io_base)
{
    // There are only two possible disks connected to a channel
    for (auto i = 0; i < 2; i++) {
        io_base.offset(ATA_REG_HDDEVSEL).out<u8>(0xA0 | (i << 4)); // First, we need to select the drive itself

        // Apparently these need to be 0 before sending IDENTIFY?!
        io_base.offset(ATA_REG_SECCOUNT0).out<u8>(0x00);
        io_base.offset(ATA_REG_LBA0).out<u8>(0x00);
        io_base.offset(ATA_REG_LBA1).out<u8>(0x00);
        io_base.offset(ATA_REG_LBA2).out<u8>(0x00);

        io_base.offset(ATA_REG_COMMAND).out<u8>(ATA_CMD_IDENTIFY); // Send the ATA_IDENTIFY command

        // Wait for the BSY flag to be reset
        while (io_base.offset(ATA_REG_STATUS).in<u8>() & ATA_SR_BSY)
            ;

        if (io_base.offset(ATA_REG_STATUS).in<u8>() == 0x00) {
#ifdef PATA_DEBUG
            klog() << "IDEController: No " << (i == 0 ? "master" : "slave") << " disk detected!";
#endif
            continue;
        }

        ByteBuffer wbuf = ByteBuffer::create_uninitialized(512);
        ByteBuffer bbuf = ByteBuffer::create_uninitialized(512);
        u8* b = bbuf.data();
        u16* w = (u16*)wbuf.data();
        const u16* wbufbase = (u16*)wbuf.data();

        for (u32 i = 0; i < 256; ++i) {
            u16 data = io_base.offset(ATA_REG_DATA).in<u16>();
            *(w++) = data;
            *(b++) = MSB(data);
            *(b++) = LSB(data);
        }

        // "Unpad" the device name string.
        for (u32 i = 93; i > 54 && bbuf[i] == ' '; --i)
            bbuf[i] = 0;

        size_t sector_size;
        u8 sectors_data = wbufbase[106];
        if (sectors_data & (1 << 14)) {
            if (sectors_data & (1 << 12))
                sector_size = (wbufbase[117] | (wbufbase[118] << 16)) * 2;
        } else {
            sector_size = 512;
        }
        u64 max_block = wbufbase[100] | wbufbase[101] << 16 | wbufbase[102] << 32 | wbufbase[103] << 48;
        klog() << "IDEController: Name=" << ((char*)bbuf.data() + 54) << ", Sector Size=" << sector_size << ", Max Sector Size=" << max_block;

        /*int major = (m_channel_number == 0) ? 3 : 4;
        if (i == 0) {
            m_master = PATADiskDevice::create(*this, PATADiskDevice::DriveType::Master, major, 0);
            m_master->set_drive_geometry(cyls, heads, spt);
        } else {
            m_slave = PATADiskDevice::create(*this, PATADiskDevice::DriveType::Slave, major, 1);
            m_slave->set_drive_geometry(cyls, heads, spt);
        }*/
    }
}

void IDEController::detect_disks()
{
    if (m_primary_io_base.is_valid())
        detect_disks(0, m_primary_io_base);
    if (m_secondary_io_base.is_valid())
        detect_disks(2, m_secondary_io_base);
}

void IDEController::wait_for_irq()
{
    Thread::current()->wait_on(m_irq_queue, "IDEController");
    disable_irq();
}

void IDEController::handle_irq(const RegisterState& regs)
{
    regs.isr_number
    u8 status = m_io_base.offset(ATA_REG_STATUS).in<u8>();

    m_entropy_source.add_random_event(status);

    u8 bstatus = m_bus_master_base.offset(2).in<u8>();
    if (!(bstatus & 0x4)) {
        // interrupt not from this device, ignore
#ifdef PATA_DEBUG
        klog() << "IDEController: ignore interrupt";
#endif
        return;
    }

    if (status & ATA_SR_ERR) {
        print_ide_status(status);
        m_device_error = m_io_base.offset(ATA_REG_ERROR).in<u8>();
        klog() << "IDEController: Error " << String::format("%b", m_device_error) << "!";
    } else {
        m_device_error = 0;
    }
#ifdef PATA_DEBUG
    klog() << "IDEController: interrupt: DRQ=" << ((status & ATA_SR_DRQ) != 0) << " BSY=" << ((status & ATA_SR_BSY) != 0) << " DRDY=" << ((status & ATA_SR_DRDY) != 0);
#endif
    m_irq_queue.wake_all();
}

bool IDEController::ata_read_sectors_with_dma(u32 lba, u16 count, u8* outbuf, bool slave_request)
{
    LOCKER(s_lock());
#ifdef PATA_DEBUG
    dbg() << "IDEController::ata_read_sectors_with_dma (" << lba << " x" << count << ") -> " << outbuf;
#endif

    prdt().offset = m_dma_buffer_page->paddr();
    prdt().size = 512 * count;

    ASSERT(prdt().size <= PAGE_SIZE);

    // Stop bus master
    m_bus_master_base.out<u8>(0);

    // Write the PRDT location
    m_bus_master_base.offset(4).out(m_prdt_page->paddr().get());

    // Turn on "Interrupt" and "Error" flag. The error flag should be cleared by hardware.
    m_bus_master_base.offset(2).out<u8>(m_bus_master_base.offset(2).in<u8>() | 0x6);

    // Set transfer direction
    m_bus_master_base.out<u8>(0x8);

    while (m_io_base.offset(ATA_REG_STATUS).in<u8>() & ATA_SR_BSY)
        ;

    m_control_base.offset(ATA_CTL_CONTROL).out<u8>(0);
    m_io_base.offset(ATA_REG_HDDEVSEL).out<u8>(0x40 | (static_cast<u8>(slave_request) << 4));
    io_delay();

    m_io_base.offset(ATA_REG_FEATURES).out<u16>(0);

    m_io_base.offset(ATA_REG_SECCOUNT0).out<u8>(0);
    m_io_base.offset(ATA_REG_LBA0).out<u8>(0);
    m_io_base.offset(ATA_REG_LBA1).out<u8>(0);
    m_io_base.offset(ATA_REG_LBA2).out<u8>(0);

    m_io_base.offset(ATA_REG_SECCOUNT0).out<u8>(count);
    m_io_base.offset(ATA_REG_LBA0).out<u8>((lba & 0x000000ff) >> 0);
    m_io_base.offset(ATA_REG_LBA1).out<u8>((lba & 0x0000ff00) >> 8);
    m_io_base.offset(ATA_REG_LBA2).out<u8>((lba & 0x00ff0000) >> 16);

    for (;;) {
        auto status = m_io_base.offset(ATA_REG_STATUS).in<u8>();
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY))
            break;
    }

    m_io_base.offset(ATA_REG_COMMAND).out<u8>(ATA_CMD_READ_DMA_EXT);
    io_delay();

    prepare_for_irq();
    // Start bus master
    m_bus_master_base.out<u8>(0x9);

    wait_for_irq();

    if (m_device_error)
        return false;

    memcpy(outbuf, m_dma_buffer_page->paddr().offset(0xc0000000).as_ptr(), 512 * count);

    // I read somewhere that this may trigger a cache flush so let's do it.
    m_bus_master_base.offset(2).out<u8>(m_bus_master_base.offset(2).in<u8>() | 0x6);
    return true;
}

bool IDEController::ata_write_sectors_with_dma(u32 lba, u16 count, const u8* inbuf, bool slave_request)
{
    LOCKER(s_lock());
#ifdef PATA_DEBUG
    dbg() << "IDEController::ata_write_sectors_with_dma (" << lba << " x" << count << ") <- " << inbuf;
#endif

    prdt().offset = m_dma_buffer_page->paddr();
    prdt().size = 512 * count;

    memcpy(m_dma_buffer_page->paddr().offset(0xc0000000).as_ptr(), inbuf, 512 * count);

    ASSERT(prdt().size <= PAGE_SIZE);

    // Stop bus master
    m_bus_master_base.out<u8>(0);

    // Write the PRDT location
    m_bus_master_base.offset(4).out<u32>(m_prdt_page->paddr().get());

    // Turn on "Interrupt" and "Error" flag. The error flag should be cleared by hardware.
    m_bus_master_base.offset(2).out<u8>(m_bus_master_base.offset(2).in<u8>() | 0x6);

    while (m_io_base.offset(ATA_REG_STATUS).in<u8>() & ATA_SR_BSY)
        ;

    m_control_base.offset(ATA_CTL_CONTROL).out<u8>(0);
    m_io_base.offset(ATA_REG_HDDEVSEL).out<u8>(0x40 | (static_cast<u8>(slave_request) << 4));
    io_delay();

    m_io_base.offset(ATA_REG_FEATURES).out<u16>(0);

    m_io_base.offset(ATA_REG_SECCOUNT0).out<u8>(0);
    m_io_base.offset(ATA_REG_LBA0).out<u8>(0);
    m_io_base.offset(ATA_REG_LBA1).out<u8>(0);
    m_io_base.offset(ATA_REG_LBA2).out<u8>(0);

    m_io_base.offset(ATA_REG_SECCOUNT0).out<u8>(count);
    m_io_base.offset(ATA_REG_LBA0).out<u8>((lba & 0x000000ff) >> 0);
    m_io_base.offset(ATA_REG_LBA1).out<u8>((lba & 0x0000ff00) >> 8);
    m_io_base.offset(ATA_REG_LBA2).out<u8>((lba & 0x00ff0000) >> 16);

    for (;;) {
        auto status = m_io_base.offset(ATA_REG_STATUS).in<u8>();
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY))
            break;
    }

    m_io_base.offset(ATA_REG_COMMAND).out<u8>(ATA_CMD_WRITE_DMA_EXT);
    io_delay();

    prepare_for_irq();
    // Start bus master
    m_bus_master_base.out<u8>(0x1);
    wait_for_irq();

    if (m_device_error)
        return false;

    // I read somewhere that this may trigger a cache flush so let's do it.
    m_bus_master_base.offset(2).out<u8>(m_bus_master_base.offset(2).in<u8>() | 0x6);
    return true;
}
}
