#include "SharedIRQHandler.h"
#include <Kernel/Arch/i386/CPU.h>
#include <Kernel/Arch/i386/PIC.h>
#include <Kernel/DeviceIRQHandler.h>

static DeviceIRQHandler* s_handler;

DeviceIRQHandler* SharedIRQHandler::get_unimplemented_handler()
{
    return s_handler;
}

SharedIRQHandler::SharedIRQHandler(u8 irq)
    : m_irq_number(irq)
{
    s_handler = new DeviceIRQHandler;
    for (int i = 0; i < 32; ++i) {
        m_irq_handlers[i] = SharedIRQHandler::get_unimplemented_handler();
    }
}

SharedIRQHandler::~SharedIRQHandler()
{
    // FIXME: We should call the destructor during Shutdown or Reboot operations only...
    // FIXME: Do a reasonable actions to shutdown all the derived IRQHandler(s) connected to this handler
    for (int i = 0; i < 32; ++i) {
        if ((m_dihar & (1 << i)) != 0) // DeviceIRQHandler must be assigned to be safely shutdown!
            m_irq_handlers[i]->safely_shutdown();
    }
}

void SharedIRQHandler::register_device_irq_handler(DeviceIRQHandler& device_handler)
{
    u32 dihar = m_dihar;
    for (int i = 0; i < 32; ++i) {
        if ((dihar & 0x1) == 0) {
            m_irq_handlers[i] = &device_handler;
            m_dihar |= (1 << i);
            break;
        }
        dihar >>= 1;
    }
}
void SharedIRQHandler::unregister_device_irq_handler(DeviceIRQHandler& device_handler)
{
    for (int i = 0; i < 32; ++i) {
        if (m_irq_handlers[i] == &device_handler) {
            m_irq_handlers[i] = SharedIRQHandler::get_unimplemented_handler();
            m_dihar &= (~(1 << i));
            break;
        }
    }
}

void SharedIRQHandler::handle_irq()
{
    for (int i = 0; i < 32; ++i) {
        if ((m_dihar & (1 << i)) != 0 && (m_dihmr & (1 << i)) == 0) // DeviceIRQHandler must be assigned and not masked!
            m_irq_handlers[i]->handle_irq();
    }
    PIC::eoi(m_irq_number);
}

void SharedIRQHandler::enable_irq()
{
    // TODO: Find a way to enable IRQs from MSI (Message Signaled Interrupts) or from I/O APICs
    PIC::enable(m_irq_number);
}

void SharedIRQHandler::disable_irq()
{
    // TODO: Find a way to disable IRQs from MSI (Message Signaled Interrupts) or from I/O APICs
    PIC::disable(m_irq_number);
}
