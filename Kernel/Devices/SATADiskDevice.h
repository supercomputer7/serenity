//
// A Disk Device Connected to a SATA Channel
//
//
#pragma once

#include <Kernel/DeviceIRQHandler.h>
#include <Kernel/Devices/DiskDevice.h>
#include <Kernel/Lock.h>

class SATAChannel;

class SATADiskDevice final : public DiskDevice {
    AK_MAKE_ETERNAL

public:
    static NonnullRefPtr<SATADiskDevice> create(SATAChannel&, int major, int minor);
    virtual ~SATADiskDevice() override;

    // ^DiskDevice
    virtual bool read_block(unsigned index, u8*) const override;
    virtual bool write_block(unsigned index, const u8*) override;
    virtual bool read_blocks(unsigned index, u16 count, u8*) override;
    virtual bool write_blocks(unsigned index, u16 count, const u8*) override;

    void set_drive_geometry(u16, u16, u16);

    // ^BlockDevice
    virtual ssize_t read(FileDescription&, u8*, ssize_t) override { return 0; }
    virtual bool can_read(const FileDescription&) const override { return true; }
    virtual ssize_t write(FileDescription&, const u8*, ssize_t) override { return 0; }
    virtual bool can_write(const FileDescription&) const override { return true; }

protected:
    explicit SATADiskDevice(SATAChannel&, int, int);

private:
    // ^DiskDevice
    virtual const char* class_name() const override;

    bool wait_for_irq();
    bool read_sectors(u32 lba, u16 count, u8* buffer);
    bool write_sectors(u32 lba, u16 count, const u8* data);

    Lock m_lock { "AHCIDiskDevice" };

    SATAChannel& m_channel;
};
