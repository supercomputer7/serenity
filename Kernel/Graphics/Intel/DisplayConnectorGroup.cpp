/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/x86/IO.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Debug.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/GraphicsManagement.h>
#include <Kernel/Graphics/Intel/DisplayConnectorGroup.h>
#include <Kernel/Memory/Region.h>
#include <Kernel/Memory/TypedMapping.h>

namespace Kernel {

namespace IntelGraphics {

#define DDC2_I2C_ADDRESS 0x50

struct PLLSettings {
    bool is_valid() const { return (n != 0 && m1 != 0 && m2 != 0 && p1 != 0 && p2 != 0); }
    u64 compute_dot_clock(u64 refclock) const
    {
        return (refclock * (5 * m1 + m2) / n) / (p1 * p2);
    }

    u64 compute_vco(u64 refclock) const
    {
        return refclock * (5 * m1 + m2) / n;
    }

    u64 compute_m() const
    {
        return 5 * m1 + m2;
    }

    u64 compute_p() const
    {
        return p1 * p2;
    }
    u64 n { 0 };
    u64 m1 { 0 };
    u64 m2 { 0 };
    u64 p1 { 0 };
    u64 p2 { 0 };
};

static constexpr PLLMaxSettings G35Limits {
    { 20'000'000, 400'000'000 },      // values in Hz, dot_clock
    { 1'400'000'000, 2'800'000'000 }, // values in Hz, VCO
    { 3, 8 },                         // n
    { 70, 120 },                      // m
    { 10, 20 },                       // m1
    { 5, 9 },                         // m2
    { 5, 80 },                        // p
    { 1, 8 },                         // p1
    { 5, 10 }                         // p2
};
}

static bool check_pll_settings(IntelGraphics::PLLSettings const& settings, size_t reference_clock, IntelGraphics::PLLMaxSettings const& limits)
{
    if (settings.n < limits.n.min || settings.n > limits.n.max) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "N is invalid {}", settings.n);
        return false;
    }
    if (settings.m1 < limits.m1.min || settings.m1 > limits.m1.max) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "m1 is invalid {}", settings.m1);
        return false;
    }
    if (settings.m2 < limits.m2.min || settings.m2 > limits.m2.max) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "m2 is invalid {}", settings.m2);
        return false;
    }
    if (settings.p1 < limits.p1.min || settings.p1 > limits.p1.max) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "p1 is invalid {}", settings.p1);
        return false;
    }

    if (settings.m1 <= settings.m2) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "m2 is invalid {} as it is bigger than m1 {}", settings.m2, settings.m1);
        return false;
    }

    auto m = settings.compute_m();
    auto p = settings.compute_p();

    if (m < limits.m.min || m > limits.m.max) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "m invalid {}", m);
        return false;
    }
    if (p < limits.p.min || p > limits.p.max) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "p invalid {}", p);
        return false;
    }

    auto dot = settings.compute_dot_clock(reference_clock);
    auto vco = settings.compute_vco(reference_clock);

    if (dot < limits.dot_clock.min || dot > limits.dot_clock.max) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "Dot clock invalid {}", dot);
        return false;
    }
    if (vco < limits.vco.min || vco > limits.vco.max) {
        dbgln_if(INTEL_GRAPHICS_DEBUG, "VCO clock invalid {}", vco);
        return false;
    }
    return true;
}

static size_t find_absolute_difference(u64 target_frequency, u64 checked_frequency)
{
    if (target_frequency >= checked_frequency)
        return target_frequency - checked_frequency;
    return checked_frequency - target_frequency;
}

Optional<IntelGraphics::PLLSettings> IntelDisplayConnectorGroup::create_pll_settings(u64 target_frequency, u64 reference_clock, IntelGraphics::PLLMaxSettings const& limits)
{
    IntelGraphics::PLLSettings settings;
    IntelGraphics::PLLSettings best_settings;
    // FIXME: Is this correct for all Intel Native graphics cards?
    settings.p2 = 10;
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Check PLL settings for ref clock of {} Hz, for target of {} Hz", reference_clock, target_frequency);
    u64 best_difference = 0xffffffff;
    for (settings.n = limits.n.min; settings.n <= limits.n.max; ++settings.n) {
        for (settings.m1 = limits.m1.max; settings.m1 >= limits.m1.min; --settings.m1) {
            for (settings.m2 = limits.m2.max; settings.m2 >= limits.m2.min; --settings.m2) {
                for (settings.p1 = limits.p1.max; settings.p1 >= limits.p1.min; --settings.p1) {
                    dbgln_if(INTEL_GRAPHICS_DEBUG, "Check PLL settings for {} {} {} {} {}", settings.n, settings.m1, settings.m2, settings.p1, settings.p2);
                    if (!check_pll_settings(settings, reference_clock, limits))
                        continue;
                    auto current_dot_clock = settings.compute_dot_clock(reference_clock);
                    if (current_dot_clock == target_frequency)
                        return settings;
                    auto difference = find_absolute_difference(target_frequency, current_dot_clock);
                    if (difference < best_difference && (current_dot_clock > target_frequency)) {
                        best_settings = settings;
                        best_difference = difference;
                    }
                }
            }
        }
    }
    if (best_settings.is_valid())
        return best_settings;
    return {};
}

