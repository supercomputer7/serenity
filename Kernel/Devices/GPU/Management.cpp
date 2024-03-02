/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Singleton.h>
#include <Kernel/Arch/Delay.h>
#if ARCH(X86_64)
#    include <Kernel/Arch/x86_64/Hypervisor/BochsDisplayConnector.h>
#endif
#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Boot/Multiboot.h>
#include <Kernel/Bus/PCI/IDs.h>
#include <Kernel/Devices/GPU/Bochs/GraphicsAdapter.h>
#include <Kernel/Devices/GPU/Console/BootFramebufferConsole.h>
#include <Kernel/Devices/GPU/Management.h>
#include <Kernel/Memory/AnonymousVMObject.h>
#include <Kernel/Sections.h>

namespace Kernel {

static Singleton<GraphicsManagement> s_the;

extern Atomic<Graphics::Console*> g_boot_console;

GraphicsManagement& GraphicsManagement::the()
{
    return *s_the;
}

bool GraphicsManagement::is_initialized()
{
    return s_the.is_initialized();
}

UNMAP_AFTER_INIT GraphicsManagement::GraphicsManagement()
{
}

void GraphicsManagement::disable_vga_emulation_access_permanently()
{
#if ARCH(X86_64)
    if (!m_vga_arbiter)
        return;
    m_vga_arbiter->disable_vga_emulation_access_permanently({});
#endif
}

void GraphicsManagement::enable_vga_text_mode_console_cursor()
{
#if ARCH(X86_64)
    if (!m_vga_arbiter)
        return;
    m_vga_arbiter->enable_vga_text_mode_console_cursor({});
#endif
}

void GraphicsManagement::disable_vga_text_mode_console_cursor()
{
#if ARCH(X86_64)
    if (!m_vga_arbiter)
        return;
    m_vga_arbiter->disable_vga_text_mode_console_cursor({});
#endif
}

void GraphicsManagement::set_vga_text_mode_cursor([[maybe_unused]] size_t console_width, [[maybe_unused]] size_t x, [[maybe_unused]] size_t y)
{
#if ARCH(X86_64)
    if (!m_vga_arbiter)
        return;
    m_vga_arbiter->set_vga_text_mode_cursor({}, console_width, x, y);
#endif
}

UNMAP_AFTER_INIT void GraphicsManagement::initialize_preset_resolution_generic_display_connector()
{
    VERIFY(!multiboot_framebuffer_addr.is_null());
    VERIFY(multiboot_framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB);
    dmesgln("Graphics: Using a preset resolution from the bootloader, without knowing the PCI device");
    m_preset_resolution_generic_display_connector = GenericDisplayConnector::must_create_with_preset_resolution(
        multiboot_framebuffer_addr,
        multiboot_framebuffer_width,
        multiboot_framebuffer_height,
        multiboot_framebuffer_pitch);
}

void GraphicsManagement::initialize()
{
#if ARCH(X86_64)
    m_vga_arbiter = VGAIOArbiter::must_create({});
#endif

    auto graphics_subsystem_mode = kernel_command_line().graphics_subsystem_mode();
    if (graphics_subsystem_mode == CommandLine::GraphicsSubsystemMode::Disabled) {
        VERIFY(!m_console);
        return;
    }

    // Note: Don't try to initialize an ISA Bochs VGA adapter if PCI hardware is
    // present but the user decided to disable its usage nevertheless.
    // Otherwise we risk using the Bochs VBE driver on a wrong physical address
    // for the framebuffer.
    if (PCI::Access::is_hardware_disabled() && !(graphics_subsystem_mode == CommandLine::GraphicsSubsystemMode::Limited && !multiboot_framebuffer_addr.is_null() && multiboot_framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB)) {
#if ARCH(X86_64)
        auto vga_isa_bochs_display_connector = BochsDisplayConnector::try_create_for_vga_isa_connector();
        if (vga_isa_bochs_display_connector) {
            dmesgln("Graphics: Using a Bochs ISA VGA compatible adapter");
            MUST(vga_isa_bochs_display_connector->set_safe_mode_setting());
            m_platform_board_specific_display_connector = vga_isa_bochs_display_connector;
            dmesgln("Graphics: Invoking manual blanking with VGA ISA ports");
            m_vga_arbiter->unblank_screen({});
            return;
        }
#endif
    }

    if (graphics_subsystem_mode == CommandLine::GraphicsSubsystemMode::Limited && !multiboot_framebuffer_addr.is_null() && multiboot_framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
        initialize_preset_resolution_generic_display_connector();
    }
}

void GraphicsManagement::set_console(Graphics::Console& console)
{
    m_console = console;

    if (auto* boot_console = g_boot_console.exchange(nullptr)) {
        // Disable the initial boot framebuffer console permanently
        boot_console->disable();
        // TODO: Even though we swapped the pointer and disabled the console
        // we technically can't safely destroy it as other CPUs might still
        // try to use it. Once we solve this problem we can drop the reference
        // that we intentionally leaked in Kernel::init().
    }
}

}
