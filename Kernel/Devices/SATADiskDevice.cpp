#include <Kernel/Devices/SATAChannel.h>
#include <Kernel/Devices/SATADiskDevice.h>

NonnullRefPtr<SATADiskDevice> SATADiskDevice::create(SATAChannel& channel, int major, int minor)
{
    return adopt(*new SATADiskDevice(channel, major, minor));
}

SATADiskDevice::SATADiskDevice(SATAChannel& channel, int major, int minor)
    : DiskDevice(major, minor)
    , m_channel(channel)
{
}

SATADiskDevice::~SATADiskDevice()
{
}

const char* SATADiskDevice::class_name() const
{
    return "SATADiskDevice";
}

bool SATADiskDevice::read_blocks(unsigned index, u16 count, u8* out)
{
    return read_sectors(index, count, out);
}

bool SATADiskDevice::read_block(unsigned index, u8* out) const
{
    return const_cast<SATADiskDevice*>(this)->read_blocks(index, 1, out);
}

bool SATADiskDevice::write_blocks(unsigned index, u16 count, const u8* data)
{
    return write_sectors(index, count, data);
}

bool SATADiskDevice::write_block(unsigned index, const u8* data)
{
    return write_blocks(index, 1, data);
}

bool SATADiskDevice::read_sectors(u32 start_sector, u16 count, u8* outbuf)
{
    return m_channel.ata_read_sectors(start_sector, count, outbuf);
}

bool SATADiskDevice::write_sectors(u32 start_sector, u16 count, const u8* inbuf)
{
    return m_channel.ata_write_sectors(start_sector, count, inbuf);
}