NonnullRefPtr<IntelDisplayConnectorGroup> IntelDisplayConnectorGroup::must_create(Badge<IntelNativeGraphicsAdapter>, PCIVGAGenericAdapter const& parent_device, Generation generation, MMIORegion const& first_region, MMIORegion const& second_region)
{
    auto registers_region = MUST(MM.allocate_kernel_region(first_region.pci_bar_paddr, first_region.pci_bar_space_length, "Intel Native Graphics Registers", Memory::Region::Access::ReadWrite));
    // Note: 0x5100 is the offset of the start of the GMBus registers
    auto gmbus_connector = MUST(GMBusConnector::create_with_physical_address(first_region.pci_bar_paddr.offset(0x5100)));
    auto connector_group = adopt_ref_if_nonnull(new (nothrow) IntelDisplayConnectorGroup(parent_device, generation, move(gmbus_connector), move(registers_region), first_region, second_region)).release_nonnull();
    MUST(connector_group->initialize_connectors());
    return connector_group;
}

IntelDisplayConnectorGroup::IntelDisplayConnectorGroup(PCIVGAGenericAdapter const& parent_device, Generation generation, NonnullOwnPtr<GMBusConnector> gmbus_connector, NonnullOwnPtr<Memory::Region> registers_region, MMIORegion const& first_region, MMIORegion const& second_region)
    : m_mmio_first_region(first_region)
    , m_mmio_second_region(second_region)
    , m_assigned_mmio_registers_region(m_mmio_first_region)
    , m_generation(generation)
    , m_registers_region(move(registers_region))
    , m_gmbus_connector(move(gmbus_connector))
    , m_parent_device(parent_device)
{
}

ErrorOr<void> IntelDisplayConnectorGroup::initialize_gen4_connectors()
{
    // Note: Just assume we will need one Gen4 "transcoder", starting at HorizontalTotalA register (0x60000)
    m_transcoders[0] = MUST(IntelDisplayTranscoder::create_with_physical_address(m_mmio_first_region.pci_bar_paddr.offset(0x60000)));
    m_planes[0] = MUST(IntelDisplayPlane::create_with_physical_address(m_mmio_first_region.pci_bar_paddr.offset(0x70180)));
    Array<u8, 128> crt_edid_bytes {};
    {
        SpinlockLocker control_lock(m_control_lock);
        MUST(m_gmbus_connector->write(DDC2_I2C_ADDRESS, 0));
        MUST(m_gmbus_connector->read(DDC2_I2C_ADDRESS, crt_edid_bytes.data(), sizeof(crt_edid_bytes)));

        // FIXME: It seems like the returned EDID is almost correct,
        // but the first byte is set to 0xD0 instead of 0x00.
        // For now, this "hack" works well enough.
        crt_edid_bytes[0] = 0x0;
    }
    auto connector = IntelNativeDisplayConnector::must_create(*this, IntelNativeDisplayConnector::Type::Analog);
    m_connectors[0] = connector;
    connector->set_edid_bytes({}, crt_edid_bytes);
    return {};
}

ErrorOr<void> IntelDisplayConnectorGroup::initialize_connectors()
{

    // Note: Intel Graphics Generation 4 is pretty ancient beast, and we should not
    // assume we can find a VBT for it. Just initialize the (assumed) CRT connector and be done with it.
    if (m_generation == Generation::Gen4) {
        TRY(initialize_gen4_connectors());
    } else {
        VERIFY_NOT_REACHED();
    }

    for (size_t connector_index = 0; connector_index < m_connectors.size(); connector_index++) {
        if (!m_connectors[connector_index])
            continue;
        if (!m_connectors[connector_index]->m_edid_valid)
            continue;
        TRY(m_connectors[connector_index]->set_safe_mode_setting());
        TRY(m_connectors[connector_index]->create_attached_framebuffer_console({}, m_mmio_second_region.pci_bar_paddr));
    }
    return {};
}

