/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Format.h>
#include <AK/Types.h>

namespace Kernel::IntelGraphics {

class GPUVirtualAddress {
public:
    GPUVirtualAddress() = default;
    constexpr explicit GPUVirtualAddress(FlatPtr address)
        : m_address(address)
    {
    }

    [[nodiscard]] constexpr bool is_null() const { return m_address == 0; }
    [[nodiscard]] constexpr bool is_page_aligned() const { return (m_address & 0xfff) == 0; }

    [[nodiscard]] constexpr GPUVirtualAddress offset(FlatPtr o) const { return GPUVirtualAddress(m_address + o); }
    [[nodiscard]] constexpr FlatPtr get() const { return m_address; }
    void set(FlatPtr address) { m_address = address; }
    void mask(FlatPtr m) { m_address &= m; }

    bool operator<=(GPUVirtualAddress const& other) const { return m_address <= other.m_address; }
    bool operator>=(GPUVirtualAddress const& other) const { return m_address >= other.m_address; }
    bool operator>(GPUVirtualAddress const& other) const { return m_address > other.m_address; }
    bool operator<(GPUVirtualAddress const& other) const { return m_address < other.m_address; }
    bool operator==(GPUVirtualAddress const& other) const { return m_address == other.m_address; }
    bool operator!=(GPUVirtualAddress const& other) const { return m_address != other.m_address; }

    [[nodiscard]] GPUVirtualAddress page_base() const { return GPUVirtualAddress(m_address & ~(FlatPtr)0xfffu); }

private:
    FlatPtr m_address { 0 };
};

}

inline Kernel::IntelGraphics::GPUVirtualAddress operator-(Kernel::IntelGraphics::GPUVirtualAddress const& a, Kernel::IntelGraphics::GPUVirtualAddress const& b)
{
    return Kernel::IntelGraphics::GPUVirtualAddress(a.get() - b.get());
}

template<>
struct AK::Formatter<Kernel::IntelGraphics::GPUVirtualAddress> : AK::Formatter<FormatString> {
    ErrorOr<void> format(FormatBuilder& builder, Kernel::IntelGraphics::GPUVirtualAddress const& value)
    {
        return AK::Formatter<FormatString>::format(builder, "V (Intel GPU) {:x}", value.get());
    }
};
