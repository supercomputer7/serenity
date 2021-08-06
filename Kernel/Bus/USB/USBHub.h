/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Bitmap.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/Platform.h>
#include <AK/RefCounted.h>
#include <AK/Result.h>
#include <AK/WeakPtr.h>
#include <Kernel/Bus/USB/Definitions.h>
#include <Kernel/Bus/USB/HostController.h>
#include <Kernel/Bus/USB/USBDevice.h>

namespace Kernel::USB {

class Hub : public USB::Device {
public:
    enum class LocateError {
        HubPoweredDown = 1,
        NotAvailable,
        UnknownError,
    };

public:
    virtual bool initialize() = 0;

    virtual bool hotplug_event_occured() const = 0;
    virtual bool enumerate() = 0;
    virtual bool power_down() = 0;
    virtual bool power_up() = 0;

    virtual u8 power_mode_bits() = 0;
    virtual Result<RefPtr<USB::Device>, LocateError> try_to_find(Address) = 0;

    virtual ~Hub() = default;

protected:
    // Root hubs are technically not a USB device, but they're represented as such
    explicit Hub(const HostController&);

private:
    //explicit Hub(const HostController&, const Hub& parent_hub);
};

class UHCIController;
class UHCIRootHub final
    : public Hub {

public:
    virtual bool initialize() override;
    virtual bool hotplug_event_occured() const override;
    virtual bool enumerate() override;
    virtual bool power_down() override;
    virtual bool power_up() override;
    virtual u8 power_mode_bits() override;
    virtual Result<RefPtr<USB::Device>, Hub::LocateError> try_to_find(Address) override;

protected:
    explicit UHCIRootHub(const UHCIController&);

private:
    void handle_hotplug_event(size_t port_index);

    void spawn_hotplug_detection_process();
    bool m_initialized { false };
    Bitmap m_connected_devices_map { 128, false };
    WeakPtr<UHCIController> m_host_controller;
    Array<RefPtr<USB::Device>, 2> m_connected_devices;
};

}
