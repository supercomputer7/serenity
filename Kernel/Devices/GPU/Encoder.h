/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <LibEDID/EDID.h>

namespace Kernel {

struct ModeSetting {
    size_t horizontal_blanking_start() const
    {
        return horizontal_active;
    }
    size_t horizontal_sync_start() const
    {
        return horizontal_active + horizontal_front_porch_pixels;
    }
    size_t horizontal_sync_end() const
    {
        return horizontal_active + horizontal_front_porch_pixels + horizontal_sync_time_pixels;
    }
    size_t horizontal_total() const
    {
        return horizontal_active + horizontal_blank_pixels;
    }

    size_t vertical_blanking_start() const
    {
        return vertical_active;
    }
    size_t vertical_sync_start() const
    {
        return vertical_active + vertical_front_porch_lines;
    }
    size_t vertical_sync_end() const
    {
        return vertical_active + vertical_front_porch_lines + vertical_sync_time_lines;
    }
    size_t vertical_total() const
    {
        return vertical_active + vertical_blank_lines;
    }

    size_t horizontal_stride; // Note: This is commonly known as "pitch"
    size_t pixel_clock_in_khz;

    size_t horizontal_active;
    size_t horizontal_front_porch_pixels;
    size_t horizontal_sync_time_pixels;
    size_t horizontal_blank_pixels;

    size_t vertical_active;
    size_t vertical_front_porch_lines;
    size_t vertical_sync_time_lines;
    size_t vertical_blank_lines;

    size_t horizontal_offset; // Note: This is commonly known as "x offset"
    size_t vertical_offset;   // Note: This is commonly known as "y offset"
};

struct Encoder {
    ModeSetting mode_setting;
    bool enabled { false };
};

}
