/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>

namespace Kernel {

static constexpr size_t divergent_display_max_width = 2440;
static constexpr size_t divergent_display_max_height = 1900;

struct [[gnu::packed]] DivergentDisplayTranscoderRegisters {
    u32 bpp;
    u32 dimensions;
    u32 stride;
    u32 special_traits;
    u32 gates;
    u32 x_offset;
    u32 y_offset;
    u32 surface_offset;
    u8 padding[0x100 - 32];
};

struct [[gnu::packed]] DivergentDisplayControllerRegisters {
    u32 imr;
    u32 isr;
    u32 fuses;
    u32 ddc_data;
    u32 ddc_byte_index;
    u32 ddc_control;
    u32 ddc_status;
    u8 padding[0x100 - 28];
};

static_assert(sizeof(DivergentDisplayControllerRegisters) == 0x100);
static_assert(sizeof(DivergentDisplayTranscoderRegisters) == 0x100);

struct [[gnu::packed]] DivergentDisplayProgrammingInterface {
    DivergentDisplayControllerRegisters controller_registers;
    DivergentDisplayTranscoderRegisters display_transcoders[8];
};

}
