/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Atomic.h>
#include <AK/Checked.h>
#include <AK/Try.h>
#include <Kernel/Arch/x86/IO.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/IDs.h>
#include <Kernel/Debug.h>
#include <Kernel/Graphics/AMD/ATOMBIOS.h>

namespace Kernel {

namespace ATOMBIOS {
struct [[gnu::packed]] CommonTableHeader {
    u16 structure_size;
    u8 table_format_revision;
    u8 table_content_revision;
};

struct [[gnu::packed]] ROMHeader {
    CommonTableHeader header;
    u8 signature[4];
    u16 bios_runtime_segment_address;
    u16 protected_mode_info_offset;
    u16 config_filename_offset;
    u16 crc_block_offset;
    u16 bios_bootup_message_offset;
    u16 int10_offset;
    u16 pci_bus_device_init_code;
    u16 io_base_address;
    u16 subsystem_vendor_id;
    u16 subsystem_id;
    u16 pci_info_offset;
    u16 master_command_table_offset;
    u16 master_data_table_offset;
    u8 extended_function_code;
    u8 reserved;
};

struct [[gnu::packed]] ROMHeaderVersion2 {
    ROMHeader rom_header;
    u32 psp_directory_table_offset;
};

struct MasterCommandTablesTable {
    CommonTableHeader header;
    u16 asic_init;
    u16 get_display_surface_size;
    u16 asic_registers_init;
    u16 vram_block_vender_detection;
    u16 digx_encoder_control;
    u16 memory_controller_init;
    u16 enable_crtc_memory_request;
    u16 memory_parameter_adjust;
    u16 dvo_encoder_control;
    u16 gpio_pin_control;
    u16 set_engine_clock;
    u16 set_memory_clock;
    u16 set_pixel_clock;
    u16 enable_display_power_gating;
    u16 reset_memory_dll;
    u16 reset_memory_device;
    u16 memory_pll_init;
    u16 adjust_display_pll;
    u16 adjust_memory_controller;
    u16 enable_asic_static_power_management;
    u16 set_uniphy_instance;
    u16 dac_load_detection;
    u16 lvtma_encoder_control;
    u16 hardware_misc_operation;
    u16 dac1_encoder_control;
    u16 dac2_encoder_control;
    u16 dvo_output_control;
    u16 cv1_output_control;
    u16 get_conditional_golden_setting;
    u16 smc_init;
    u16 patch_mc_setting;
    u16 mc_sequential_control;
    u16 gfx_harvesting;
    u16 enable_scaler;
    u16 blank_crtc;
    u16 enable_crtc;
    u16 get_pixel_clock;
    u16 enable_vga_render;
    u16 get_sclock_over_mclock_ratio;
    u16 set_crtc_timing;
    u16 set_crtc_over_scan;
    u16 get_smu_clock_Info;
    u16 select_crtc_source;
    u16 enable_graph_surfaces;
    u16 update_crtc_double_buffer_registers;
    u16 lut_auto_fill;
    u16 set_dce_clock;
    u16 get_memory_clock;
    u16 get_engine_clock;
    u16 set_crtc_using_dtd_timing;
    u16 external_encoder_control;
    u16 lvtma_output_control;
    u16 vram_block_detection_by_strap;
    u16 memory_clean_up;
    u16 process_i2c_channel_transaction;
    u16 write_one_byte_to_hw_assisted_i2c;
    u16 read_hardware_assisted_i2c_status;
    u16 speed_fan_control;
    u16 power_connector_detection;
    u16 mc_synchronization;
    u16 compute_memory_engine_pll;
    u16 gfx_Init;
    u16 vram_get_current_info_block;
    u16 dynamic_memory_settings;
    u16 memory_training;
    u16 enable_spread_spectrum_on_pixel_pll;
    u16 tmdsa_output_control;
    u16 set_voltage;
    u16 dac1_output_control;
    u16 read_efuse_value;
    u16 compute_memory_clock_parameter;
    u16 clock_source;
    u16 memory_device_init;
    u16 get_display_object_info;
    u16 dig1_encoder_control;
    u16 dig2_encoder_control;
    u16 dig1_transmitter_control;
    u16 dig2_transmitter_control;
    u16 process_aux_channel_transaction;
    u16 display_port_encoder_service;
    u16 get_voltage_info;
};

}

NonnullOwnPtr<ATOMBIOSParser> ATOMBIOSParser::must_initialize(Memory::MappedROM mapped_atombios)
{
    auto parser = adopt_own_if_nonnull(new (nothrow) ATOMBIOSParser(move(mapped_atombios))).release_nonnull();
    return parser;
}

ATOMBIOSParser::ATOMBIOSParser(Memory::MappedROM mapped_atombios)
    : m_mapped_atombios_rom(move(mapped_atombios))
{
    dbgln("ATOMBIOS: @ {}, decoding start at {}", m_mapped_atombios_rom.paddr, *(u16*)m_mapped_atombios_rom.base_address().offset(0x48).as_ptr());
    dbgln("ATOMBIOS Device Model: {}", trimmed_model_name_string());
}

StringView ATOMBIOSParser::trimmed_model_name_string() const
{
    Span<const u8> name_string_span;
    auto raw_model_name_string = locate_model_name_string();
    if (*raw_model_name_string.as_ptr() == '\r' && *raw_model_name_string.offset(1).as_ptr() == '\n')
        name_string_span = Span<const u8>(raw_model_name_string.offset(2).as_ptr(), 510);
    else
        name_string_span = Span<const u8>(raw_model_name_string.as_ptr(), 512);
    auto string_view = StringView(name_string_span);
    auto end_of_string_limit = string_view.find('\n');
    VERIFY(end_of_string_limit.has_value());
    return StringView(Span<const u8>(string_view.characters_without_null_termination(), end_of_string_limit.value())).trim_whitespace();
}

VirtualAddress ATOMBIOSParser::atom_rom_table_base() const
{
    auto rom_table_base = *(u16*)m_mapped_atombios_rom.base_address().offset(0x48).as_ptr();
    return m_mapped_atombios_rom.base_address().offset(rom_table_base);
}

VirtualAddress ATOMBIOSParser::locate_model_name_string() const
{
    auto name_string_offset = *(u16*)atom_rom_table_base().offset(__builtin_offsetof(ATOMBIOS::ROMHeader, bios_bootup_message_offset)).as_ptr();
    return m_mapped_atombios_rom.base_address().offset(name_string_offset);
    //= m_mapped_atombios_rom.base()
}

// VirtualAddress ATOMBIOSParser::locate_command_table_start(size_t table_index)
//{
//     VERIFY(table_index < (__builtin_offsetof(ATOMBIOS::MasterCommandTablesTable, get_voltage_info) / 2));
//     auto master_command_table_offset = *(u16*)atom_rom_table_base().offset(__builtin_offsetof(ATOMBIOS::ROMHeader, master_command_table_offset));
//     return m_mapped_atombios_rom.base_address().offset(master_command_table_offset);
//     //= m_mapped_atombios_rom.base()
// }

// void ATOMBIOSParser::execute_table(size_t table_index)
void ATOMBIOSParser::execute_table(size_t)
{
    // VERIFY(table_index < (__builtin_offsetof(ATOMBIOS::MasterCommandTablesTable, get_voltage_info) / 2));
    // ATOMBIOSParser::ExecutionContext execution_context {};
    // execution_context.current_pointed_opcode
    // switch ()
}
void ATOMBIOSParser::op_move(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_bitwise_and(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_bitwise_or(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_bitwise_shift_left(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_bitwise_shift_right(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_shift_left(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_shift_right(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_multiply(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_divide(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_add(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_subtract(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_setport(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_setregblock(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_setfbbase(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_compare(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_switch(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_jump(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_test(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_delay(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_callable(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_repeat(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_clear(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_nop(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_eot(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_mask(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_postcard(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_beep(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_savereg(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_restorereg(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_setdatablock(ATOMBIOSParser::ExecutionContext const&)
{
}
void ATOMBIOSParser::op_xor(ATOMBIOSParser::ExecutionContext const&)
{
}
}
