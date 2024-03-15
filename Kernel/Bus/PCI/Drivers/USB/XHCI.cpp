/*
 * Copyright (c) 2023, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Bus/PCI/Access.h>
#include <Kernel/Bus/PCI/Drivers/Driver.h>

namespace Kernel::PCI {

class XHCIDriver final : public PCI::Driver {
public:
    static void init();

    XHCIDriver()
        : Driver("XHCIDriver"sv)
    {
    }

    virtual ErrorOr<void> probe(PCI::Device&) override;
    virtual void detach(PCI::Device&) override;
    virtual ClassID class_id() const override { return PCI::ClassID::SerialBus; }
    virtual Span<HardwareIDMatch const> matches() override;
};

ErrorOr<void> XHCIDriver::probe(PCI::Device& pci_device)
{
    dbgln("XHCI @ {}: No actual current support for XHCI controllers, returning ENOTSUP", pci_device.device_id().address());
    return Error::from_errno(ENOTSUP);
}

void XHCIDriver::detach(PCI::Device&)
{
    TODO();
}

const static HardwareIDMatch __matches[] = {
    {
        .subclass_code = to_underlying(PCI::SerialBus::SubclassID::USB),
        .revision_id = Optional<RevisionID> {},
        .hardware_id = {
            0xffff,
            0xffff,
        },
        .subsystem_id_match = Optional<HardwareIDMatch::SubsystemIDMatch> {},
        .programming_interface = to_underlying(PCI::SerialBus::USBProgIf::xHCI),
    },
};

Span<HardwareIDMatch const> XHCIDriver::matches()
{
    return __matches;
}

void XHCIDriver::init()
{
    auto driver = MUST(adopt_nonnull_ref_or_enomem(new XHCIDriver()));
    PCI::Access::the().register_driver(driver);
}

PCI_DEVICE_DRIVER(XHCIDriver);

}
