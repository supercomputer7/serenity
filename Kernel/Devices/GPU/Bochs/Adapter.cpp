/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Atomic.h>
#include <AK/Checked.h>
#include <AK/Try.h>
#if ARCH(X86_64)
#    include <Kernel/Arch/x86_64/Hypervisor/BochsDisplayConnector.h>
#endif
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/IDs.h>
#include <Kernel/Devices/GPU/Bochs/Adapter.h>
#include <Kernel/Devices/GPU/Bochs/Definitions.h>
#include <Kernel/Devices/GPU/Bochs/QEMUDisplayConnector.h>
#include <Kernel/Devices/GPU/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Devices/GPU/Management.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Sections.h>

namespace Kernel {

UNMAP_AFTER_INIT ErrorOr<bool> BochsGPUAdapter::probe(PCI::DeviceIdentifier const& pci_device_identifier)
{
    PCI::HardwareID id = pci_device_identifier.hardware_id();
    if (id.vendor_id == PCI::VendorID::QEMUOld && id.device_id == 0x1111)
        return true;
    if (id.vendor_id == PCI::VendorID::VirtualBox && id.device_id == 0xbeef)
        return true;
    return false;
}

UNMAP_AFTER_INIT ErrorOr<NonnullLockRefPtr<GPUDevice>> BochsGPUAdapter::create(PCI::DeviceIdentifier const& pci_device_identifier)
{
    auto adapter = TRY(adopt_nonnull_lock_ref_or_enomem(new (nothrow) BochsGPUAdapter(pci_device_identifier)));
    TRY(adapter->initialize_adapter(pci_device_identifier));
    return adapter;
}

UNMAP_AFTER_INIT BochsGPUAdapter::BochsGPUAdapter(PCI::DeviceIdentifier const& device_identifier)
    : PCI::Device(const_cast<PCI::DeviceIdentifier&>(device_identifier))
{
}

UNMAP_AFTER_INIT ErrorOr<void> BochsGPUAdapter::initialize_adapter(PCI::DeviceIdentifier const& pci_device_identifier)
{
    // Note: If we use VirtualBox graphics adapter (which is based on Bochs one), we need to use IO ports
    // Note: Bochs (the real bochs graphics adapter in the Bochs emulator) uses revision ID of 0x0
    // and doesn't support memory-mapped IO registers.

    // Note: In non x86-builds, we should never encounter VirtualBox hardware nor Pure Bochs VBE graphics,
    // so just assume we can use the QEMU BochsVBE-compatible graphics adapter only.
    auto bar0_space_size = PCI::get_BAR_space_size(pci_device_identifier, PCI::HeaderType0BaseRegister::BAR0);
#if ARCH(X86_64)
    bool virtual_box_hardware = (pci_device_identifier.hardware_id().vendor_id == 0x80ee && pci_device_identifier.hardware_id().device_id == 0xbeef);
    if (pci_device_identifier.revision_id().value() == 0x0 || virtual_box_hardware) {
        m_display_connector = BochsDisplayConnector::must_create(PhysicalAddress(PCI::get_BAR0(pci_device_identifier) & 0xfffffff0), bar0_space_size, virtual_box_hardware);
    } else {
        auto registers_mapping = TRY(Memory::map_typed_writable<BochsDisplayMMIORegisters volatile>(PhysicalAddress(PCI::get_BAR2(pci_device_identifier) & 0xfffffff0)));
        VERIFY(registers_mapping.region);
        m_display_connector = QEMUDisplayConnector::must_create(PhysicalAddress(PCI::get_BAR0(pci_device_identifier) & 0xfffffff0), bar0_space_size, move(registers_mapping));
    }
#else
    auto registers_mapping = TRY(Memory::map_typed_writable<BochsDisplayMMIORegisters volatile>(PhysicalAddress(PCI::get_BAR2(pci_device_identifier) & 0xfffffff0)));
    VERIFY(registers_mapping.region);
    m_display_connector = QEMUDisplayConnector::must_create(PhysicalAddress(PCI::get_BAR0(pci_device_identifier) & 0xfffffff0), bar0_space_size, move(registers_mapping));
#endif

    // Note: According to Gerd Hoffmann - "The linux driver simply does
    // the unblank unconditionally. With bochs-display this is not needed but
    // it also has no bad side effect".
    // FIXME: If the error is ENOTIMPL, ignore it for now until we implement
    // unblank support for VBoxDisplayConnector class too.
    unblank();
    TRY(m_display_connector->set_safe_mode_setting());

    return {};
}

static void set_register_with_io(u16 index, u16 data)
{
    IO::out16(VBE_DISPI_IOPORT_INDEX, index);
    IO::out16(VBE_DISPI_IOPORT_DATA, data);
}

static u16 get_register_with_io(u16 index)
{
    IO::out16(VBE_DISPI_IOPORT_INDEX, index);
    return IO::in16(VBE_DISPI_IOPORT_DATA);
}

bool BochsGPUAdapter::probe_plain_vga_isa(Badge<GPUManagement>)
{
    VERIFY(PCI::Access::is_hardware_disabled());
    return get_register_with_io(0) == VBE_DISPI_ID5;
}

ErrorOr<NonnullLockRefPtr<GPUDevice>> BochsGPUAdapter::try_create_for_plain_vga_isa(Badge<GPUManagement>)
{
    VERIFY(PCI::Access::is_hardware_disabled());
    auto video_ram_64k_chunks_count = get_register_with_io(to_underlying(BochsDISPIRegisters::VIDEO_RAM_64K_CHUNKS_COUNT));
    if (video_ram_64k_chunks_count == 0 || video_ram_64k_chunks_count == 0xffff) {
        dmesgln("GPU: Bochs ISA VGA compatible adapter does not indicate amount of VRAM, default to 8 MiB");
        video_ram_64k_chunks_count = (8 * MiB) / (64 * KiB);
    } else {
        dmesgln("GPU: Bochs ISA VGA compatible adapter indicates {} bytes of VRAM", video_ram_64k_chunks_count * (64 * KiB));
    }

    // Note: The default physical address for isa-vga framebuffer in QEMU is 0xE0000000.
    // Since this is probably hardcoded at other OSes in their guest drivers,
    // we can assume this is going to stay the same framebuffer physical address for
    // this device and will not be changed in the future.
    auto device_or_error = DeviceManagement::try_create_device<BochsDisplayConnector>(PhysicalAddress(0xE0000000), video_ram_64k_chunks_count * (64 * KiB));
    VERIFY(!device_or_error.is_error());
    auto connector = device_or_error.release_value();
    MUST(connector->create_attached_framebuffer_console());
    MUST(connector->initialize_edid_for_generic_monitor({}));
    return connector;
}

NonnullLockRefPtr<BochsDisplayConnector> BochsDisplayConnector::must_create(PhysicalAddress framebuffer_address, size_t framebuffer_resource_size, bool virtual_box_hardware)
{
    auto device_or_error = DeviceManagement::try_create_device<BochsDisplayConnector>(framebuffer_address, framebuffer_resource_size);
    VERIFY(!device_or_error.is_error());
    auto connector = device_or_error.release_value();
    MUST(connector->create_attached_framebuffer_console());
    if (virtual_box_hardware)
        MUST(connector->initialize_edid_for_generic_monitor(Array<u8, 3> { 'V', 'B', 'X' }));
    else
        MUST(connector->initialize_edid_for_generic_monitor({}));
    return connector;
}

BochsDisplayConnector::BochsDisplayConnector(PhysicalAddress framebuffer_address, size_t framebuffer_resource_size)
    : DisplayConnector(framebuffer_address, framebuffer_resource_size, false)
{
}

ErrorOr<void> BochsDisplayConnector::create_attached_framebuffer_console()
{
    // We assume safe resolution is 1024x768x32
    m_framebuffer_console = GPU::ContiguousFramebufferConsole::initialize(m_framebuffer_address.value(), 1024, 768, 1024 * sizeof(u32));
    GPUManagement::the().set_console(*m_framebuffer_console);
    return {};
}

BochsDisplayConnector::IndexID BochsDisplayConnector::index_id() const
{
    return get_register_with_io(0);
}

void BochsDisplayConnector::enable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->enable();
}

