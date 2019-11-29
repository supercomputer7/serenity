#pragma once

#include <AK/Types.h>
#include <Kernel/SharedIRQHandler.h>

class DeviceIRQHandler {
public:
    virtual ~DeviceIRQHandler();
    virtual void handle_irq();
    virtual void safely_shutdown();

    u8 irq_number() const { return m_irq_number; }

    void enable_irq();
    void disable_irq();

protected:
    friend class SharedIRQHandler;
    virtual void init_interrupts();
    explicit DeviceIRQHandler(u8 irq);
    DeviceIRQHandler();
    void set_interrupt_number(u8 irq);
    void safely_set_interrupt_number(u8 irq);

private:
    u8 m_irq_number { 0 };
};
