/*
 * Copyright (c) 2020-2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <Kernel/CommandLine.h>
#include <Kernel/FileSystem/SysFSComponent.h>
#include <Kernel/Firmware/ACPI/Definitions.h>
#include <Kernel/Firmware/ACPI/Initialize.h>
#include <Kernel/Firmware/SysFSFirmware.h>
#include <Kernel/Interrupts/IRQHandler.h>
#include <Kernel/Memory/MappedROM.h>
#include <Kernel/Memory/Region.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/PhysicalAddress.h>
#include <Kernel/VirtualAddress.h>

namespace Kernel {

class ATOMBIOSParser {
public:
    struct ExecutionContext {
        u8* current_pointed_opcode { nullptr };
        u8 opcode_argument { 0 };
    };

public:
    static NonnullOwnPtr<ATOMBIOSParser> must_initialize(Memory::MappedROM);

    void asic_init();
    void set_display_timings();

    ~ATOMBIOSParser() = default;

private:
    explicit ATOMBIOSParser(Memory::MappedROM);

    void execute_table(size_t index);

    void op_move(ExecutionContext const&);
    void op_bitwise_and(ExecutionContext const&);
    void op_bitwise_or(ExecutionContext const&);
    void op_bitwise_shift_left(ExecutionContext const&);
    void op_bitwise_shift_right(ExecutionContext const&);
    void op_shift_left(ExecutionContext const&);
    void op_shift_right(ExecutionContext const&);
    void op_multiply(ExecutionContext const&);
    void op_divide(ExecutionContext const&);
    void op_add(ExecutionContext const&);
    void op_subtract(ExecutionContext const&);
    void op_setport(ExecutionContext const&);
    void op_setregblock(ExecutionContext const&);
    void op_setfbbase(ExecutionContext const&);
    void op_compare(ExecutionContext const&);
    void op_switch(ExecutionContext const&);
    void op_jump(ExecutionContext const&);
    void op_test(ExecutionContext const&);
    void op_delay(ExecutionContext const&);
    void op_callable(ExecutionContext const&);
    void op_repeat(ExecutionContext const&);
    void op_clear(ExecutionContext const&);
    void op_nop(ExecutionContext const&);
    void op_eot(ExecutionContext const&);
    void op_mask(ExecutionContext const&);
    void op_postcard(ExecutionContext const&);
    void op_beep(ExecutionContext const&);
    void op_savereg(ExecutionContext const&);
    void op_restorereg(ExecutionContext const&);
    void op_setdatablock(ExecutionContext const&);
    void op_xor(ExecutionContext const&);

    VirtualAddress atom_rom_table_base() const;
    VirtualAddress locate_model_name_string() const;
    StringView trimmed_model_name_string() const;

    Memory::MappedROM m_mapped_atombios_rom;
};

}
