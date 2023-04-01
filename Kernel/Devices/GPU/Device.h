/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/API/Ioctl.h>
#include <Kernel/Devices/CharacterDevice.h>
#include <Kernel/Memory/SharedFramebufferVMObject.h>
#include <LibEDID/EDID.h>

namespace Kernel {

class GPUManagement;
class GPUDevice : public CharacterDevice {
    friend class GPUManagement;
    friend class DeviceManagement;

public:
    virtual ~GPUDevice() = default;

protected:
    GPUDevice();

    virtual GPUDeviceProperties device_properties() const = 0;
    virtual ErrorOr<void> change_connector_modesetting(GPUConnectorModeSetting) = 0;
    virtual ErrorOr<GPUConnectorModeSetting> current_connector_modesetting(size_t index, DisplayConnector::ModeSetting) = 0;

    mutable Spinlock<LockRank::None> m_control_lock {};
    mutable Spinlock<LockRank::None> m_modeset_lock {};

private:
    // ^File
    virtual bool is_seekable() const override { return false; }
    virtual bool can_read(OpenFileDescription const&, u64) const final override { return true; }
    virtual bool can_write(OpenFileDescription const&, u64) const final override { return true; }
    virtual ErrorOr<size_t> read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t) override final;
    virtual ErrorOr<size_t> write(OpenFileDescription&, u64, UserOrKernelBuffer const&, size_t) override final;
    virtual ErrorOr<NonnullLockRefPtr<Memory::VMObject>> vmobject_for_mmap(Process&, Memory::VirtualRange const&, u64&, bool) = 0;
    virtual ErrorOr<void> ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg) override final;
    virtual StringView class_name() const override final { return "GPUDevice"sv; }

    GPUDevice& operator=(GPUDevice const&) = delete;
    GPUDevice& operator=(GPUDevice&&) = delete;
    GPUDevice(GPUDevice&&) = delete;

    virtual void will_be_destroyed() override;
    virtual ErrorOr<void> after_inserting() override;

    ErrorOr<bool> ioctl_requires_ownership(unsigned request) const;

private:
    LockWeakPtr<Process> m_responsible_process;
    Spinlock<LockRank::None> m_responsible_process_lock {};

    IntrusiveListNode<GPUDevice, LockRefPtr<GPUDevice>> m_list_node;

    SpinlockProtected<Vector<DisplayConnector>, LockRank::None> m_display_connectors;
};
}
