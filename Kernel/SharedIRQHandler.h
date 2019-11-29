#pragma once

#include <AK/InlineLinkedList.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>

class DeviceIRQHandler;
class SharedIRQHandler {
public:
    explicit SharedIRQHandler(u8 irq);
    virtual ~SharedIRQHandler();
    void handle_irq();

    void register_device_irq_handler(DeviceIRQHandler&);
    void unregister_device_irq_handler(DeviceIRQHandler&);

    u8 irq_number() const { return m_irq_number; }

    void enable_irq();
    void disable_irq();

private:
    static DeviceIRQHandler* get_unimplemented_handler();

    u32 m_dihmr; /* DeviceIRQHandler(s) Mask Register */
    u32 m_dihar; /* DeviceIRQHandler(s) Assignment Register */
    DeviceIRQHandler* m_irq_handlers[32];
    u8 m_irq_number { 0 };
    bool waiting_for_irq { false };
};