void BochsDisplayConnector::disable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->disable();
}

ErrorOr<void> BochsDisplayConnector::flush_first_surface()
{
    return Error::from_errno(ENOTSUP);
}

ErrorOr<void> BochsDisplayConnector::set_safe_mode_setting()
{
    DisplayConnector::ModeSetting safe_mode_set {
        .horizontal_stride = 1024 * sizeof(u32),
        .pixel_clock_in_khz = 0, // Note: There's no pixel clock in paravirtualized hardware
        .horizontal_active = 1024,
        .horizontal_front_porch_pixels = 0, // Note: There's no horizontal_front_porch_pixels in paravirtualized hardware
        .horizontal_sync_time_pixels = 0,   // Note: There's no horizontal_sync_time_pixels in paravirtualized hardware
        .horizontal_blank_pixels = 0,       // Note: There's no horizontal_blank_pixels in paravirtualized hardware
        .vertical_active = 768,
        .vertical_front_porch_lines = 0, // Note: There's no vertical_front_porch_lines in paravirtualized hardware
        .vertical_sync_time_lines = 0,   // Note: There's no vertical_sync_time_lines in paravirtualized hardware
        .vertical_blank_lines = 0,       // Note: There's no vertical_blank_lines in paravirtualized hardware
        .horizontal_offset = 0,
        .vertical_offset = 0,
    };
    return set_mode_setting(safe_mode_set);
}

