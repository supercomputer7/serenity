/*
 * Copyright (c) 2020, Liav A. <liavalb@hotmail.co.il>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/Singleton.h>
#include <AK/StringView.h>
#include <Kernel/ACPI/MultiProcessorParser.h>
#include <Kernel/API/Syscall.h>
#include <Kernel/CommandLine.h>
#include <Kernel/IO.h>
#include <Kernel/Interrupts/APIC.h>
#include <Kernel/Interrupts/IOAPIC.h>
#include <Kernel/Interrupts/InterruptManagement.h>
#include <Kernel/Interrupts/PIC.h>
#include <Kernel/Interrupts/SharedIRQHandler.h>
#include <Kernel/Interrupts/SpuriousInterruptHandler.h>
#include <Kernel/Interrupts/UnhandledInterruptHandler.h>
#include <Kernel/VM/MemoryManager.h>
#include <Kernel/VM/TypedMapping.h>

#define PCAT_COMPAT_FLAG 0x1

namespace Kernel {

template<typename LockType, typename GenericInterruptHandler>
class NO_DISCARD ScopedInterruptSpinLock {
    AK_MAKE_NONCOPYABLE(ScopedInterruptSpinLock);

public:
    ScopedInterruptSpinLock() = delete;
    ScopedInterruptSpinLock& operator=(ScopedInterruptSpinLock&&) = delete;

    ScopedInterruptSpinLock(LockType& lock, GenericInterruptHandler* handler)
        : m_lock(&lock)
    {
        ASSERT(m_lock);
        ASSERT(handler);
        m_prev_flags = m_lock->lock();
        m_have_lock = true;
        if (handler->is_global_to_all_cpus()) {
            unlock();
        }
    }

    ~ScopedInterruptSpinLock()
    {
        if (m_lock && m_have_lock) {
            m_lock->unlock(m_prev_flags);
        }
    }

    ALWAYS_INLINE void lock()
    {
        ASSERT(m_lock);
        ASSERT(!m_have_lock);
        m_prev_flags = m_lock->lock();
        m_have_lock = true;
    }

    ALWAYS_INLINE void unlock()
    {
        ASSERT(m_lock);
        ASSERT(m_have_lock);
        m_lock->unlock(m_prev_flags);
        m_prev_flags = 0;
        m_have_lock = false;
    }

    [[nodiscard]] ALWAYS_INLINE bool have_lock() const
    {
        return m_have_lock;
    }

private:
    LockType* m_lock { nullptr };
    u32 m_prev_flags { 0 };
    bool m_have_lock { false };
};

static AK::Singleton<InterruptManagement> s_interrupt_management;

bool InterruptManagement::initialized()
{
    return (s_interrupt_management.is_initialized());
}

InterruptManagement& InterruptManagement::the()
{
    return *s_interrupt_management;
}

static void revert_to_unused_handler(u8 interrupt_number)
{
    new UnhandledInterruptHandler(interrupt_number);
}

void InterruptManagement::register_interrupt_handler(u8 interrupt_number, GenericInterruptHandler& handler)
{
    ScopedSpinLock lock(m_locks[interrupt_number]);
    ASSERT(interrupt_number < GENERIC_INTERRUPT_HANDLERS_COUNT);

    if (m_initializing_interrupt_handlers) {
        m_interrupt_handlers[interrupt_number] = &handler;
        return;
    }

    if (m_interrupt_handlers[interrupt_number]) {
        if (m_interrupt_handlers[interrupt_number]->type() == HandlerType::UnhandledInterruptHandler) {
            m_interrupt_handlers[interrupt_number] = &handler;
            return;
        }
        if (m_interrupt_handlers[interrupt_number]->is_shared_handler() && !m_interrupt_handlers[interrupt_number]->is_sharing_with_others()) {
            ASSERT(m_interrupt_handlers[interrupt_number]->type() == HandlerType::SharedIRQHandler);
            static_cast<SharedIRQHandler*>(m_interrupt_handlers[interrupt_number])->register_handler(handler);
            return;
        }
        if (!m_interrupt_handlers[interrupt_number]->is_shared_handler()) {
            if (m_interrupt_handlers[interrupt_number]->type() == HandlerType::SpuriousInterruptHandler) {
                static_cast<SpuriousInterruptHandler*>(m_interrupt_handlers[interrupt_number])->register_handler(handler);
                return;
            }
            ASSERT(m_interrupt_handlers[interrupt_number]->type() == HandlerType::IRQHandler);
            auto& previous_handler = *m_interrupt_handlers[interrupt_number];
            m_interrupt_handlers[interrupt_number] = nullptr;
            SharedIRQHandler::initialize(interrupt_number);
            static_cast<SharedIRQHandler*>(m_interrupt_handlers[interrupt_number])->register_handler(previous_handler);
            static_cast<SharedIRQHandler*>(m_interrupt_handlers[interrupt_number])->register_handler(handler);
            return;
        }
        ASSERT_NOT_REACHED();
    } else {
        m_interrupt_handlers[interrupt_number] = &handler;
    }
}

void InterruptManagement::unregister_interrupt_handler(u8 interrupt_number, GenericInterruptHandler& handler)
{
    ScopedSpinLock lock(m_locks[interrupt_number]);
    ASSERT(m_interrupt_handlers[interrupt_number] != nullptr);
    if (m_interrupt_handlers[interrupt_number]->type() == HandlerType::UnhandledInterruptHandler) {
        dbg() << "Trying to unregister unused handler (?)";
        return;
    }
    if (m_interrupt_handlers[interrupt_number]->is_shared_handler() && !m_interrupt_handlers[interrupt_number]->is_sharing_with_others()) {
        ASSERT(m_interrupt_handlers[interrupt_number]->type() == HandlerType::SharedIRQHandler);
        static_cast<SharedIRQHandler*>(m_interrupt_handlers[interrupt_number])->unregister_handler(handler);
        if (!static_cast<SharedIRQHandler*>(m_interrupt_handlers[interrupt_number])->sharing_devices_count()) {
            revert_to_unused_handler(interrupt_number);
        }
        return;
    }
    if (!m_interrupt_handlers[interrupt_number]->is_shared_handler()) {
        ASSERT(m_interrupt_handlers[interrupt_number]->type() == HandlerType::IRQHandler);
        revert_to_unused_handler(interrupt_number);
        return;
    }
    ASSERT_NOT_REACHED();
}



void InterruptManagement::handle_interrupt(u8 interrupt_vector, const RegisterState& regs)
{
    if (interrupt_vector != 172)
    ScopedInterruptSpinLock lock = ScopedInterruptSpinLock(m_locks[interrupt_vector], m_interrupt_handlers[interrupt_vector]);
    auto* handler = m_interrupt_handlers[interrupt_vector];
    handler->increment_invoking_counter();
    handler->handle_interrupt(regs);
    handler->eoi();
}

void InterruptManagement::initialize()
{
    InterruptManagement::the();
    ASSERT(InterruptManagement::the().initializing_interrupt_handlers());
    for (size_t index = 0; index < GENERIC_INTERRUPT_HANDLERS_COUNT; index++)
        new UnhandledInterruptHandler(index);
    InterruptManagement::the().set_end_of_initializing_interrupt_handlers();
    APIC::initialize();

    if (kernel_command_line().lookup("smp").value_or("off") == "on")
        InterruptManagement::the().switch_to_ioapic_mode();
    else
        InterruptManagement::the().switch_to_pic_mode();
    Syscall::initialize();
}

void InterruptManagement::enumerate_interrupt_handlers(Function<void(GenericInterruptHandler&)> callback)
{
    for (int i = 0; i < GENERIC_INTERRUPT_HANDLERS_COUNT; i++) {
        auto* handler = m_interrupt_handlers[i];
        ASSERT(handler != nullptr);
        if (handler->type() != HandlerType::UnhandledInterruptHandler)
            callback(*handler);
    }
}

IRQController& InterruptManagement::get_interrupt_controller(int index)
{
    ASSERT(index >= 0);
    ASSERT(!m_interrupt_controllers[index].is_null());
    return *m_interrupt_controllers[index];
}

u8 InterruptManagement::get_mapped_interrupt_vector(u8 original_irq)
{
    // FIXME: For SMP configuration (with IOAPICs) use a better routing scheme to make redirections more efficient.
    // FIXME: Find a better way to handle conflict with Syscall interrupt gate.
    if (InterruptManagement::the().initializing_interrupt_handlers())
        return original_irq;
    ASSERT((original_irq + IRQ_VECTOR_BASE) != syscall_vector);
    return original_irq;
}

RefPtr<IRQController> InterruptManagement::get_responsible_irq_controller(u8 interrupt_vector)
{
    if (m_interrupt_controllers.size() == 1 && m_interrupt_controllers[0]->type() == IRQControllerType::i8259) {
        return m_interrupt_controllers[0];
    }
    for (auto irq_controller : m_interrupt_controllers) {
        if (irq_controller->gsi_base() <= interrupt_vector)
            if (!irq_controller->is_hard_disabled())
                return irq_controller;
    }
    ASSERT_NOT_REACHED();
}

PhysicalAddress InterruptManagement::search_for_madt()
{
    dbg() << "Early access to ACPI tables for interrupt setup";
    auto rsdp = ACPI::StaticParsing::find_rsdp();
    if (!rsdp.has_value())
        return {};
    return ACPI::StaticParsing::find_table(rsdp.value(), "APIC");
}

InterruptManagement::InterruptManagement()
    : m_madt(search_for_madt())
{
    m_interrupt_controllers.resize(1);
}

void InterruptManagement::switch_to_pic_mode()
{
    klog() << "Interrupts: Switch to Legacy PIC mode";
    InterruptDisabler disabler;
    m_smp_enabled = false;
    m_interrupt_controllers[0] = adopt(*new PIC());
    SpuriousInterruptHandler::initialize(7);
    SpuriousInterruptHandler::initialize(15);
    for (auto& irq_controller : m_interrupt_controllers) {
        ASSERT(irq_controller);
        if (irq_controller->type() == IRQControllerType::i82093AA) {
            irq_controller->hard_disable();
            dbg() << "Interrupts: Detected " << irq_controller->model() << " - Disabled";
        } else {
            dbg() << "Interrupts: Detected " << irq_controller->model();
        }
    }
}

void InterruptManagement::switch_to_ioapic_mode()
{
    klog() << "Interrupts: Switch to IOAPIC mode";
    InterruptDisabler disabler;

    if (m_madt.is_null()) {
        dbg() << "Interrupts: ACPI MADT is not available, reverting to PIC mode";
        switch_to_pic_mode();
        return;
    }

    dbg() << "Interrupts: MADT @ P " << m_madt.as_ptr();
    locate_apic_data();
    m_smp_enabled = true;
    if (m_interrupt_controllers.size() == 1) {
        if (get_interrupt_controller(0).type() == IRQControllerType::i8259) {
            klog() << "Interrupts: NO IOAPIC detected, Reverting to PIC mode.";
            return;
        }
    }
    for (auto& irq_controller : m_interrupt_controllers) {
        ASSERT(irq_controller);
        if (irq_controller->type() == IRQControllerType::i8259) {
            irq_controller->hard_disable();
            dbg() << "Interrupts: Detected " << irq_controller->model() << " Disabled";
        } else {
            dbg() << "Interrupts: Detected " << irq_controller->model();
        }
    }

    if (auto mp_parser = MultiProcessorParser::autodetect()) {
        m_pci_interrupt_overrides = mp_parser->get_pci_interrupt_redirections();
    }

    APIC::the().init_bsp();
}

void InterruptManagement::locate_apic_data()
{
    ASSERT(!m_madt.is_null());
    auto madt = map_typed<ACPI::Structures::MADT>(m_madt);

    int irq_controller_count = 0;
    if (madt->flags & PCAT_COMPAT_FLAG) {
        m_interrupt_controllers[0] = adopt(*new PIC());
        irq_controller_count++;
    }
    size_t entry_index = 0;
    size_t entries_length = madt->h.length - sizeof(ACPI::Structures::MADT);
    auto* madt_entry = madt->entries;
    while (entries_length > 0) {
        size_t entry_length = madt_entry->length;
        if (madt_entry->type == (u8)ACPI::Structures::MADTEntryType::IOAPIC) {
            auto* ioapic_entry = (const ACPI::Structures::MADTEntries::IOAPIC*)madt_entry;
            dbg() << "IOAPIC found @ MADT entry " << entry_index << ", MMIO Registers @ " << PhysicalAddress(ioapic_entry->ioapic_address);
            m_interrupt_controllers.resize(1 + irq_controller_count);
            m_interrupt_controllers[irq_controller_count] = adopt(*new IOAPIC(PhysicalAddress(ioapic_entry->ioapic_address), ioapic_entry->gsi_base));
            irq_controller_count++;
        }
        if (madt_entry->type == (u8)ACPI::Structures::MADTEntryType::InterruptSourceOverride) {
            auto* interrupt_override_entry = (const ACPI::Structures::MADTEntries::InterruptSourceOverride*)madt_entry;
            m_isa_interrupt_overrides.empend(
                interrupt_override_entry->bus,
                interrupt_override_entry->source,
                interrupt_override_entry->global_system_interrupt,
                interrupt_override_entry->flags);
            dbg() << "Interrupts: Overriding INT 0x" << String::format("%x", interrupt_override_entry->source) << " with GSI " << interrupt_override_entry->global_system_interrupt << ", for bus 0x" << String::format("%x", interrupt_override_entry->bus);
        }
        madt_entry = (ACPI::Structures::MADTEntryHeader*)(VirtualAddress(madt_entry).offset(entry_length).get());
        entries_length -= entry_length;
        entry_index++;
    }
}

}
