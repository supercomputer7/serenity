/*
 * Copyright (c) 2021-2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullOwnPtr.h>
#include <AK/Platform.h>
#include <AK/Types.h>
#if ARCH(X86_64)
#    include <Kernel/Arch/x86_64/VGA/IOArbiter.h>
#endif
#include <Kernel/Bus/PCI/Definitions.h>
#include <Kernel/Devices/GPU/Console/Console.h>
#include <Kernel/Devices/GPU/Device.h>
#include <Kernel/Devices/GPU/DisplayConnector.h>
#include <Kernel/Devices/GPU/Generic/DisplayConnector.h>
#include <Kernel/Devices/GPU/VirtIO/Adapter.h>
#include <Kernel/Library/NonnullLockRefPtr.h>
#include <Kernel/Memory/Region.h>

namespace Kernel {

class GPUManagement {

public:
    static GPUManagement& the();
    static bool is_initialized();
    void initialize();

    unsigned allocate_minor_device_number() { return m_current_minor_number++; };
    GPUManagement();

    void attach_new_display_connector(Badge<DisplayConnector>, DisplayConnector&);
    void detach_display_connector(Badge<DisplayConnector>, DisplayConnector&);

    void set_vga_text_mode_cursor(size_t console_width, size_t x, size_t y);
    void disable_vga_text_mode_console_cursor();
    void disable_vga_emulation_access_permanently();

    LockRefPtr<GPU::Console> console() const { return m_console; }
    void set_console(GPU::Console&);

    void deactivate_graphical_mode();
    void activate_graphical_mode();

private:
    void enable_vga_text_mode_console_cursor();

    ErrorOr<void> determine_and_initialize_gpu_device(PCI::DeviceIdentifier const&);

    void initialize_preset_resolution_generic_display_connector();

    LockRefPtr<GPU::Console> m_console;

    unsigned m_current_minor_number { 0 };

    SpinlockProtected<IntrusiveList<&GPUDevice::m_list_node>, LockRank::None> m_gpu_device_nodes {};
#if ARCH(X86_64)
    OwnPtr<VGAIOArbiter> m_vga_arbiter;
#endif
};

}
