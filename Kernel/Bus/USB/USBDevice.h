/*
 * Copyright (c) 2021, Jesse Buhagiar <jooster669@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/OwnPtr.h>
#include <AK/Types.h>
#include <Kernel/Bus/USB/USBPipe.h>
//#include <Kernel/Bus/USB/USBHub.h>
#include <Kernel/Bus/USB/Definitions.h>
#include <Kernel/KResult.h>

namespace Kernel::USB {

//
// Some nice info from FTDI on device enumeration and how some of this
// glues together:
//
// https://www.ftdichip.com/Support/Documents/TechnicalNotes/TN_113_Simplified%20Description%20of%20USB%20Device%20Enumeration.pdf
class Hub;
class Device : public RefCounted<Device> {
public:

    enum class DeviceSpeed : u8 {
        FullSpeed = 0,
        LowSpeed,
        HighSpeed,
    };

public:
    static KResultOr<NonnullRefPtr<Device>> try_create(const HostController&, const Hub&, DeviceSpeed);

    ~Device();

    KResult enumerate();

    DeviceSpeed speed() const { return m_device_speed; }
    Address address() const { return m_address; }

    const USBDeviceDescriptor& device_descriptor() const { return m_device_descriptor; }

protected:
    // This constructor is used by a Root hub device
    Device(const HostController&, DeviceSpeed);

private:
    Device(const HostController&, const Hub&, DeviceSpeed, NonnullOwnPtr<Pipe> default_pipe);

    DeviceSpeed m_device_speed; // What speed is this device running at
    Address m_address;         // USB address assigned to this device
    ID m_id;

    // Device description

    USBDeviceDescriptor m_device_descriptor; // Device Descriptor obtained from USB Device
    OwnPtr<Pipe> m_default_pipe; // Default communication pipe (endpoint0) used during enumeration
    
    RefPtr<USB::Hub> m_parent_hub;
    NonnullRefPtr<HostController> m_parent_controller;
};
}
