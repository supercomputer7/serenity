/*
 * Copyright (c) 2020-2022, Liav A. <liavalb@hotmail.co.il>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Platform.h>
#include <AK/Singleton.h>
#include <AK/StringView.h>
#include <AK/UUID.h>
#if ARCH(X86_64)
#    include <Kernel/Arch/x86_64/ISABus/IDEController.h>
#endif
#if ARCH(AARCH64)
#    include <Kernel/Arch/aarch64/RPi/SDHostController.h>
#endif
#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Bus/PCI/Access.h>
#include <Kernel/Bus/PCI/Controller/VolumeManagementDevice.h>
#include <Kernel/Devices/BlockDevice.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Devices/Storage/NVMe/NVMeController.h>
#include <Kernel/Devices/Storage/SD/SDHostController.h>
#include <Kernel/Devices/Storage/StorageManagement.h>
#include <Kernel/FileSystem/Ext2FS/FileSystem.h>
#include <Kernel/FileSystem/VirtualFileSystem.h>
#include <Kernel/Library/Panic.h>
#include <Kernel/Tasks/WorkQueue.h>
#include <LibPartition/EBRPartitionTable.h>
#include <LibPartition/GUIDPartitionTable.h>
#include <LibPartition/MBRPartitionTable.h>
#include <LibPartition/PartitionTable.h>

namespace Kernel {

static Singleton<StorageManagement> s_the;
static Atomic<u32> s_storage_device_minor_number;
static Atomic<u32> s_partition_device_minor_number;
static Atomic<u32> s_controller_id;

static Atomic<u32> s_relative_ata_controller_id;
static Atomic<u32> s_relative_nvme_controller_id;
static Atomic<u32> s_relative_sd_controller_id;

static constexpr StringView partition_uuid_prefix = "PARTUUID:"sv;

static constexpr StringView partition_number_prefix = "part"sv;
static constexpr StringView block_device_prefix = "block"sv;

static constexpr StringView ata_device_prefix = "ata"sv;
static constexpr StringView nvme_device_prefix = "nvme"sv;
static constexpr StringView logical_unit_number_device_prefix = "lun"sv;
static constexpr StringView sd_device_prefix = "sd"sv;

UNMAP_AFTER_INIT StorageManagement::StorageManagement()
{
}

u32 StorageManagement::generate_relative_nvme_controller_id(Badge<NVMeController>)
{
    auto controller_id = s_relative_nvme_controller_id.load();
    s_relative_nvme_controller_id++;
    return controller_id;
}

u32 StorageManagement::generate_relative_ata_controller_id(Badge<ATAController>)
{
    auto controller_id = s_relative_ata_controller_id.load();
    s_relative_ata_controller_id++;
    return controller_id;
}

u32 StorageManagement::generate_relative_sd_controller_id(Badge<SDHostController>)
{
    auto controller_id = s_relative_sd_controller_id.load();
    s_relative_sd_controller_id++;
    return controller_id;
}

UNMAP_AFTER_INIT void StorageManagement::dump_storage_devices_and_partitions() const
{
    m_storage_devices.with([&](auto& list) {
        dbgln("StorageManagement: Detected {} storage devices", list.size_slow());

        for (auto const& storage_device : list) {
            storage_device.partitions().with([&storage_device](auto& partitions_list) {
                if (partitions_list.is_empty()) {
                    dbgln("  Device: block{}:{} (no partitions)", storage_device.major(), storage_device.minor());
                } else {
                    dbgln("  Device: block{}:{} ({} partitions)", storage_device.major(), storage_device.minor(), partitions_list.size_slow());
                    unsigned partition_number = 1;
                    for (auto const& partition : partitions_list) {
                        dbgln("    Partition: {}, block{}:{} (UUID {})", partition_number, partition.major(), partition.minor(), partition.metadata().unique_guid().to_string());
                        partition_number++;
                    }
                }
            });
        }
    });
}

ErrorOr<NonnullOwnPtr<Partition::PartitionTable>> StorageManagement::try_to_initialize_partition_table(StorageDevice& device)
{
    auto mbr_table_or_error = Partition::MBRPartitionTable::try_to_initialize(device);
    if (!mbr_table_or_error.is_error())
        return mbr_table_or_error.release_value();
    auto ebr_table_or_error = Partition::EBRPartitionTable::try_to_initialize(device);
    if (!ebr_table_or_error.is_error()) {
        return ebr_table_or_error.release_value();
    }
    return TRY(Partition::GUIDPartitionTable::try_to_initialize(device));
}

class StorageDeviceIOWorkHolder {
public:
    static ErrorOr<NonnullOwnPtr<StorageDeviceIOWorkHolder>> create(StorageDevice& device)
    {
        return adopt_nonnull_own_or_enomem(new (nothrow) StorageDeviceIOWorkHolder(device));
    }

    ~StorageDeviceIOWorkHolder() = default;

    StorageDevice& device() { return *m_device; }

private:
    explicit StorageDeviceIOWorkHolder(StorageDevice& device)
        : m_device(device)
    {
    }
    NonnullRefPtr<StorageDevice> const m_device;
};

void StorageManagement::enumerate_device_holder_partitions(void* holder)
{
    auto& device = static_cast<StorageDeviceIOWorkHolder*>(holder)->device();
    auto partition_table_or_error = StorageManagement::try_to_initialize_partition_table(device);
    if (partition_table_or_error.is_error()) {
        dbgln("Storage: Failed to create partition table for {} device [controller:target:disk] {}:{}:{} due to {}",
            device.class_name(),
            device.logical_unit_number_address().controller_id,
            device.logical_unit_number_address().target_id,
            device.logical_unit_number_address().disk_id,
            partition_table_or_error.release_error());
        return;
    }
    auto partition_table = partition_table_or_error.release_value();
    device.partitions().with([&device, &partition_table](auto& partitions_list) {
        ArmedScopeGuard clear_list_on_failure([&partitions_list] {
            partitions_list.clear();
        });
        for (auto partition_metadata : partition_table->partitions()) {
            auto partition_or_error = StorageDevicePartition::create(device, generate_partition_minor_number(), partition_metadata);
            if (partition_or_error.is_error()) {
                dbgln("Storage: Failed to create partition for {} device [controller:target:disk] {}:{}:{} due to {}",
                    device.class_name(),
                    device.logical_unit_number_address().controller_id,
                    device.logical_unit_number_address().target_id,
                    device.logical_unit_number_address().disk_id,
                    partition_or_error.release_error());
                return;
            }
            partitions_list.append(partition_or_error.release_value());
        }
        clear_list_on_failure.disarm();
    });
}

static void remove_device_holder(void* holder)
{
    delete static_cast<StorageDeviceIOWorkHolder*>(holder);
}

ErrorOr<void> StorageManagement::enumerate_device_partitions(StorageDevice& device)
{
    auto device_holder = TRY(StorageDeviceIOWorkHolder::create(device));
    // NOTE: We use g_storage_detect_work instead of g_io_work because
    // detecting partitions later in the code will use g_io_work as well
    // to do async IO for that task.
    TRY(g_storage_detect_work->try_queue(enumerate_device_holder_partitions, (void*)device_holder.leak_ptr(), remove_device_holder));
    return {};
}

UNMAP_AFTER_INIT Optional<unsigned> StorageManagement::extract_boot_device_partition_number_parameter(StringView device_prefix)
{
    VERIFY(m_boot_argument.starts_with(device_prefix));
    VERIFY(!m_boot_argument.starts_with(partition_uuid_prefix));
    auto storage_device_relative_address_view = m_boot_argument.substring_view(device_prefix.length());
    auto parameter_view = storage_device_relative_address_view.find_last_split_view(';');
    if (parameter_view == storage_device_relative_address_view)
        return {};
    if (!parameter_view.starts_with(partition_number_prefix)) {
        PANIC("StorageManagement: Invalid root boot parameter.");
    }

    auto parameter_number = parameter_view.substring_view(partition_number_prefix.length()).to_number<unsigned>();
    if (!parameter_number.has_value()) {
        PANIC("StorageManagement: Invalid root boot parameter.");
    }

    return parameter_number.value();
}

UNMAP_AFTER_INIT Array<unsigned, 3> StorageManagement::extract_boot_device_address_parameters(StringView device_prefix)
{
    VERIFY(!m_boot_argument.starts_with(partition_uuid_prefix));
    Array<unsigned, 3> address_parameters;
    auto parameters_view = m_boot_argument.substring_view(device_prefix.length()).find_first_split_view(';');
    size_t parts_count = 0;
    bool parse_failure = false;
    parameters_view.for_each_split_view(':', SplitBehavior::Nothing, [&](StringView parameter_view) {
        if (parse_failure)
            return;
        if (parts_count > 2)
            return;
        auto parameter_number = parameter_view.to_number<unsigned>();
        if (!parameter_number.has_value()) {
            parse_failure = true;
            return;
        }
        address_parameters[parts_count] = parameter_number.value();
        parts_count++;
    });

    if (parts_count > 3) {
        dbgln("StorageManagement: Detected {} parts in boot device parameter.", parts_count);
        PANIC("StorageManagement: Invalid root boot parameter.");
    }
    if (parse_failure) {
        PANIC("StorageManagement: Invalid root boot parameter.");
    }

    return address_parameters;
}

UNMAP_AFTER_INIT void StorageManagement::resolve_partition_from_boot_device_parameter(StorageDevice const& chosen_storage_device, StringView boot_device_prefix)
{
    auto possible_partition_number = extract_boot_device_partition_number_parameter(boot_device_prefix);
    if (!possible_partition_number.has_value())
        return;

    auto partition_number = possible_partition_number.value();
    chosen_storage_device.partitions().with([this, partition_number](auto& partitions_list) {
        if ((partitions_list.size_slow() <= partition_number))
            return;
        auto it = partitions_list.begin();
        for (size_t index = 0; index < partition_number; index++)
            ++it;
        m_boot_block_device = *it;
        VERIFY(m_boot_block_device);
    });
}

UNMAP_AFTER_INIT void StorageManagement::determine_hardware_relative_boot_device(StringView relative_hardware_prefix, Function<bool(StorageDevice const&)> filter_device_callback)
{
    VERIFY(m_boot_argument.starts_with(relative_hardware_prefix));
    auto address_parameters = extract_boot_device_address_parameters(relative_hardware_prefix);

    RefPtr<StorageDevice> chosen_storage_device;
    m_storage_devices.with([&](auto& list) {
        for (auto& storage_device : list) {
            if (!filter_device_callback(storage_device))
                continue;
            auto storage_device_lun = storage_device.logical_unit_number_address();
            if (storage_device.parent_controller_hardware_relative_id() == address_parameters[0]
                && storage_device_lun.target_id == address_parameters[1]
                && storage_device_lun.disk_id == address_parameters[2]) {
                m_boot_block_device = storage_device;
                chosen_storage_device = storage_device;
                break;
            }
        }
    });

    if (chosen_storage_device)
        resolve_partition_from_boot_device_parameter(*chosen_storage_device, relative_hardware_prefix);
}

UNMAP_AFTER_INIT void StorageManagement::determine_ata_boot_device()
{
    determine_hardware_relative_boot_device(ata_device_prefix, [](StorageDevice const& device) -> bool {
        return device.command_set() == StorageDevice::CommandSet::ATA;
    });
}

UNMAP_AFTER_INIT void StorageManagement::determine_nvme_boot_device()
{
    determine_hardware_relative_boot_device(nvme_device_prefix, [](StorageDevice const& device) -> bool {
        return device.command_set() == StorageDevice::CommandSet::NVMe;
    });
}

UNMAP_AFTER_INIT void StorageManagement::determine_sd_boot_device()
{
    determine_hardware_relative_boot_device(sd_device_prefix, [](StorageDevice const& device) -> bool {
        return device.command_set() == StorageDevice::CommandSet::SD;
    });
}

UNMAP_AFTER_INIT void StorageManagement::determine_block_boot_device()
{
    VERIFY(m_boot_argument.starts_with(block_device_prefix));
    auto parameters_view = extract_boot_device_address_parameters(block_device_prefix);

    // Note: We simply fetch the corresponding BlockDevice with the major and minor parameters.
    // We don't try to accept and resolve a partition number as it will make this code much more
    // complicated. This rule is also explained in the boot_device_addressing(7) manual page.
    auto device = DeviceManagement::the().get_device(parameters_view[0], parameters_view[1]);
    if (device && device->is_block_device())
        m_boot_block_device = *static_ptr_cast<BlockDevice>(device);
}

void StorageManagement::attach_device(StorageDevice& device)
{
    m_storage_devices.with([&](auto& list) {
        list.append(device);
    });
}

void StorageManagement::detach_device(StorageDevice& device)
{
    m_storage_devices.with([&](auto& list) {
        list.remove(device);
    });
}

UNMAP_AFTER_INIT void StorageManagement::determine_boot_device_with_logical_unit_number()
{
    VERIFY(m_boot_argument.starts_with(logical_unit_number_device_prefix));
    auto address_parameters = extract_boot_device_address_parameters(logical_unit_number_device_prefix);

    RefPtr<StorageDevice> chosen_storage_device;

    m_storage_devices.with([&](auto& list) {
        for (auto& storage_device : list) {
            auto storage_device_lun = storage_device.logical_unit_number_address();
            if (storage_device_lun.controller_id == address_parameters[0]
                && storage_device_lun.target_id == address_parameters[1]
                && storage_device_lun.disk_id == address_parameters[2]) {
                m_boot_block_device = storage_device;
                chosen_storage_device = storage_device;
                break;
            }
        }
    });

    if (chosen_storage_device)
        resolve_partition_from_boot_device_parameter(*chosen_storage_device, logical_unit_number_device_prefix);
}

UNMAP_AFTER_INIT bool StorageManagement::determine_boot_device(StringView boot_argument)
{
    m_boot_argument = boot_argument;

    if (m_boot_argument.starts_with(block_device_prefix)) {
        determine_block_boot_device();
        return m_boot_block_device;
    }

    if (m_boot_argument.starts_with(partition_uuid_prefix)) {
        determine_boot_device_with_partition_uuid();
        return m_boot_block_device;
    }

    if (m_boot_argument.starts_with(logical_unit_number_device_prefix)) {
        determine_boot_device_with_logical_unit_number();
        return m_boot_block_device;
    }

    if (m_boot_argument.starts_with(ata_device_prefix)) {
        determine_ata_boot_device();
        return m_boot_block_device;
    }

    if (m_boot_argument.starts_with(nvme_device_prefix)) {
        determine_nvme_boot_device();
        return m_boot_block_device;
    }

    if (m_boot_argument.starts_with(sd_device_prefix)) {
        determine_sd_boot_device();
        return m_boot_block_device;
    }
    PANIC("StorageManagement: Invalid root boot parameter.");
}

UNMAP_AFTER_INIT void StorageManagement::determine_boot_device_with_partition_uuid()
{
    VERIFY(m_boot_argument.starts_with(partition_uuid_prefix));

    auto partition_uuid = UUID(m_boot_argument.substring_view(partition_uuid_prefix.length()), UUID::Endianness::Mixed);

    m_storage_devices.with([&](auto& list) {
        for (auto& storage_device : list) {
            storage_device.partitions().with([&](auto& partitions_list) {
                for (auto& partition : partitions_list) {
                    if (partition.metadata().unique_guid().is_zero())
                        continue;
                    if (partition.metadata().unique_guid() == partition_uuid) {
                        m_boot_block_device = partition;
                        break;
                    }
                }
            });
        }
    });
}

LockRefPtr<BlockDevice> StorageManagement::boot_block_device() const
{
    return m_boot_block_device.strong_ref();
}

MajorNumber StorageManagement::storage_type_major_number()
{
    return 3;
}
MinorNumber StorageManagement::generate_storage_minor_number()
{
    return s_storage_device_minor_number.fetch_add(1);
}

MinorNumber StorageManagement::generate_partition_minor_number()
{
    return s_partition_device_minor_number.fetch_add(1);
}

u32 StorageManagement::generate_controller_id()
{
    return s_controller_id.fetch_add(1);
}

NonnullRefPtr<FileSystem> StorageManagement::root_filesystem() const
{
    auto boot_device_description = boot_block_device();
    if (!boot_device_description) {
        dump_storage_devices_and_partitions();
        PANIC("StorageManagement: Couldn't find a suitable device to boot from");
    }
    auto description_or_error = OpenFileDescription::try_create(boot_device_description.release_nonnull());
    VERIFY(!description_or_error.is_error());

    Array<u8, PAGE_SIZE> mount_specific_data;
    mount_specific_data.fill(0);
    auto file_system = Ext2FS::try_create(description_or_error.release_value(), mount_specific_data.span()).release_value();

    if (auto result = file_system->initialize(); result.is_error()) {
        dump_storage_devices_and_partitions();
        PANIC("StorageManagement: Couldn't open root filesystem: {}", result.error());
    }
    return file_system;
}

UNMAP_AFTER_INIT void StorageManagement::initialize(bool, bool)
{
    VERIFY(s_storage_device_minor_number == 0);
    if (PCI::Access::is_disabled()) {
#if ARCH(X86_64)
        // Note: If PCI is disabled, we assume that at least we have an ISA IDE controller
        // to probe and use
        auto isa_ide_controller = MUST(ISAIDEController::initialize());
        m_controllers.append(isa_ide_controller);
#endif
    }

#if ARCH(AARCH64)
    auto& rpi_sdhc = RPi::SDHostController::the();
    if (auto maybe_error = rpi_sdhc.initialize(); maybe_error.is_error()) {
        dmesgln("Unable to initialize RaspberryPi's SD Host Controller: {}", maybe_error.error());
    } else {
        m_controllers.append(rpi_sdhc);
    }
#endif
}

StorageManagement& StorageManagement::the()
{
    return *s_the;
}

}
