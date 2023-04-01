/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <LibEDID/EDID.h>

namespace Kernel {

struct DisplayConnector {
    enum class Type {
        Virtual,
        Analog,
        DVO,
        LVDS,
        TVOut,
        HDMI,
        DisplayPort,
        EmbeddedDisplayPort,
    };

    enum class State {
        Disconnected,
        Connected,
    };

    EDID::Parser::RawBytes edid_bytes {};
    bool edid_valid { false };

    State const m_state { State::Disconnected };
    Type const m_type { Type::Virtual };
};
}