ErrorOr<void> IntelDisplayConnectorGroup::set_safe_mode_setting(Badge<IntelNativeDisplayConnector>, IntelNativeDisplayConnector& connector)
{
    if (!connector.m_edid_parser.has_value())
        return Error::from_errno(ENOTSUP);
    if (!connector.m_edid_parser.value().detailed_timing(0).has_value())
        return Error::from_errno(ENOTSUP);
    auto details = connector.m_edid_parser.value().detailed_timing(0).release_value();

    DisplayConnector::ModeSetting modesetting {
        // Note: We assume that we always use 32 bit framebuffers.
        .horizontal_stride = details.horizontal_addressable_pixels() * sizeof(u32),
        .pixel_clock_in_khz = details.pixel_clock_khz(),
        .horizontal_active = details.horizontal_addressable_pixels(),
        .horizontal_front_porch_pixels = details.horizontal_front_porch_pixels(),
        .horizontal_sync_time_pixels = details.horizontal_sync_pulse_width_pixels(),
        .horizontal_blank_pixels = details.horizontal_blanking_pixels(),
        .vertical_active = details.vertical_addressable_lines(),
        .vertical_front_porch_lines = details.vertical_front_porch_lines(),
        .vertical_sync_time_lines = details.vertical_sync_pulse_width_lines(),
        .vertical_blank_lines = details.vertical_blanking_lines(),
    };

    return set_mode_setting(connector, modesetting);
}

ErrorOr<void> IntelDisplayConnectorGroup::set_mode_setting(Badge<IntelNativeDisplayConnector>, IntelNativeDisplayConnector& connector, DisplayConnector::ModeSetting const& mode_setting)
{
    return set_mode_setting(connector, mode_setting);
}

ErrorOr<void> IntelDisplayConnectorGroup::set_mode_setting(IntelNativeDisplayConnector& connector, DisplayConnector::ModeSetting const& mode_setting)
{
    SpinlockLocker locker(connector.m_modeset_lock);

    DisplayConnector::ModeSetting actual_mode_setting = mode_setting;
    actual_mode_setting.horizontal_stride = actual_mode_setting.horizontal_active * sizeof(u32);
    VERIFY(actual_mode_setting.horizontal_stride != 0);
    if (m_generation == Generation::Gen4) {
        TRY(set_gen4_mode_setting(connector, actual_mode_setting));
    } else {
        VERIFY_NOT_REACHED();
    }

    connector.m_current_mode_setting = actual_mode_setting;
    connector.m_framebuffer_address = m_mmio_second_region.pci_bar_paddr;
    auto rounded_size = TRY(Memory::page_round_up(connector.m_current_mode_setting.horizontal_stride * connector.m_current_mode_setting.horizontal_active));
    connector.m_framebuffer_region = MUST(MM.allocate_kernel_region(m_mmio_second_region.pci_bar_paddr, rounded_size, "Intel Native Graphics Framebuffer", Memory::Region::Access::ReadWrite));
    [[maybe_unused]] auto result = connector.m_framebuffer_region->set_write_combine(true);
    if (!connector.m_framebuffer_console.is_null())
        static_cast<Graphics::GenericFramebufferConsoleImpl*>(connector.m_framebuffer_console.ptr())->set_resolution(actual_mode_setting.horizontal_active, actual_mode_setting.vertical_active, actual_mode_setting.horizontal_stride);
    return {};
}

ErrorOr<void> IntelDisplayConnectorGroup::set_gen4_mode_setting(IntelNativeDisplayConnector& connector, DisplayConnector::ModeSetting const& mode_setting)
{
    VERIFY(connector.m_modeset_lock.is_locked());
    SpinlockLocker control_lock(m_control_lock);
    SpinlockLocker modeset_lock(m_modeset_lock);
    if (!set_crt_resolution(mode_setting))
        return Error::from_errno(ENOTSUP);
    return {};
}

void IntelDisplayConnectorGroup::enable_vga_plane()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
}