void BochsGPUAdapter::change_framebuffer_mode_setting_with_mmio(ModeSetting const& mode_setting)
{
    m_registers->write16(to_underlying(BochsDISPIRegisters::ENABLE), 0);
    m_registers->write16(to_underlying(BochsDISPIRegisters::XRES), (u16)mode_setting.horizontal_active);
    m_registers->write16(to_underlying(BochsDISPIRegisters::YRES), (u16)mode_setting.vertical_active);
    m_registers->write16(to_underlying(BochsDISPIRegisters::VIRT_WIDTH), (u16)mode_setting.horizontal_active);
    m_registers->write16(to_underlying(BochsDISPIRegisters::VIRT_HEIGHT), (u16)mode_setting.vertical_active * 2);
    m_registers->write16(to_underlying(BochsDISPIRegisters::X_OFFSET), mode_setting.horizontal_offset);
    m_registers->write16(to_underlying(BochsDISPIRegisters::Y_OFFSET), mode_setting.vertical_offset);
    m_registers->write16(to_underlying(BochsDISPIRegisters::ENABLE), to_underlying(BochsFramebufferSettings::Enabled) | to_underlying(BochsFramebufferSettings::LinearFramebuffer));
    m_registers->write16(to_underlying(BochsDISPIRegisters::BANK), 0);
}

static void change_framebuffer_mode_setting_with_io(ModeSetting const& mode_setting)
{
    set_register_with_io(to_underlying(BochsDISPIRegisters::ENABLE), 0);
    set_register_with_io(to_underlying(BochsDISPIRegisters::XRES), (u16)mode_setting.horizontal_active);
    set_register_with_io(to_underlying(BochsDISPIRegisters::YRES), (u16)mode_setting.vertical_active);
    set_register_with_io(to_underlying(BochsDISPIRegisters::VIRT_WIDTH), (u16)mode_setting.horizontal_active);
    set_register_with_io(to_underlying(BochsDISPIRegisters::VIRT_HEIGHT), (u16)mode_setting.vertical_active * 2);
    set_register_with_io(to_underlying(BochsDISPIRegisters::X_OFFSET), mode_setting.horizontal_offset);
    set_register_with_io(to_underlying(BochsDISPIRegisters::Y_OFFSET), mode_setting.vertical_offset);
    set_register_with_io(to_underlying(BochsDISPIRegisters::ENABLE), to_underlying(BochsFramebufferSettings::Enabled) | to_underlying(BochsFramebufferSettings::LinearFramebuffer));
    set_register_with_io(to_underlying(BochsDISPIRegisters::BANK), 0);
}

ErrorOr<void> BochsGPUAdapter::validate_framebuffer_mode_setting_with_mmio(ModeSetting const& mode_setting)
{
    if ((u16)mode_setting.horizontal_active != m_dispi_mmio_registers->read16(to_underlying(BochsDISPIRegisters::XRES)) || (u16)mode_setting.vertical_active != m_dispi_mmio_registers->read16(to_underlying(BochsDISPIRegisters::YRES)))
        return Error::from_errno(ENOTIMPL);
    return {};
}

static ErrorOr<void> validate_framebuffer_mode_setting(ModeSetting const& mode_setting)
{
    if ((u16)mode_setting.horizontal_active != get_register_with_io(to_underlying(BochsDISPIRegisters::XRES)) || (u16)mode_setting.vertical_active != m_registers->read16(to_underlying(BochsDISPIRegisters::YRES)))
        return Error::from_errno(ENOTIMPL);
    return {};
}

