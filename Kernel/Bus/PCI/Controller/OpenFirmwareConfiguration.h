struct PCIConfiguration {
    FlatPtr mmio_32bit_base { 0 };
    FlatPtr mmio_32bit_end { 0 };
    FlatPtr mmio_64bit_base { 0 };
    FlatPtr mmio_64bit_end { 0 };
    // The keys contains the bus, device & function at the same offsets as OpenFirmware PCI addresses,
    // with the least significant 8 bits being the interrupt pin.
    HashMap<PCIInterruptSpecifier, u64> masked_interrupt_mapping;
    PCIInterruptSpecifier interrupt_mask;
};

void HostController::configure_attached_devices(PCIConfiguration&);

void HostController::configure_attached_devices(PCIConfiguration& config)
{
    // First, Assign PCI-to-PCI bridge bus numbering
    u8 bus_id = 0;
    enumerate_attached_devices([this, &bus_id](EnumerableDeviceIdentifier const& device_identifier) {
        // called for each PCI device encountered (including bridges)
        if (read8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::CLASS) != PCI::ClassID::Bridge)
            return;
        if (read8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::SUBCLASS) != PCI::Bridge::SubclassID::PCI_TO_PCI)
            return;
        write8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::SECONDARY_BUS, ++bus_id);
        write8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::SUBORDINATE_BUS, 0xFF); },
        [this, &bus_id](EnumerableDeviceIdentifier const& device_identifier) {
            // called after a bridge was recursively enumerated
            write8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::SUBORDINATE_BUS, bus_id);
        });

    // Second, Assign BAR addresses & Interrupt numbers
    // TODO: We currently naively assign addresses bump-allocator style - Switch to a proper allocator if this is not good enough
    enumerate_attached_devices([this, &config](EnumerableDeviceIdentifier const& device_identifier) {
        // device-generic handling
        auto header_type = read8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::HEADER_TYPE);
        auto const max_bar = (header_type == 0) ? RegisterOffset::BAR5 : RegisterOffset::BAR1;
        for (u32 bar_offset = to_underlying(RegisterOffset::BAR0); bar_offset <= to_underlying(max_bar); bar_offset += 4) {
            auto bar_value = read32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset);
            auto bar_space_type = get_BAR_space_type(bar_value);
            auto bar_prefetchable = (bar_value >> 3) & 1;
            if (bar_space_type != BARSpaceType::Memory32BitSpace && bar_space_type != BARSpaceType::Memory64BitSpace)
                continue; // We only support memory-mapped BAR configuration at the moment
            if (bar_space_type == BARSpaceType::Memory32BitSpace) {
                write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset, 0xFFFFFFFF);
                auto bar_size = read32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset);
                write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset, bar_value);
                bar_size &= bar_address_mask;
                bar_size = (~bar_size) + 1;
                if (bar_size == 0)
                    continue;
                auto mmio_32bit_address = align_up_to(config.mmio_32bit_base, bar_size);
                if (mmio_32bit_address + bar_size <= config.mmio_32bit_end) {
                    write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset, mmio_32bit_address);
                    config.mmio_32bit_base = mmio_32bit_address + bar_size;
                    continue;
                }
                auto mmio_64bit_address = align_up_to(config.mmio_64bit_base, bar_size);
                if (bar_prefetchable && mmio_64bit_address + bar_size <= config.mmio_64bit_end && mmio_64bit_address + bar_size <= NumericLimits<u32>::max()) {
                    write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset, mmio_64bit_address);
                    config.mmio_64bit_base = mmio_64bit_address + bar_size;
                    continue;
                }
                dmesgln("PCI: Ran out of 32-bit MMIO address space");
                VERIFY_NOT_REACHED();
            }
            // 64-bit space
            auto next_bar_value = read32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset + 4);
            write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset, 0xFFFFFFFF);
            write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset + 4, 0xFFFFFFFF);
            u64 bar_size = read32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset);
            bar_size |= (u64)read32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset + 4) << 32;
            write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset, bar_value);
            write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset + 4, next_bar_value);
            bar_size &= bar_address_mask;
            bar_size = (~bar_size) + 1;
            if (bar_size == 0) {
                bar_offset += 4;
                continue;
            }
            auto mmio_64bit_address = align_up_to(config.mmio_64bit_base, bar_size);
            if (bar_prefetchable && mmio_64bit_address + bar_size <= config.mmio_64bit_end) {
                write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset, mmio_64bit_address & 0xFFFFFFFF);
                write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset + 4, mmio_64bit_address >> 32);
                config.mmio_64bit_base = mmio_64bit_address + bar_size;
                bar_offset += 4;
                continue;
            }
            auto mmio_32bit_address = align_up_to(config.mmio_32bit_base, bar_size);
            if (mmio_32bit_address + bar_size <= config.mmio_32bit_end) {
                write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset, mmio_32bit_address & 0xFFFFFFFF);
                write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), bar_offset + 4, mmio_32bit_address >> 32);
                config.mmio_32bit_base = mmio_32bit_address + bar_size;
                bar_offset += 4;
                continue;
            }
            dmesgln("PCI: Ran out of 64-bit MMIO address space");
            VERIFY_NOT_REACHED();
        }
        // enable memory space
        auto command_value = read16_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::COMMAND);
        command_value |= 1 << 1; // memory space enable
        write16_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::COMMAND, command_value);
        // assign interrupt number
        auto interrupt_pin = read8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::INTERRUPT_PIN);
        auto masked_identifier = PCIInterruptSpecifier{
            .interrupt_pin = interrupt_pin,
            .function = device_identifier.address().function(),
            .device = device_identifier.address().device(),
            .bus = device_identifier.address().bus()
        };
        masked_identifier &= config.interrupt_mask;
        auto interrupt_number = config.masked_interrupt_mapping.get(masked_identifier);
        if (interrupt_number.has_value())
            write8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::INTERRUPT_LINE, interrupt_number.value());

        if (read8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::CLASS) != PCI::ClassID::Bridge)
            return;
        if (read8_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::SUBCLASS) != PCI::Bridge::SubclassID::PCI_TO_PCI)
            return;
        // bridge-specific handling
        config.mmio_32bit_base = align_up_to(config.mmio_32bit_base, MiB);
        config.mmio_64bit_base = align_up_to(config.mmio_64bit_base, MiB);
        write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::MEMORY_BASE, config.mmio_32bit_base >> 16);
        write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::PREFETCHABLE_MEMORY_BASE, config.mmio_64bit_base >> 16);
        write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::PREFETCHABLE_MEMORY_BASE_UPPER_32_BITS, config.mmio_64bit_base >> 32); },
        [this, &config](EnumerableDeviceIdentifier const& device_identifier) {
            // called after a bridge was recursively enumerated
            config.mmio_32bit_base = align_up_to(config.mmio_32bit_base, MiB);
            config.mmio_64bit_base = align_up_to(config.mmio_64bit_base, MiB);
            write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::MEMORY_LIMIT, config.mmio_32bit_base >> 16);
            write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::PREFETCHABLE_MEMORY_LIMIT, config.mmio_64bit_base >> 16);
            write32_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::PREFETCHABLE_MEMORY_LIMIT_UPPER_32_BITS, config.mmio_64bit_base >> 32);
            // enable bridging
            auto command_value = read16_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::COMMAND);
            command_value |= 1 << 2; // enable forwarding of requests by the bridge
            write16_field(device_identifier.address().bus(), device_identifier.address().device(), device_identifier.address().function(), PCI::RegisterOffset::COMMAND, command_value);
        });
}

