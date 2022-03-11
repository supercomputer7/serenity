/*
 * Copyright (c) 2021-2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullOwnPtr.h>
#include <AK/NonnullRefPtr.h>
#include <AK/NonnullRefPtrVector.h>
#include <AK/Types.h>
#include <Kernel/Bus/PCI/Definitions.h>
#include <Kernel/Graphics/Console/Console.h>
#include <Kernel/Graphics/GenericGraphicsAdapter.h>
#include <Kernel/Graphics/VGA/GenericAdapter.h>
#include <Kernel/Graphics/DisplayConnector.h>
#include <Kernel/Memory/Region.h>

namespace Kernel {

class GraphicsManagement {

public:
    static GraphicsManagement& the();
    static bool is_initialized();
    bool initialize();

    unsigned allocate_minor_device_number() { return m_current_minor_number++; };
    GraphicsManagement();

    void attach_new_display_connector(Badge<DisplayConnector>, DisplayConnector&);
    void detach_display_connector(Badge<DisplayConnector>, DisplayConnector&);

    bool console_mode_only() const;
    bool use_bootloader_framebuffer_only() const;

    Spinlock& main_vga_lock() { return m_main_vga_lock; }
    RefPtr<Graphics::Console> console() const { return m_console; }
    void set_console(Graphics::Console&);

    void deactivate_graphical_mode();
    void activate_graphical_mode();

private:
    bool determine_and_initialize_graphics_device(PCI::DeviceIdentifier const&);
    bool initialize_isa_graphics_device();
    bool initialize_isa_graphics_device_with_preset_resolution(PhysicalAddress framebuffer_address, size_t width, size_t height, size_t pitch);

    NonnullRefPtrVector<GenericGraphicsAdapter> m_graphics_devices;
    RefPtr<Graphics::Console> m_console;

    // Note: there could be multiple VGA adapters, but only one can operate in VGA mode
    RefPtr<VGAGenericAdapter> m_vga_adapter;
    unsigned m_current_minor_number { 0 };

    IntrusiveList<&DisplayConnector::m_list_node> m_display_connector_nodes;

    Spinlock m_main_vga_lock;
};

}
