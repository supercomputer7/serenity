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
#include <Kernel/Devices/GPU/DisplayConnector.h>
#include <Kernel/Devices/GPU/GPUDevice.h>
#include <Kernel/Devices/GPU/Generic/DisplayConnector.h>
#include <Kernel/Devices/GPU/VirtIO/GraphicsAdapter.h>
#include <Kernel/Library/NonnullLockRefPtr.h>
#include <Kernel/Memory/Region.h>

namespace Kernel {

class GraphicsManagement {

public:
    static GraphicsManagement& the();
    static bool is_initialized();
    void initialize();

    unsigned allocate_minor_device_number() { return m_current_minor_number++; }
    GraphicsManagement();

    void set_vga_text_mode_cursor(size_t console_width, size_t x, size_t y);
    void disable_vga_text_mode_console_cursor();
    void disable_vga_emulation_access_permanently();

    LockRefPtr<Graphics::Console> console() const { return m_console; }
    void set_console(Graphics::Console&);

private:
    void enable_vga_text_mode_console_cursor();

    void initialize_preset_resolution_generic_display_connector();

    Vector<NonnullLockRefPtr<GPUDevice>> m_graphics_devices;
    LockRefPtr<Graphics::Console> m_console;

    // Note: This is only used when booting with kernel commandline that includes "graphics_subsystem_mode=limited"
    LockRefPtr<GenericDisplayConnector> m_preset_resolution_generic_display_connector;

    LockRefPtr<DisplayConnector> m_platform_board_specific_display_connector;

    unsigned m_current_minor_number { 0 };

#if ARCH(X86_64)
    OwnPtr<VGAIOArbiter> m_vga_arbiter;
#endif
};

}
