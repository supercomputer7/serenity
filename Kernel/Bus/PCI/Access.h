/*
 * Copyright (c) 2020, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Bitmap.h>
#include <AK/HashMap.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/Try.h>
#include <AK/Vector.h>
#include <Kernel/Bus/PCI/Controller/HostController.h>
#include <Kernel/Bus/PCI/Definitions.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Bus/PCI/Drivers/Driver.h>
#include <Kernel/Locking/Spinlock.h>

namespace Kernel::PCI::Access {

static bool is_disabled();
static bool is_hardware_disabled();

static void set_disabled();
static void set_hardware_disabled();

ErrorOr<void> add_host_controller_and_scan_for_devices(NonnullRefPtr<HostController>);

static void register_driver(NonnullRefPtr<Driver> driver);
static void register_device(NonnullRefPtr<Device> device);
static RefPtr<Driver> get_driver_by_name(StringView name);
static void unregister_driver(NonnullRefPtr<Driver> driver);

}
