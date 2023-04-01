/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/API/Ioctl.h>
#include <Kernel/Devices/GPU/DisplayConnector.h>
#include <Kernel/Devices/GPU/Management.h>
#include <Kernel/FileSystem/SysFS/Subsystems/DeviceIdentifiers/CharacterDevicesDirectory.h>
#include <Kernel/FileSystem/SysFS/Subsystems/Devices/GPU/DisplayConnector/DeviceDirectory.h>
#include <Kernel/FileSystem/SysFS/Subsystems/Devices/GPU/DisplayConnector/Directory.h>
#include <Kernel/Memory/MemoryManager.h>

namespace Kernel {

DisplayConnector::DisplayConnector(PhysicalAddress framebuffer_address, size_t framebuffer_resource_size, bool enable_write_combine_optimization)
    : CharacterDevice(226, GPUManagement::the().allocate_minor_device_number())
    , m_enable_write_combine_optimization(enable_write_combine_optimization)
    , m_framebuffer_at_arbitrary_physical_range(false)
    , m_framebuffer_address(framebuffer_address)
    , m_framebuffer_resource_size(framebuffer_resource_size)
{
}

DisplayConnector::DisplayConnector(size_t framebuffer_resource_size, bool enable_write_combine_optimization)
    : CharacterDevice(226, GPUManagement::the().allocate_minor_device_number())
    , m_enable_write_combine_optimization(enable_write_combine_optimization)
    , m_framebuffer_at_arbitrary_physical_range(true)
    , m_framebuffer_address({})
    , m_framebuffer_resource_size(framebuffer_resource_size)
{
}

ErrorOr<NonnullLockRefPtr<Memory::VMObject>> DisplayConnector::vmobject_for_mmap(Process&, Memory::VirtualRange const&, u64& offset, bool)
{
    VERIFY(m_shared_framebuffer_vmobject);
    if (offset != 0)
        return Error::from_errno(ENOTSUP);

    return *m_shared_framebuffer_vmobject;
}

ErrorOr<size_t> DisplayConnector::read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t)
{
    return Error::from_errno(ENOTIMPL);
}

ErrorOr<size_t> DisplayConnector::write(OpenFileDescription&, u64, UserOrKernelBuffer const&, size_t)
{
    return Error::from_errno(ENOTIMPL);
}

void DisplayConnector::will_be_destroyed()
{
    GPUManagement::the().detach_display_connector({}, *this);

    // NOTE: We check if m_symlink_sysfs_component is not null, because if we failed
    // at some point in DisplayConnector::after_inserting(), then that method will tear down
    // the object internal members safely, so we don't want to do it again here.
    if (m_symlink_sysfs_component) {
        before_will_be_destroyed_remove_symlink_from_device_identifier_directory();
        m_symlink_sysfs_component.clear();
    }

    // NOTE: We check if m_sysfs_device_directory is not null, because if we failed
    // at some point in DisplayConnector::after_inserting(), then that method will tear down
    // the object internal members safely, so we don't want to do it again here.
    if (m_sysfs_device_directory) {
        SysFSDisplayConnectorsDirectory::the().unplug({}, *m_sysfs_device_directory);
        m_sysfs_device_directory.clear();
    }

    before_will_be_destroyed_remove_from_device_management();
}

ErrorOr<void> DisplayConnector::allocate_framebuffer_resources(size_t rounded_size)
{
    VERIFY((rounded_size % PAGE_SIZE) == 0);
    if (!m_framebuffer_at_arbitrary_physical_range) {
        VERIFY(m_framebuffer_address.value().page_base() == m_framebuffer_address.value());
        m_shared_framebuffer_vmobject = TRY(Memory::SharedFramebufferVMObject::try_create_for_physical_range(m_framebuffer_address.value(), rounded_size));
        m_framebuffer_region = TRY(MM.allocate_kernel_region(m_framebuffer_address.value().page_base(), rounded_size, "Framebuffer"sv, Memory::Region::Access::ReadWrite));
    } else {
        m_shared_framebuffer_vmobject = TRY(Memory::SharedFramebufferVMObject::try_create_at_arbitrary_physical_range(rounded_size));
        m_framebuffer_region = TRY(MM.allocate_kernel_region_with_vmobject(m_shared_framebuffer_vmobject->real_writes_framebuffer_vmobject(), rounded_size, "Framebuffer"sv, Memory::Region::Access::ReadWrite));
    }

    m_framebuffer_data = m_framebuffer_region->vaddr().as_ptr();
    m_fake_writes_framebuffer_region = TRY(MM.allocate_kernel_region_with_vmobject(m_shared_framebuffer_vmobject->fake_writes_framebuffer_vmobject(), rounded_size, "Fake Writes Framebuffer"sv, Memory::Region::Access::ReadWrite));
    return {};
}