ErrorOr<void> QEMUDisplayConnector::fetch_and_initialize_edid()
{
    Array<u8, 128> bochs_edid;
    static_assert(sizeof(BochsDisplayMMIORegisters::edid_data) >= sizeof(EDID::Definitions::EDID));
    memcpy(bochs_edid.data(), (u8 const*)(m_registers.base_address().offset(__builtin_offsetof(BochsDisplayMMIORegisters, edid_data)).as_ptr()), sizeof(bochs_edid));
    set_edid_bytes(bochs_edid);
    return {};
}

ErrorOr<void> QEMUDisplayConnector::create_attached_framebuffer_console()
{
    // We assume safe resolution is 1024x768x32
    m_framebuffer_console = GPU::ContiguousFramebufferConsole::initialize(m_framebuffer_address.value(), 1024, 768, 1024 * sizeof(u32));
    GPUManagement::the().set_console(*m_framebuffer_console);
    return {};
}

ErrorOr<void> QEMUDisplayConnector::set_safe_mode_setting()
{
    DisplayConnector::ModeSetting safe_mode_set {
        .horizontal_stride = 1024 * sizeof(u32),
        .pixel_clock_in_khz = 0, // Note: There's no pixel clock in paravirtualized hardware
        .horizontal_active = 1024,
        .horizontal_front_porch_pixels = 0, // Note: There's no horizontal_front_porch_pixels in paravirtualized hardware
        .horizontal_sync_time_pixels = 0,   // Note: There's no horizontal_sync_time_pixels in paravirtualized hardware
        .horizontal_blank_pixels = 0,       // Note: There's no horizontal_blank_pixels in paravirtualized hardware
        .vertical_active = 768,
        .vertical_front_porch_lines = 0, // Note: There's no vertical_front_porch_lines in paravirtualized hardware
        .vertical_sync_time_lines = 0,   // Note: There's no vertical_sync_time_lines in paravirtualized hardware
        .vertical_blank_lines = 0,       // Note: There's no vertical_blank_lines in paravirtualized hardware
        .horizontal_offset = 0,
        .vertical_offset = 0,
    };
    return set_mode_setting(safe_mode_set);
}

ErrorOr<void> QEMUDisplayConnector::unblank()
{
    if (!m_vga_mmio_registers)
        return ENOTSUP;
    SpinlockLocker locker(m_modeset_lock);
    full_memory_barrier();
    m_vga_mmio_registers->write(0, 0x20);
    full_memory_barrier();
    return {};
}

ErrorOr<void> QEMUDisplayConnector::set_mode_setting(ModeSetting const& mode_setting)
{
    SpinlockLocker locker(m_modeset_lock);
    VERIFY(m_framebuffer_console);
    size_t width = mode_setting.horizontal_active;
    size_t height = mode_setting.vertical_active;

    if (Checked<size_t>::multiplication_would_overflow(width, height, sizeof(u32)))
        return EOVERFLOW;

#if ARCH(X86_64)
    if (m_dispi_mmio_registers) {
        change_framebuffer_mode_setting_with_mmio(mode_setting);
    } else {
        change_framebuffer_mode_setting_with_io(mode_setting)
    }
#else
    if (!m_dispi_mmio_registers)
        return ENOTSUP;
    change_framebuffer_mode_setting_with_mmio(mode_setting);
#elif

    m_framebuffer_console->set_resolution(width, height, width * sizeof(u32));

    DisplayConnector::ModeSetting mode_set {
        .horizontal_stride = m_registers->bochs_regs.xres * sizeof(u32),
        .pixel_clock_in_khz = 0, // Note: There's no pixel clock in paravirtualized hardware
        .horizontal_active = m_registers->bochs_regs.xres,
        .horizontal_front_porch_pixels = 0, // Note: There's no horizontal_front_porch_pixels in paravirtualized hardware
        .horizontal_sync_time_pixels = 0,   // Note: There's no horizontal_sync_time_pixels in paravirtualized hardware
        .horizontal_blank_pixels = 0,       // Note: There's no horizontal_blank_pixels in paravirtualized hardware
        .vertical_active = m_registers->bochs_regs.yres,
        .vertical_front_porch_lines = 0, // Note: There's no vertical_front_porch_lines in paravirtualized hardware
        .vertical_sync_time_lines = 0,   // Note: There's no vertical_sync_time_lines in paravirtualized hardware
        .vertical_blank_lines = 0,       // Note: There's no vertical_blank_lines in paravirtualized hardware
        .horizontal_offset = 0,
        .vertical_offset = 0,
    };

    m_current_mode_setting = mode_set;
    return {};
}

}
