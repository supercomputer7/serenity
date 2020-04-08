#pragma once

#include <AK/NonnullRefPtr.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <AK/Types.h>
#include <Kernel/FileSystem/FileSystem.h>
#include <Kernel/FileSystem/Inode.h>
#include <Kernel/FileSystem/InodeMetadata.h>
#include <Kernel/KResult.h>

namespace Kernel {

class Device;
class DevTmpFSInode;
class DevTmpFSRootDirectory;
class DevTmpFSDirectory;
class DevTmpFSDeviceInode;

class DevTmpFS final : public FS {
    friend class DevTmpFSDirectory;

public:
    virtual ~DevTmpFS() override;
    static NonnullRefPtr<DevTmpFS> create();
    static DevTmpFS& the();

    virtual bool initialize() override;
    virtual const char* class_name() const override { return "DevTmpFS"; }

    virtual InodeIdentifier root_inode() const override;
    virtual KResultOr<NonnullRefPtr<Inode>> create_inode(InodeIdentifier parent_id, const String& name, mode_t, off_t size, dev_t, uid_t, gid_t) override;
    virtual KResult create_directory(InodeIdentifier parent_inode, const String& name, mode_t, uid_t, gid_t) override;
    virtual RefPtr<Inode> get_inode(InodeIdentifier) const override;

    void register_persistent_device(Device&);
    void unregister_persistent_device(Device&);

    virtual bool supports_watchers() const override { return true; }

private:
    RefPtr<DevTmpFSInode> try_to_find_listed_inode(InodeIdentifier) const;

    unsigned find_a_free_inode();

    DevTmpFS();
    HashTable<RefPtr<DevTmpFSInode>> m_inodes;
    NonnullRefPtr<DevTmpFSRootDirectory> m_root_inode;
    unsigned m_count { 2 };
};

class DevTmpFSInode : public Inode {
public:
    virtual StringView name() const = 0;
    virtual ~DevTmpFSInode() override;

    virtual bool is_persistent() const { return false; }

protected:
    DevTmpFSInode(DevTmpFS&, unsigned index);
};

class DevTmpFSDirectory : public DevTmpFSInode {
    friend class DevTmpFS;

public:
    static NonnullRefPtr<DevTmpFSDirectory> create(DevTmpFS&, unsigned index, String name);
    virtual ~DevTmpFSDirectory() override;

protected:
    HashTable<RefPtr<DevTmpFSInode>> m_inodes;
    DevTmpFSDirectory(DevTmpFS&, unsigned index);
    DevTmpFSDirectory(DevTmpFS&, unsigned index, String name);
    virtual StringView name() const override { return m_name; }
    RefPtr<DevTmpFSInode> lookup_listed_nodes(StringView name);

private:
    // ^Inode
    virtual ssize_t read_bytes(off_t, ssize_t, u8* buffer, FileDescription*) const override;
    virtual InodeMetadata metadata() const override;
    virtual bool traverse_as_directory(Function<bool(const FS::DirectoryEntry&)>) const override;
    virtual RefPtr<Inode> lookup(StringView name) override;
    virtual void flush_metadata() override;
    virtual ssize_t write_bytes(off_t, ssize_t, const u8* buffer, FileDescription*) override;
    virtual KResult add_child(InodeIdentifier child_id, const StringView& name, mode_t) override;
    virtual KResult remove_child(const StringView& name) override;
    virtual size_t directory_entry_count() const override;
    virtual KResult chmod(mode_t) override;
    virtual KResult chown(uid_t, gid_t) override;
    String m_name;
};

class DevTmpFSRootDirectory final : public DevTmpFSDirectory {
    friend class DevTmpFS;

public:
    virtual ~DevTmpFSRootDirectory() override;
    virtual StringView name() const override { return "."; }
    virtual bool is_persistent() const override { return true; }

private:
    DevTmpFSRootDirectory(DevTmpFS&, unsigned index);

    // ^Inode
    virtual InodeMetadata metadata() const override;
};

class DevTmpFSDeviceInode final : public DevTmpFSInode {
    friend class DevTmpFS;

public:
    static NonnullRefPtr<DevTmpFSDeviceInode> create(DevTmpFS&, unsigned index, Device& device, String name, bool is_persistent = false);

    virtual ~DevTmpFSDeviceInode() override;
    NonnullRefPtr<Device> device() const;
    u32 encoded_signature() const;
    StringView name() const;
    virtual bool is_persistent() const override { return m_persistent; }

protected:
private:
    DevTmpFSDeviceInode(DevTmpFS&, unsigned index, Device& device, String name, bool is_persistent);

    // ^Inode
    virtual ssize_t read_bytes(off_t, ssize_t, u8* buffer, FileDescription*) const override;
    virtual InodeMetadata metadata() const override;
    virtual bool traverse_as_directory(Function<bool(const FS::DirectoryEntry&)>) const override;
    virtual RefPtr<Inode> lookup(StringView name) override;
    virtual void flush_metadata() override;
    virtual ssize_t write_bytes(off_t, ssize_t, const u8* buffer, FileDescription*) override;
    virtual KResult add_child(InodeIdentifier child_id, const StringView& name, mode_t) override;
    virtual KResult remove_child(const StringView& name) override;
    virtual size_t directory_entry_count() const override;
    virtual KResult chmod(mode_t) override;
    virtual KResult chown(uid_t, gid_t) override;

    NonnullRefPtr<Device> m_device;
    String m_name;
    bool m_persistent;
    u32 m_encoded_signature;
    InodeMetadata m_metadata;
};

}