[[maybe_unused]] static StringView convert_global_generation_register_index_to_string(IntelGraphics::GlobalGenerationRegister index)
{
    switch (index) {
    case IntelGraphics::GlobalGenerationRegister::PipeAConf:
        return "PipeAConf"sv;
    case IntelGraphics::GlobalGenerationRegister::PipeBConf:
        return "PipeBConf"sv;
    case IntelGraphics::GlobalGenerationRegister::DPLLDivisorA0:
        return "DPLLDivisorA0"sv;
    case IntelGraphics::GlobalGenerationRegister::DPLLDivisorA1:
        return "DPLLDivisorA1"sv;
    case IntelGraphics::GlobalGenerationRegister::DPLLControlA:
        return "DPLLControlA"sv;
    case IntelGraphics::GlobalGenerationRegister::DPLLControlB:
        return "DPLLControlB"sv;
    case IntelGraphics::GlobalGenerationRegister::DPLLMultiplierA:
        return "DPLLMultiplierA"sv;
    case IntelGraphics::GlobalGenerationRegister::AnalogDisplayPort:
        return "AnalogDisplayPort"sv;
    case IntelGraphics::GlobalGenerationRegister::VGADisplayPlaneControl:
        return "VGADisplayPlaneControl"sv;
    default:
        VERIFY_NOT_REACHED();
    }
}

void IntelDisplayConnectorGroup::write_to_general_register(RegisterOffset offset, u32 value) const
{
    VERIFY(m_control_lock.is_locked());
    SpinlockLocker lock(m_registers_lock);
    auto* reg = (u32 volatile*)m_registers_region->vaddr().offset(offset.value()).as_ptr();
    *reg = value;
}
u32 IntelDisplayConnectorGroup::read_from_general_register(RegisterOffset offset) const
{
    VERIFY(m_control_lock.is_locked());
    SpinlockLocker lock(m_registers_lock);
    auto* reg = (u32 volatile*)m_registers_region->vaddr().offset(offset.value()).as_ptr();
    u32 value = *reg;
    return value;
}

void IntelDisplayConnectorGroup::write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister index, u32 value) const
{
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Intel Graphics Display Connector:: Write to {} value of {:x}", convert_global_generation_register_index_to_string(index), value);
    write_to_general_register(to_underlying(index), value);
}
u32 IntelDisplayConnectorGroup::read_from_global_generation_register(IntelGraphics::GlobalGenerationRegister index) const
{
    u32 value = read_from_general_register(to_underlying(index));
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Intel Graphics Display Connector: Read from {} value of {:x}", convert_global_generation_register_index_to_string(index), value);
    return value;
}

bool IntelDisplayConnectorGroup::pipe_a_enabled() const
{
    VERIFY(m_control_lock.is_locked());
    return read_from_global_generation_register(IntelGraphics::GlobalGenerationRegister::PipeAConf) & (1 << 30);
}

bool IntelDisplayConnectorGroup::pipe_b_enabled() const
{
    VERIFY(m_control_lock.is_locked());
    return read_from_global_generation_register(IntelGraphics::GlobalGenerationRegister::PipeBConf) & (1 << 30);
}

static size_t compute_dac_multiplier(size_t pixel_clock_in_khz)
{
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Intel native graphics: Pixel clock is {} KHz", pixel_clock_in_khz);
    VERIFY(pixel_clock_in_khz >= 25000);
    if (pixel_clock_in_khz >= 100000) {
        return 1;
    } else if (pixel_clock_in_khz >= 50000) {
        return 2;
    } else {
        return 4;
    }
}

bool IntelDisplayConnectorGroup::set_crt_resolution(DisplayConnector::ModeSetting const& mode_setting)
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());

    // Note: Just in case we still allow access to VGA IO ports, disable it now.
    GraphicsManagement::the().disable_vga_emulation_access_permanently();

    auto dac_multiplier = compute_dac_multiplier(mode_setting.pixel_clock_in_khz);
    auto pll_settings = create_pll_settings((1000 * mode_setting.pixel_clock_in_khz * dac_multiplier), 96'000'000, IntelGraphics::G35Limits);
    if (!pll_settings.has_value())
        return false;
    auto settings = pll_settings.value();

    disable_dac_output();
    MUST(m_planes[0]->disable({}));
    disable_pipe_a();
    disable_pipe_b();
    disable_dpll();
    disable_vga_emulation();

    dbgln_if(INTEL_GRAPHICS_DEBUG, "PLL settings for {} {} {} {} {}", settings.n, settings.m1, settings.m2, settings.p1, settings.p2);
    enable_dpll_without_vga(settings, dac_multiplier);
    MUST(m_transcoders[0]->set_mode_setting_timings({}, mode_setting));

    VERIFY(!pipe_a_enabled());
    enable_pipe_a();
    MUST(m_planes[0]->set_plane_settings({}, m_mmio_second_region.pci_bar_paddr, IntelDisplayPlane::PipeSelect::PipeA, mode_setting.horizontal_active));
    MUST(m_planes[0]->enable({}));
    enable_dac_output();

    return true;
}

