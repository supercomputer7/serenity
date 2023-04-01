/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>

namespace Kernel {

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA 0x01CF

#define BOCHS_DISPLAY_LITTLE_ENDIAN 0x1e1e1e1e
#define BOCHS_DISPLAY_BIG_ENDIAN 0xbebebebe

#define VBE_DISPI_ID5 0xB0C5

enum class BochsFramebufferSettings {
    Enabled = 0x1,
    LinearFramebuffer = 0x40,
};

enum class BochsDISPIRegisters {
    ID = 0x0,
    XRES = 0x1,
    YRES = 0x2,
    BPP = 0x3,
    ENABLE = 0x4,
    BANK = 0x5,
    VIRT_WIDTH = 0x6,
    VIRT_HEIGHT = 0x7,
    X_OFFSET = 0x8,
    Y_OFFSET = 0x9,
    VIDEO_RAM_64K_CHUNKS_COUNT = 0xA,
};

}