ErrorOr<void> DisplayConnector::after_inserting()
{
    after_inserting_add_to_device_management();
    ArmedScopeGuard clean_from_device_management([&] {
        before_will_be_destroyed_remove_from_device_management();
    });

    auto sysfs_display_connector_device_directory = DisplayConnectorSysFSDirectory::create(SysFSDisplayConnectorsDirectory::the(), *this);
    m_sysfs_device_directory = sysfs_display_connector_device_directory;
    SysFSDisplayConnectorsDirectory::the().plug({}, *sysfs_display_connector_device_directory);
    ArmedScopeGuard clean_from_sysfs_display_connector_device_directory([&] {
        SysFSDisplayConnectorsDirectory::the().unplug({}, *m_sysfs_device_directory);
        m_sysfs_device_directory.clear();
    });

    VERIFY(!m_symlink_sysfs_component);
    auto sys_fs_component = TRY(SysFSSymbolicLinkDeviceComponent::try_create(SysFSCharacterDevicesDirectory::the(), *this, *m_sysfs_device_directory));
    m_symlink_sysfs_component = sys_fs_component;
    after_inserting_add_symlink_to_device_identifier_directory();
    ArmedScopeGuard clean_symlink_to_device_identifier_directory([&] {
        VERIFY(m_symlink_sysfs_component);
        before_will_be_destroyed_remove_symlink_from_device_identifier_directory();
        m_symlink_sysfs_component.clear();
    });

    if (auto result_or_error = Memory::page_round_up(m_framebuffer_resource_size); result_or_error.is_error()) {
        // NOTE: The amount of framebuffer resource being specified is erroneous, then default to 16 MiB.
        TRY(allocate_framebuffer_resources(16 * MiB));
        m_framebuffer_resource_size = 16 * MiB;
    } else {
        if (auto allocation_result = allocate_framebuffer_resources(result_or_error.release_value()); allocation_result.is_error()) {
            // NOTE: The amount of framebuffer resource being specified is too big, use 16 MiB just to get going.
            TRY(allocate_framebuffer_resources(16 * MiB));
            m_framebuffer_resource_size = 16 * MiB;
        }
    }

    clean_from_device_management.disarm();
    clean_from_sysfs_display_connector_device_directory.disarm();
    clean_symlink_to_device_identifier_directory.disarm();

    GPUManagement::the().attach_new_display_connector({}, *this);
    if (m_enable_write_combine_optimization) {
        [[maybe_unused]] auto result = m_framebuffer_region->set_write_combine(true);
    }
    return {};
}

bool DisplayConnector::console_mode() const
{
    VERIFY(m_control_lock.is_locked());
    return m_console_mode;
}

void DisplayConnector::set_display_mode(Badge<GPUManagement>, DisplayMode mode)
{
    SpinlockLocker locker(m_control_lock);

    {
        SpinlockLocker locker(m_modeset_lock);
        [[maybe_unused]] auto result = set_y_offset(0);
    }

    m_console_mode = mode == DisplayMode::Console ? true : false;
    if (m_console_mode) {
        VERIFY(m_framebuffer_region->size() == m_fake_writes_framebuffer_region->size());
        memcpy(m_fake_writes_framebuffer_region->vaddr().as_ptr(), m_framebuffer_region->vaddr().as_ptr(), m_framebuffer_region->size());
        m_shared_framebuffer_vmobject->switch_to_fake_sink_framebuffer_writes({});
        enable_console();
    } else {
        disable_console();
        m_shared_framebuffer_vmobject->switch_to_real_framebuffer_writes({});
        VERIFY(m_framebuffer_region->size() == m_fake_writes_framebuffer_region->size());
        memcpy(m_framebuffer_region->vaddr().as_ptr(), m_fake_writes_framebuffer_region->vaddr().as_ptr(), m_framebuffer_region->size());
    }
}