bool IntelDisplayConnectorGroup::wait_for_enabled_pipe_a(size_t milliseconds_timeout) const
{
    size_t current_time = 0;
    while (current_time < milliseconds_timeout) {
        if (pipe_a_enabled())
            return true;
        IO::delay(1000);
        current_time++;
    }
    return false;
}
bool IntelDisplayConnectorGroup::wait_for_disabled_pipe_a(size_t milliseconds_timeout) const
{
    size_t current_time = 0;
    while (current_time < milliseconds_timeout) {
        if (!pipe_a_enabled())
            return true;
        IO::delay(1000);
        current_time++;
    }
    return false;
}

bool IntelDisplayConnectorGroup::wait_for_disabled_pipe_b(size_t milliseconds_timeout) const
{
    size_t current_time = 0;
    while (current_time < milliseconds_timeout) {
        if (!pipe_b_enabled())
            return true;
        IO::delay(1000);
        current_time++;
    }
    return false;
}

void IntelDisplayConnectorGroup::disable_dpll()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::DPLLControlA, 0);
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::DPLLControlB, 0);
}

void IntelDisplayConnectorGroup::disable_pipe_a()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::PipeAConf, 0);
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Disabling Pipe A");
    wait_for_disabled_pipe_a(100);
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Disabling Pipe A - done.");
}

void IntelDisplayConnectorGroup::disable_pipe_b()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::PipeAConf, 0);
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Disabling Pipe B");
    wait_for_disabled_pipe_b(100);
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Disabling Pipe B - done.");
}

void IntelDisplayConnectorGroup::enable_pipe_a()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
    VERIFY(!(read_from_global_generation_register(IntelGraphics::GlobalGenerationRegister::PipeAConf) & (1 << 31)));
    VERIFY(!(read_from_global_generation_register(IntelGraphics::GlobalGenerationRegister::PipeAConf) & (1 << 30)));
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::PipeAConf, (1 << 31) | (1 << 24));
    dbgln_if(INTEL_GRAPHICS_DEBUG, "enabling Pipe A");
    // FIXME: Seems like my video card is buggy and doesn't set the enabled bit (bit 30)!!
    wait_for_enabled_pipe_a(100);
    dbgln_if(INTEL_GRAPHICS_DEBUG, "enabling Pipe A - done.");
}

void IntelDisplayConnectorGroup::set_dpll_registers(IntelGraphics::PLLSettings const& settings)
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::DPLLDivisorA0, (settings.m2 - 2) | ((settings.m1 - 2) << 8) | ((settings.n - 2) << 16));
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::DPLLDivisorA1, (settings.m2 - 2) | ((settings.m1 - 2) << 8) | ((settings.n - 2) << 16));

    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::DPLLControlA, 0);
}

void IntelDisplayConnectorGroup::enable_dpll_without_vga(IntelGraphics::PLLSettings const& settings, size_t dac_multiplier)
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());

    set_dpll_registers(settings);

    IO::delay(200);

    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::DPLLControlA, (6 << 9) | (settings.p1) << 16 | (1 << 26) | (1 << 28) | (1 << 31));
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::DPLLMultiplierA, (dac_multiplier - 1) | ((dac_multiplier - 1) << 8));

    // The specification says we should wait (at least) about 150 microseconds
    // after enabling the DPLL to allow the clock to stabilize
    IO::delay(200);
    VERIFY(read_from_global_generation_register(IntelGraphics::GlobalGenerationRegister::DPLLControlA) & (1 << 31));
}

void IntelDisplayConnectorGroup::disable_dac_output()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::AnalogDisplayPort, 0b11 << 10);
}

void IntelDisplayConnectorGroup::enable_dac_output()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::AnalogDisplayPort, (1 << 31));
}

void IntelDisplayConnectorGroup::disable_vga_emulation()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_modeset_lock.is_locked());
    write_to_global_generation_register(IntelGraphics::GlobalGenerationRegister::VGADisplayPlaneControl, (1 << 31));
    read_from_global_generation_register(IntelGraphics::GlobalGenerationRegister::VGADisplayPlaneControl);
}

}
