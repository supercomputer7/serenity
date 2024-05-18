/*
 * Copyright (c) 2021, Luke Wilde <lukew@serenityos.org>
 * Copyright (c) 2023, Jesse Buhagiar <jesse.buhagiar@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Singleton.h>
#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Bus/PCI/Definitions.h>
#include <Kernel/Bus/USB/USBCore.h>
#include <Kernel/Sections.h>

namespace Kernel::USB::Core {

USBController::List s_controllers;
static Singleton<SpinlockProtected<Vector<NonnullRefPtr<Driver>, LockRank::None>>> s_available_drivers;

void register_driver(NonnullRefPtr<Driver> driver)
{
    dbgln_if(USB_DEBUG, "Registering driver {}", driver->name());
    s_available_drivers.with([driver](auto& drivers) {
        drivers.append(driver);
    });
}

RefPtr<Driver> get_driver_by_name(StringView name)
{
    return s_available_drivers.with([name](auto& drivers) -> RefPtr<Driver> {
        auto it = drivers.find_if([name](auto driver) { return driver->name() == name; });
        return it.is_end() ? nullptr : RefPtr { *it };
    });
}

void unregister_driver(NonnullRefPtr<Driver> driver)
{
    auto& the_instance = the();
    dbgln_if(USB_DEBUG, "Unregistering driver {}", driver->name());
    s_available_drivers.with([driver](auto& drivers) {
        auto const& found_driver = drivers.find(driver);
        if (!found_driver.is_end())
            drivers.remove(found_driver.index());
    });
}

}
