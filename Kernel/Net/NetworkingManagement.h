/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Function.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <Kernel/Bus/PCI/Definitions.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Locking/SpinlockProtected.h>
#include <Kernel/Memory/Region.h>
#include <Kernel/Net/NetworkAdapter.h>

namespace Kernel {

class NetworkAdapter;
class NetworkingManagement {
    friend class NetworkAdapter;

public:
    static NetworkingManagement& the();
    static bool is_initialized();
    bool initialize();

    static ErrorOr<FixedStringBuffer<IFNAMSIZ>> generate_interface_name_from_pci_address(PCI::Device&);

    NetworkingManagement();

    void for_each(Function<void(NetworkAdapter&)>);
    ErrorOr<void> try_for_each(Function<ErrorOr<void>(NetworkAdapter&)>);

    RefPtr<NetworkAdapter> from_ipv4_address(IPv4Address const&) const;
    RefPtr<NetworkAdapter> lookup_by_name(StringView) const;

    NonnullRefPtr<NetworkAdapter> loopback_adapter() const;

    void attach_adapter(NetworkAdapter&);
    void detach_adapter(NetworkAdapter&);

private:
    SpinlockProtected<IntrusiveList<&NetworkAdapter::m_list_node>, LockRank::None> m_adapters {};
    RefPtr<NetworkAdapter> m_loopback_adapter;
};

}
