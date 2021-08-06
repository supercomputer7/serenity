/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullOwnPtr.h>
#include <AK/Platform.h>
#include <AK/RefCounted.h>
#include <Kernel/Bus/PCI/DeviceController.h>

namespace Kernel::USB {

class Transfer;
class HostController
    : public PCI::DeviceController
    , public RefCounted<USB::HostController> {

public:
    virtual ~HostController() = default;
    virtual bool initialize() = 0;

    virtual KResultOr<size_t> submit_control_transfer(Transfer& transfer) = 0;
    virtual u16 read_port_status(size_t index) const = 0;

protected:
    explicit HostController(PCI::Address address)
        : PCI::DeviceController(address)
    {
    }
};

}
