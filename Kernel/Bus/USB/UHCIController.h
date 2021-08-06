/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2020-2021, Jesse Buhagiar <jooster669@gmail.com>
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullOwnPtr.h>
#include <AK/Weakable.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Bus/USB/Definitions.h>
#include <Kernel/Bus/USB/HostController.h>
#include <Kernel/Bus/USB/UHCIDescriptorTypes.h>
#include <Kernel/Bus/USB/USBDevice.h>
#include <Kernel/Bus/USB/USBHub.h>
#include <Kernel/Bus/USB/USBTransfer.h>
#include <Kernel/IO.h>
#include <Kernel/Process.h>
#include <Kernel/Time/TimeManagement.h>
#include <Kernel/VM/AnonymousVMObject.h>

namespace Kernel::USB {

class UHCIController final
    : public HostController
    , public Weakable<UHCIController> {
    friend class UHCIRootHub;

public:
    static RefPtr<UHCIController> try_to_initialize(PCI::Address);
    virtual ~UHCIController() override;

    void reset();
    void stop();
    void start();

private:
    UHCIController(PCI::Address, PCI::ID);

    virtual bool initialize() override { return true; }

    virtual KResultOr<size_t> submit_control_transfer(Transfer& transfer) override;

    RefPtr<USB::Device> const get_device_from_address(Address device_address);

    u16 read_usbcmd() { return m_io_base.offset(0).in<u16>(); }
    u16 read_usbsts() { return m_io_base.offset(0x2).in<u16>(); }
    u16 read_usbintr() { return m_io_base.offset(0x4).in<u16>(); }
    u16 read_frnum() { return m_io_base.offset(0x6).in<u16>(); }
    u32 read_flbaseadd() { return m_io_base.offset(0x8).in<u32>(); }
    u8 read_sofmod() { return m_io_base.offset(0xc).in<u8>(); }

    virtual u16 read_port_status(size_t index) const override;
    void write_port_status(size_t index, u16 value);

    void write_usbcmd(u16 value) { m_io_base.offset(0).out(value); }
    void write_usbsts(u16 value) { m_io_base.offset(0x2).out(value); }
    void write_usbintr(u16 value) { m_io_base.offset(0x4).out(value); }
    void write_frnum(u16 value) { m_io_base.offset(0x6).out(value); }
    void write_flbaseadd(u32 value) { m_io_base.offset(0x8).out(value); }
    void write_sofmod(u8 value) { m_io_base.offset(0xc).out(value); }

    //virtual bool handle_irq(const RegisterState&) override;

    void do_debug_transfer();

    void create_structures();
    void setup_schedule();
    size_t poll_transfer_queue(QueueHead& transfer_queue);

    TransferDescriptor* create_transfer_descriptor(Pipe& pipe, PacketID direction, size_t data_len);
    KResult create_chain(Pipe& pipe, PacketID direction, Ptr32<u8>& buffer_address, size_t max_size, size_t transfer_size, TransferDescriptor** td_chain, TransferDescriptor** last_td);
    void free_descriptor_chain(TransferDescriptor* first_descriptor);

    QueueHead* allocate_queue_head() const;
    TransferDescriptor* allocate_transfer_descriptor() const;

private:
    IOAddress m_io_base;

    Vector<QueueHead*> m_free_qh_pool;
    Vector<TransferDescriptor*> m_free_td_pool;
    Vector<TransferDescriptor*> m_iso_td_list;

    QueueHead* m_interrupt_transfer_queue;
    QueueHead* m_lowspeed_control_qh;
    QueueHead* m_fullspeed_control_qh;
    QueueHead* m_bulk_qh;
    QueueHead* m_dummy_qh; // Needed for PIIX4 hack

    OwnPtr<Region> m_framelist;
    OwnPtr<Region> m_qh_pool;
    OwnPtr<Region> m_td_pool;

    RefPtr<UHCIRootHub> m_root_hub;
};

}
