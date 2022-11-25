/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>

namespace Kernel {

struct [[gnu::packed]] SDHCHostRegisters {
    // Note: When Host Version 4 is enabled, this register is being used as 32 bit block_count,
    // otherwise as the SDMA address
    union {
        u32 sdma_address;
        u32 host_version4_block_count;
    };
    u16 block_size;
    // Note: When Host Version 4 is enabled, this register is not being used and should be set to zero.
    u16 block_count;
    u32 command_argument;
    u16 transfer_mode;
    u16 command;
    u32 response[4];
    // Note: This register is being used in PIO fashion (i.e. non DMA transaction)
    u32 pio_buffer_data;
    // Note: This register (read only) is used to determine many interesting status scenarios
    // and conditions like hotplug of SD card (insertion, removal) etc.
    u32 controller_status;
    u8 host_control;
    u8 power_control;
    u8 block_gap_control;
    u8 wakeup_control;
    u16 clock_control;
    u8 timeout_control;
    u8 software_reset;
    // Note: This register is used to determine many interesting status scenarios
    // and conditions like hotplug of SD card (insertion, removal) etc that triggered an IRQ.
    u16 normal_interrupt_status;
    u16 error_interrupt_status;

    u16 normal_interrupt_status_enable;
    u16 error_interrupt_status_enable;

    u16 normal_interrupt_trigger_enable;
    u16 error_interrupt_trigger_enable;

    u16 auto_cmd_error_status;

    u16 host_control2;
    u32 capabilities_low;
    u32 capabilities_high;
    u64 max_currents_for_voltages;
    // Note: These two registers are meant for aiding with testing of handling
    // errors by host driver.
    u16 force_auto_cmd_error_event;
    u16 force_error_event;

    u8 adma_error;
    u8 reserved[3]; /* 0x55 - 0x58 */
    u64 adma_address;
    u8 reserved2[24]; /* 0x60 - 0x77 */
    u64 adma3_integrated_descriptor_address;

    // category B registers, for UHS bus
    u16 uhs_block_size;
    u8 reserved3[2];
    u32 uhs_blocks_count;
    u8 uhs_command_packet[20];
    u16 uhs_transfer_mode;
    u16 uhs_command;
    u8 uhs_response[20];
    u8 uhs_msg_select;
    u8 reserved4[3];
    u32 uhs_msg;
    u16 uhs_device_interrupt_status;
    u8 uhs_device_select;
    u8 uhs_device_interrupt_code;
    u16 uhs_software_reset;
    u16 uhs_timer_control;

    u32 uhs_error_interrupt_status;
    u32 uhs_error_interrupt_status_enable;
    u32 uhs_error_interrupt_trigger_enable;

    u8 reserved5[46];

    u16 slot_interrupt_status;
    u16 host_controller_version;
};

static_assert(__builtin_offsetof(SDHCHostRegisters, block_size) == 0x4);
static_assert(__builtin_offsetof(SDHCHostRegisters, block_count) == 0x6);
static_assert(__builtin_offsetof(SDHCHostRegisters, command_argument) == 0x8);
static_assert(__builtin_offsetof(SDHCHostRegisters, transfer_mode) == 0xC);
static_assert(__builtin_offsetof(SDHCHostRegisters, command) == 0xE);
static_assert(__builtin_offsetof(SDHCHostRegisters, response) == 0x10);
static_assert(__builtin_offsetof(SDHCHostRegisters, normal_interrupt_status) == 0x30);
static_assert(__builtin_offsetof(SDHCHostRegisters, capabilities_low) == 0x40);
static_assert(__builtin_offsetof(SDHCHostRegisters, adma_address) == 0x58);
static_assert(__builtin_offsetof(SDHCHostRegisters, adma3_integrated_descriptor_address) == 0x78);

}
