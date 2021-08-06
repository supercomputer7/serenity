/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Singleton.h>
#include <Kernel/Bus/USB/UHCIController.h>
#include <Kernel/Bus/USB/USBManagement.h>
#include <Kernel/CommandLine.h>
#include <Kernel/Sections.h>

namespace Kernel::USB {

static AK::Singleton<Management> s_the;

UNMAP_AFTER_INIT Management::Management()
{
}

UNMAP_AFTER_INIT RefPtr<HostController> Management::determine_host_controller_type(PCI::Address address) const
{
    if (auto candidate = UHCIController::try_to_initialize(address); !candidate.is_null())
        return candidate;
    return {};
}

UNMAP_AFTER_INIT void Management::enumerate()
{
    if (!kernel_command_line().disable_usb()) {
        PCI::enumerate([&](const PCI::Address& address, PCI::ID) {
            // Note: PCI class 0xC is the class of Serial bus controllers, subclass 0x3 for USB controllers
            if (PCI::get_class(address) != 0xC || PCI::get_subclass(address) != 0x3)
                return;
            if (auto host_controller = determine_host_controller_type(address); !host_controller.is_null())
                m_host_controllers.append(host_controller.release_nonnull());
        });
    }
}

UNMAP_AFTER_INIT void Management::initialize()
{
    VERIFY(!s_the.is_initialized());
    s_the.ensure_instance();
    s_the->enumerate();
}

Management& Management::the()
{
    return *s_the;
}

}
