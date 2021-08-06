/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Bus/USB/UHCIController.h>
#include <Kernel/Bus/USB/USBHub.h>

static constexpr u8 UHCI_ROOT_PORT_COUNT = 2;
static constexpr u16 UHCI_PORTSC_CURRRENT_CONNECT_STATUS = 0x0001;
static constexpr u16 UHCI_PORTSC_CONNECT_STATUS_CHANGED = 0x0002;
static constexpr u16 UHCI_PORTSC_PORT_ENABLED = 0x0004;
static constexpr u16 UHCI_PORTSC_PORT_ENABLE_CHANGED = 0x0008;
static constexpr u16 UHCI_PORTSC_LINE_STATUS = 0x0030;
static constexpr u16 UHCI_PORTSC_RESUME_DETECT = 0x40;
static constexpr u16 UHCI_PORTSC_LOW_SPEED_DEVICE = 0x0100;
static constexpr u16 UHCI_PORTSC_PORT_RESET = 0x0200;
static constexpr u16 UHCI_PORTSC_SUSPEND = 0x1000;

namespace Kernel::USB {

Hub::Hub(const HostController& host_controller)
    : USB::Device(host_controller, DeviceSpeed::HighSpeed)
{
}

UHCIRootHub::UHCIRootHub(const UHCIController& parent_controller)
    : Hub(parent_controller)
    , m_host_controller(parent_controller)
{
}

bool UHCIRootHub::initialize()
{
    return true;
}
bool UHCIRootHub::hotplug_event_occured() const
{
    auto controller = m_host_controller.strong_ref();
    return (controller->read_port_status(0) & UHCI_PORTSC_CONNECT_STATUS_CHANGED)
        || (controller->read_port_status(1) & UHCI_PORTSC_CONNECT_STATUS_CHANGED);
}
bool UHCIRootHub::enumerate()
{
    return false;
}
bool UHCIRootHub::power_down()
{
    return false;
}
bool UHCIRootHub::power_up()
{
    return false;
}
u8 UHCIRootHub::power_mode_bits()
{
    return 0;
}

Result<RefPtr<USB::Device>, Hub::LocateError> UHCIRootHub::try_to_find(Address device_address)
{
    if (m_connected_devices_map.get(device_address.device()))
        return LocateError::NotAvailable;
    // FIXME: When we start to support USB hubs, we should try to enumerate those too.
    if (m_connected_devices[0]->address() == device_address) {
        return m_connected_devices[0];
    }
    if (m_connected_devices[1]->address() == device_address) {
        return m_connected_devices[1];
    }
    return Hub::LocateError::NotAvailable;
}

void UHCIRootHub::handle_hotplug_event(size_t port_index)
{
    // FIXME: When we start to support USB hubs, we should try to enumerate those too.
    auto controller = m_host_controller.strong_ref();
    if (controller->read_port_status(port_index) & UHCI_PORTSC_CURRRENT_CONNECT_STATUS) {
        dmesgln("UHCI: Device attach detected on Root Port 1!");
        // Reset the port
        u16 port_data = controller->read_port_status(port_index);
        controller->write_port_status(port_index, port_data | UHCI_PORTSC_PORT_RESET);
        IO::delay(500);

        controller->write_port_status(port_index, port_data & ~UHCI_PORTSC_PORT_RESET);
        IO::delay(500);

        controller->write_port_status(port_index, port_data & (~UHCI_PORTSC_PORT_ENABLE_CHANGED | ~UHCI_PORTSC_CONNECT_STATUS_CHANGED));

        port_data = controller->read_port_status(0);
        controller->write_port_status(0, port_data | UHCI_PORTSC_PORT_ENABLED);
        dbgln("port should be enabled now: {:#04x}\n", controller->read_port_status(port_index));

        USB::Device::DeviceSpeed speed = (port_data & UHCI_PORTSC_LOW_SPEED_DEVICE) ? USB::Device::DeviceSpeed::LowSpeed : USB::Device::DeviceSpeed::FullSpeed;
        auto device = USB::Device::try_create(*controller, *this, speed);

        if (device.is_error())
            dmesgln("UHCI: Device creation failed on port 1 ({})", device.error());

        m_connected_devices.at(port_index) = device.value();
//        VERIFY(s_procfs_usb_bus_folder);
//        s_procfs_usb_bus_folder->plug(device.value());
    } else {
        // FIXME: Clean up (and properly) the RefPtr to the device in m_devices
        //VERIFY(s_procfs_usb_bus_folder);
        VERIFY(m_connected_devices.at(port_index));
        dmesgln("UHCI: Device detach detected on Root Port 1");
        //s_procfs_usb_bus_folder->unplug(*m_devices.at(0));
    }
}

void UHCIRootHub::spawn_hotplug_detection_process()
{
    RefPtr<Thread> usb_hotplug_thread;

    Process::create_kernel_process(usb_hotplug_thread, "UHCIHotplug", [&] {
        for (;;) {
            if (hotplug_event_occured()) {
                handle_hotplug_event(0);
                handle_hotplug_event(1);
            }
            (void)Thread::current()->sleep(Time::from_seconds(1));
        }
    });
}
}
