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

#include <Kernel/Arch/i386/CPU.h>
#include <Kernel/Assertions.h>
#include <Kernel/Interrupts/InterruptSupervisor.h>
#include <Kernel/Interrupts/InterruptManagement.h>
#include <Kernel/Interrupts/PIC.h>

namespace Kernel {

InterruptSupervisor::InterruptSupervisor()
{
    register_itself();
}

InterruptSupervisor::InterruptSupervisor(size_t expected_handlers_count)
{
    m_handlers.ensure_capacity(expected_handlers_count);
    register_itself();
}

InterruptSupervisor::~InterruptSupervisor()
{
}
void InterruptSupervisor::register_itself() const
{
    InterruptManagement::the().register_interrupt_supervisor(*this);
}

void InterruptSupervisor::register_handler(const GenericInterruptHandler& handler)
{
    // FIXME: When SMP is enabled, using InterruptDisabler is not enough.
    InterruptDisabler disabler;
    ASSERT(!m_handlers.find_first_index(const_cast<GenericInterruptHandler*>(&handler)).has_value());
    m_handlers.append(const_cast<GenericInterruptHandler*>(&handler));
}
void InterruptSupervisor::unregister_handler(const GenericInterruptHandler& handler)
{
    // FIXME: When SMP is enabled, using InterruptDisabler is not enough.
    InterruptDisabler disabler;
    auto remove_index = m_handlers.find_first_index(const_cast<GenericInterruptHandler*>(&handler));
    ASSERT(remove_index.has_value());
    m_handlers.remove(remove_index.value());
}

}