struct PCIInterruptSpecifier {
    u8 interrupt_pin { 0 };
    FunctionNumber function { 0 };
    DeviceNumber device { 0 };
    BusNumber bus { 0 };

    bool operator==(PCIInterruptSpecifier const& other) const
    {
        return bus == other.bus && device == other.device && function == other.function && interrupt_pin == other.interrupt_pin;
    }
    PCIInterruptSpecifier operator&(PCIInterruptSpecifier other) const
    {
        return PCIInterruptSpecifier {
            .interrupt_pin = static_cast<u8>(interrupt_pin & other.interrupt_pin),
            .function = function.value() & other.function.value(),
            .device = device.value() & other.device.value(),
            .bus = bus.value() & other.bus.value(),
        };
    }

    PCIInterruptSpecifier& operator&=(PCIInterruptSpecifier const& other)
    {
        *this = *this & other;
        return *this;
    }
};

}
namespace AK {
template<>
struct Traits<Kernel::PCI::PCIInterruptSpecifier> : public DefaultTraits<Kernel::PCI::PCIInterruptSpecifier> {
    static unsigned hash(Kernel::PCI::PCIInterruptSpecifier value)
    {
        return int_hash(value.bus.value() << 24 | value.device.value() << 16 | value.function.value() << 8 | value.interrupt_pin);
    }
};

}