ErrorOr<void> DisplayConnector::initialize_edid_for_generic_monitor(Optional<Array<u8, 3>> possible_manufacturer_id_string)
{
    u8 raw_manufacturer_id[2] = { 0x0, 0x0 };
    if (possible_manufacturer_id_string.has_value()) {
        Array<u8, 3> manufacturer_id_string = possible_manufacturer_id_string.release_value();
        u8 byte1 = (((static_cast<u8>(manufacturer_id_string[0]) - '@') & 0x1f) << 2) | (((static_cast<u8>(manufacturer_id_string[1]) - '@') >> 3) & 3);
        u8 byte2 = ((static_cast<u8>(manufacturer_id_string[2]) - '@') & 0x1f) | (((static_cast<u8>(manufacturer_id_string[1]) - '@') << 5) & 0xe0);
        Array<u8, 2> manufacturer_id_string_packed_bytes = { byte1, byte2 };
        raw_manufacturer_id[0] = manufacturer_id_string_packed_bytes[1];
        raw_manufacturer_id[1] = manufacturer_id_string_packed_bytes[0];
    }

    Array<u8, 128> virtual_monitor_edid = {
        0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, /* header */
        raw_manufacturer_id[1], raw_manufacturer_id[0], /* manufacturer */
        0x00, 0x00,                                     /* product code */
        0x00, 0x00, 0x00, 0x00,                         /* serial number goes here */
        0x01,                                           /* week of manufacture */
        0x00,                                           /* year of manufacture */
        0x01, 0x03,                                     /* EDID version */
        0x80,                                           /* capabilities - digital */
        0x00,                                           /* horiz. res in cm, zero for projectors */
        0x00,                                           /* vert. res in cm */
        0x78,                                           /* display gamma (120 == 2.2). */
        0xEE,                                           /* features (standby, suspend, off, RGB, std */
                                                        /* colour space, preferred timing mode) */
        0xEE, 0x91, 0xA3, 0x54, 0x4C, 0x99, 0x26, 0x0F, 0x50, 0x54,
        /* chromaticity for standard colour space. */
        0x00, 0x00, 0x00, /* no default timings */
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, /* no standard timings */
        0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x06, 0x00, 0x02, 0x02,
        0x02, 0x02,
        /* descriptor block 1 goes below */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* descriptor block 2, monitor ranges */
        0x00, 0x00, 0x00, 0xFD, 0x00,
        0x00, 0xC8, 0x00, 0xC8, 0x64, 0x00, 0x0A, 0x20, 0x20, 0x20,
        0x20, 0x20,
        /* 0-200Hz vertical, 0-200KHz horizontal, 1000MHz pixel clock */
        0x20,
        /* descriptor block 3, monitor name */
        0x00, 0x00, 0x00, 0xFC, 0x00,
        'G', 'e', 'n', 'e', 'r', 'i', 'c', 'S', 'c', 'r', 'e', 'e', 'n',
        /* descriptor block 4: dummy data */
        0x00, 0x00, 0x00, 0x10, 0x00,
        0x0A, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20,
        0x00, /* number of extensions */
        0x00  /* checksum goes here */
    };
    // Note: Fix checksum to avoid warnings about checksum mismatch.
    size_t checksum = 0;
    // Note: Read all 127 bytes to add them to the checksum. Byte 128 is zeroed so
    // we could technically add it to the sum result, but it could lead to an error if it contained
    // a non-zero value, so we are not using it.
    for (size_t index = 0; index < sizeof(virtual_monitor_edid) - 1; index++)
        checksum += virtual_monitor_edid[index];
    virtual_monitor_edid[127] = 0x100 - checksum;
    set_edid_bytes(virtual_monitor_edid);
    return {};
}

void DisplayConnector::set_edid_bytes(Array<u8, 128> const& edid_bytes, bool might_be_invalid)
{
    memcpy((u8*)m_edid_bytes, edid_bytes.data(), sizeof(m_edid_bytes));
    if (auto parsed_edid = EDID::Parser::from_bytes({ m_edid_bytes, sizeof(m_edid_bytes) }); !parsed_edid.is_error()) {
        m_edid_parser = parsed_edid.release_value();
        m_edid_valid = true;
    } else {
        if (!might_be_invalid) {
            dmesgln("DisplayConnector: Print offending EDID");
            for (size_t x = 0; x < 128; x = x + 16) {
                dmesgln("{:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x}",
                    m_edid_bytes[x], m_edid_bytes[x + 1], m_edid_bytes[x + 2], m_edid_bytes[x + 3],
                    m_edid_bytes[x + 4], m_edid_bytes[x + 5], m_edid_bytes[x + 6], m_edid_bytes[x + 7],
                    m_edid_bytes[x + 8], m_edid_bytes[x + 9], m_edid_bytes[x + 10], m_edid_bytes[x + 11],
                    m_edid_bytes[x + 12], m_edid_bytes[x + 13], m_edid_bytes[x + 14], m_edid_bytes[x + 15]);
            }
            dmesgln("DisplayConnector: Parsing EDID failed: {}", parsed_edid.error());
        }
    }
}

ErrorOr<void> DisplayConnector::flush_rectangle(size_t, FBRect const&)
{
    return Error::from_errno(ENOTSUP);
}

DisplayConnector::ModeSetting DisplayConnector::current_mode_setting() const
{
    SpinlockLocker locker(m_modeset_lock);
    return m_current_mode_setting;
}

ErrorOr<ByteBuffer> DisplayConnector::get_edid() const
{
    if (!m_edid_valid)
        return Error::from_errno(ENODEV);
    return ByteBuffer::copy(m_edid_bytes, sizeof(m_edid_bytes));
}

}
