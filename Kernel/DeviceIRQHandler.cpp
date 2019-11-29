#include "DeviceIRQHandler.h"
#include <Kernel/Arch/i386/CPU.h>
#include <Kernel/Arch/i386/PIC.h>

void DeviceIRQHandler::handle_irq()
{
    kprintf("Unimplemented IRQ handling.");
}

void DeviceIRQHandler::init_interrupts()
{
    kprintf("Unimplemented IRQ handler interrupts initialization.");
}

void DeviceIRQHandler::safely_shutdown()
{
    kprintf("Unimplemented IRQ handler shutting down.");
}

DeviceIRQHandler::DeviceIRQHandler()
    : m_irq_number(0)
{
}

DeviceIRQHandler::DeviceIRQHandler(u8 irq)
    : m_irq_number(irq)
{
}

DeviceIRQHandler::~DeviceIRQHandler()
{
}

void DeviceIRQHandler::set_interrupt_number(u8 irq)
{
    m_irq_number = irq;
}

void DeviceIRQHandler::safely_set_interrupt_number(u8 irq)
{
    disable_irq();
    m_irq_number = irq;
}

void DeviceIRQHandler::enable_irq()
{
    register_device_irq_handler(m_irq_number, *this);
}

void DeviceIRQHandler::disable_irq()
{
    unregister_device_irq_handler(m_irq_number, *this);
}