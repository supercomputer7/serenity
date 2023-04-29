/*
 * Copyright (c) 2023, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Bitmap.h>
#include <AK/String.h>
#include <AK/Types.h>
#include <Kernel/API/DeviceFileTypes.h>

namespace SystemServer {

class RegisteredDeviceNode {
public:
    RegisteredDeviceNode(String device_path, MinorNumber minor)
        : m_device_path(move(device_path))
        , m_minor(minor)
    {
    }

    StringView device_path() const { return m_device_path.bytes_as_string_view(); }
    MinorNumber minor_number() const { return m_minor; }

private:
    String m_device_path;
    MinorNumber m_minor { 0 };
};

}

namespace AK {

template<>
struct Traits<SystemServer::RegisteredDeviceNode> : public GenericTraits<SystemServer::RegisteredDeviceNode> {
    static unsigned hash(SystemServer::RegisteredDeviceNode const& node)
    {
        return int_hash(node.minor_number().value());
    }

    static bool equals(SystemServer::RegisteredDeviceNode const& a, SystemServer::RegisteredDeviceNode const& b)
    {
        return a.minor_number() == b.minor_number();
    }
};

}
