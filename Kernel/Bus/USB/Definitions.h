/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullOwnPtr.h>
#include <AK/Platform.h>
#include <AK/RefCounted.h>
#include <AK/Types.h>
#include <Kernel/Bus/PCI/DeviceController.h>

namespace Kernel::USB {

struct ID {
    u16 vendor_id { 0 };
    u16 device_id { 0 };

    bool is_null() const { return !vendor_id && !device_id; }

    bool operator==(const ID& other) const
    {
        return vendor_id == other.vendor_id && device_id == other.device_id;
    }
    bool operator!=(const ID& other) const
    {
        return vendor_id != other.vendor_id || device_id != other.device_id;
    }
};

struct Address {
public:
    Address() = default;
    Address(u8 bus, u8 device)
        : m_bus(bus)
        , m_device(device)
    {
    }

    Address(const Address& address)
        : m_bus(address.bus())
        , m_device(address.device())
    {
    }

    bool is_null() const { return !m_bus && !m_device; }
    operator bool() const { return !is_null(); }

    // Disable default implementations that would use surprising integer promotion.
    bool operator<=(const Address&) const = delete;
    bool operator>=(const Address&) const = delete;
    bool operator<(const Address&) const = delete;
    bool operator>(const Address&) const = delete;

    bool operator==(const Address& other) const
    {
        if (this == &other)
            return true;
        return m_bus == other.m_bus && m_device == other.m_device;
    }
    bool operator!=(const Address& other) const
    {
        return !(*this == other);
    }

    u8 bus() const { return m_bus; }
    u8 device() const { return m_device; }

protected:
    u8 m_bus { 0 };
    u8 m_device { 0 };
};

}
