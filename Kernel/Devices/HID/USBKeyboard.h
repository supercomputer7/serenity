/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/CircularQueue.h>
#include <Kernel/API/MousePacket.h>
#include <Kernel/Bus/USB/USBDevice.h>
#include <Kernel/Devices/HID/KeyboardDevice.h>
#include <Kernel/Random.h>

namespace Kernel {
class USBKeyboard final
    : public USB::Device {
public:
    static RefPtr<USBKeyboard> try_to_initialize(const I8042Controller&);
    bool initialize();

    virtual ~PS2MouseDevice() override;

    virtual const char* purpose() const override { return class_name(); }

    // ^I8042Device
    virtual void irq_handle_byte_read(u8 byte) override;
    virtual void enable_interrupts() override
    {
        enable_irq();
    }

protected:
    explicit USBKeyboard(const HostController&, const Hub&);

    struct RawPacket {
        union {
            u32 dword;
            u8 bytes[4];
        };
    };

    u8 read_from_device();
    u8 send_command(u8 command);
    u8 send_command(u8 command, u8 data);
    MousePacket parse_data_packet(const RawPacket&);
    void set_sample_rate(u8);
    u8 get_device_id();

    u8 m_data_state { 0 };
    RawPacket m_data;
    bool m_has_wheel { false };
    bool m_has_five_buttons { false };
};

}
