#include <AK/StringBuilder.h>
#include <Kernel/Devices/Device.h>
#include <Kernel/Devices/DeviceRegistrar.h>
#include <Kernel/FileSystem/DevTmpFS.h>
#include <Kernel/FileSystem/InodeMetadata.h>
#include <Kernel/FileSystem/VirtualFileSystem.h>
#include <Kernel/Time/TimeManagement.h>

namespace Kernel {

static DevTmpFS* s_devtmpfs;

static String get_static_device_name(Device& device)
{
    if (!device.is_block_device()) {
        dbg() << "DevTmpFS: Assigning name to Character device, major - " << device.major() << ", minor - " << device.minor();
        switch (device.major()) {
        case 1:
            if (device.minor() == 8)
                return String("random");
            else if (device.minor() == 3)
                return String("null");
            else if (device.minor() == 5)
                return String("zero");
            else if (device.minor() == 7)
                return String("full");
            else if (device.minor() == 18) {
                return String("debuglog");
            }
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    klog() << "DevTmpFS: Can't determine a name for device, major - " << device.major() << ", minor - " << device.minor();
    ASSERT_NOT_REACHED();
}

DevTmpFS& DevTmpFS::the()
{
    if (s_devtmpfs == nullptr) {
        s_devtmpfs = new DevTmpFS;
    }
    return *s_devtmpfs;
}

DevTmpFS::DevTmpFS()
    : m_root_inode(adopt(*new DevTmpFSRootDirectory(*this, 1)))
{
}

DevTmpFS::~DevTmpFS()
{
}
NonnullRefPtr<DevTmpFS> DevTmpFS::create()
{
    ASSERT(s_devtmpfs != nullptr);
    return adopt(*s_devtmpfs);
}

bool DevTmpFS::initialize()
{
    return true;
}

InodeIdentifier DevTmpFS::root_inode() const
{
    return { fsid(), 1 };
}

unsigned DevTmpFS::find_a_free_inode()
{
    LOCKER(m_lock);
    return m_count++;
}

KResultOr<NonnullRefPtr<Inode>> DevTmpFS::create_inode(InodeIdentifier parent_id, const String& name, mode_t mode, off_t, dev_t dev, uid_t, gid_t)
{
    LOCKER(m_lock);
    InodeMetadata metadata;
    metadata.mode = mode;
    if (!metadata.is_device())
        return KResult(-ENODEV);

    metadata.major_device = (dev & 0xfff00) >> 8;
    metadata.minor_device = (dev & 0xff) | ((dev >> 12) & 0xfff00);

    auto parent = get_inode(parent_id);
    if (parent.is_null())
        return KResult(-EFAULT);
    if (!parent->metadata().is_directory())
        return KResult(-ENOTDIR);

    auto inode_id = find_a_free_inode();
    auto inode = DevTmpFSDeviceInode::create(*this, inode_id, *DeviceRegistrar::the().get_device(metadata.major_device, metadata.minor_device), move(name));
    m_inodes.set(inode);

    parent->add_child({ fsid(), inode_id }, name, mode);
    return inode;
}

KResult DevTmpFS::create_directory(InodeIdentifier parent_id, const String& name, mode_t mode, uid_t, gid_t)
{
    LOCKER(m_lock);
    InodeMetadata metadata;
    metadata.mode = mode;

    // Fix up the mode to definitely be a directory.
    // FIXME: This is a bit on the hackish side.
    mode &= ~0170000;
    mode |= 0040000;

    auto parent = get_inode(parent_id);
    if (parent.is_null())
        return KResult(-EFAULT);
    if (!parent->metadata().is_directory())
        return KResult(-ENOTDIR);

    auto inode_id = find_a_free_inode();
    m_inodes.set(DevTmpFSDirectory::create(*this, inode_id, move(name)));
    parent->add_child({ fsid(), inode_id }, name, mode);

    return KSuccess;
}

RefPtr<Inode> DevTmpFS::get_inode(InodeIdentifier identifier) const
{
    return try_to_find_listed_inode(identifier);
}

RefPtr<DevTmpFSInode> DevTmpFS::try_to_find_listed_inode(InodeIdentifier identifier) const
{
    LOCKER(m_lock);
    if (identifier.index() == 1)
        return m_root_inode;

    for (auto& inode : m_inodes) {
        ASSERT(!inode.is_null());
        if (inode->index() == identifier.index())
            return inode;
    }

    ASSERT_NOT_REACHED();
}

void DevTmpFS::register_persistent_device(Device& device)
{
    m_root_inode->m_inodes.set(DevTmpFSDeviceInode::create(*this, m_count++, device, get_static_device_name(device), true));
    dbg() << "DevTmpFS: Registered persistent device: major - " << device.major() << ", minor - " << device.minor() << ", under the name " << get_static_device_name(device);
}

void DevTmpFS::unregister_persistent_device(Device&)
{
}

DevTmpFSInode::DevTmpFSInode(DevTmpFS& fs, unsigned index)
    : Inode(fs, index)
{
}

DevTmpFSInode::~DevTmpFSInode()
{
}

NonnullRefPtr<DevTmpFSDirectory> DevTmpFSDirectory::create(DevTmpFS& fs, unsigned index, String name)
{
    return adopt(*new DevTmpFSDirectory(fs, index, name));
}

DevTmpFSDirectory::DevTmpFSDirectory(DevTmpFS& fs, unsigned index)
    : DevTmpFSInode(fs, index)
{
}

DevTmpFSDirectory::DevTmpFSDirectory(DevTmpFS& fs, unsigned index, String name)
    : DevTmpFSInode(fs, index)
    , m_name(name)
{
}

DevTmpFSDirectory::~DevTmpFSDirectory()
{
}

ssize_t DevTmpFSDirectory::read_bytes(off_t, ssize_t, u8*, FileDescription*) const
{
    ASSERT_NOT_REACHED();
}

ssize_t DevTmpFSDirectory::write_bytes(off_t, ssize_t, const u8*, FileDescription*)
{
    ASSERT_NOT_REACHED();
}

bool DevTmpFSDirectory::traverse_as_directory(Function<bool(const FS::DirectoryEntry&)> callback) const
{
    dbg() << "DevTmpFS: traverse DevTmpFSRootDirectory " << index() << " as directory";
    if (identifier().index() > 1)
        return false;

    callback({ ".", 1, identifier(), 0 });
    callback({ "..", 2, identifier(), 0 });

    for (auto& inode : m_inodes) {
        ASSERT(!inode.is_null());
        callback({ (const_cast<DevTmpFSInode&>(*inode)).name().characters_without_null_termination(), (const_cast<DevTmpFSInode&>(*inode)).name().length(), inode->identifier(), 0 });
    }

    return true;
}

size_t DevTmpFSDirectory::directory_entry_count() const
{
    return m_inodes.size();
}

RefPtr<Inode> DevTmpFSDirectory::lookup(StringView name)
{
    return lookup_listed_nodes(name);
}

InodeMetadata DevTmpFSDirectory::metadata() const
{
    InodeMetadata metadata;
    metadata.inode = { fsid(), index() };
    metadata.mode = 0040555;
    metadata.uid = 0;
    metadata.gid = 0;
    metadata.size = 0;
    metadata.mtime = mepoch;
    return metadata;
}

void DevTmpFSDirectory::flush_metadata()
{
}

KResult DevTmpFSDirectory::add_child(InodeIdentifier inode, const StringView& name, mode_t)
{
    LOCKER(m_lock);

    auto lookup_result = lookup_listed_nodes(name);
    if (!lookup_result.is_null()) {
        dbg() << "DevTmpFSDirectory::add_child(): Name '" << name << "' already exists in directoyry inode " << index();
        return KResult(-EEXIST);
    }

    auto new_inode = DevTmpFS::the().try_to_find_listed_inode(inode);
    ASSERT(!new_inode.is_null());
    m_inodes.set(new_inode);
    return KSuccess;
}

KResult DevTmpFSDirectory::remove_child(const StringView& name)
{
    LOCKER(m_lock);

    auto lookup_result = lookup_listed_nodes(name);
    if (lookup_result.is_null()) {
        dbg() << "DevTmpFSDirectory::remove_child(): cannot find name '" << name << "' in directoyry inode " << index();
        return KResult(-ENOENT);
    }

    if (lookup_result->is_persistent()) {
        dbg() << "DevTmpFSDirectory::remove_child(): cannot remove persistent inode '" << name << "' in directoyry inode " << index();
        return KResult(-EPERM);
    }

    m_inodes.remove(lookup_result);
    return KSuccess;
}

RefPtr<DevTmpFSInode> DevTmpFSDirectory::lookup_listed_nodes(StringView name)
{
    LOCKER(m_lock);
    dbg() << "DevTmpFS: Directory lookup on StringView " << name;

    if (name == "." || name == "..")
        return this;

    for (auto& inode : m_inodes) {
        ASSERT(!inode.is_null());
        if (inode->name() == name)
            return inode;
    }

    return nullptr;
}

KResult DevTmpFSDirectory::chmod(mode_t)
{
    LOCKER(m_lock);
    return KResult(-EPERM);
}

KResult DevTmpFSDirectory::chown(uid_t, gid_t)
{
    return KResult(-EPERM);
}

DevTmpFSRootDirectory::DevTmpFSRootDirectory(DevTmpFS& fs, unsigned index)
    : DevTmpFSDirectory(fs, index)
{
}

DevTmpFSRootDirectory::~DevTmpFSRootDirectory()
{
}

InodeMetadata DevTmpFSRootDirectory::metadata() const
{
    InodeMetadata metadata;
    metadata.inode = { fsid(), 1 };
    metadata.mode = 0040555;
    metadata.uid = 0;
    metadata.gid = 0;
    metadata.size = 0;
    metadata.mtime = mepoch;
    return metadata;
}

NonnullRefPtr<DevTmpFSDeviceInode> DevTmpFSDeviceInode::create(DevTmpFS& fs, unsigned index, Device& device, String name, bool is_persistent)
{
    return adopt(*new DevTmpFSDeviceInode(fs, index, device, name, is_persistent));
}

DevTmpFSDeviceInode::DevTmpFSDeviceInode(DevTmpFS& fs, unsigned index, Device& device, String name, bool is_persistent)
    : DevTmpFSInode(fs, index)
    , m_device(device)
    , m_name(name)
    , m_persistent(is_persistent)
    , m_encoded_signature(encoded_device(device.major(), device.minor()))
{
}

DevTmpFSDeviceInode::~DevTmpFSDeviceInode()
{
}

ssize_t DevTmpFSDeviceInode::read_bytes(off_t, ssize_t size, u8* buffer, FileDescription* file_description) const
{
    ASSERT(file_description != nullptr);
    if (device()->can_read(*file_description)) {
        return device()->read(*file_description, buffer, size);
    }
    klog() << "DevTmpFS: Can't read from inode " << index();
    return 0;
}

ssize_t DevTmpFSDeviceInode::write_bytes(off_t, ssize_t size, const u8* buffer, FileDescription* file_description)
{
    ASSERT(file_description != nullptr);
    if (device()->can_write(*file_description)) {
        return device()->write(*file_description, buffer, size);
    }
    klog() << "DevTmpFS: Can't write to inode " << index();
    return 0;
}

InodeMetadata DevTmpFSDeviceInode::metadata() const
{
    InodeMetadata metadata;
    metadata.inode = { fsid(), index() };
    metadata.size = 0;
    metadata.uid = 0;
    metadata.gid = 0;

    metadata.mode = 0020644;
    if (device()->is_block_device())
        metadata.mode |= 0060000;
    else if (device()->is_character_device())
        metadata.mode |= 0020000;
    else
        ASSERT_NOT_REACHED();

    metadata.major_device = device()->major();
    metadata.minor_device = device()->minor();
    metadata.mtime = mepoch;
    return metadata;
}

bool DevTmpFSDeviceInode::traverse_as_directory(Function<bool(const FS::DirectoryEntry&)>) const
{
    klog() << "DevTmpFS: traverse DevTmpFSInode " << metadata().inode.index() << "as directory, failed!";
    return false;
}

size_t DevTmpFSDeviceInode::directory_entry_count() const
{
    ASSERT_NOT_REACHED();
}

RefPtr<Inode> DevTmpFSDeviceInode::lookup(StringView name)
{
    klog() << "DevTmpFS Inode " << index() << ": Inode lookup " << name << ", Abort!";
    return nullptr;
}

StringView DevTmpFSDeviceInode::name() const
{
    return m_name;
}

NonnullRefPtr<Device> DevTmpFSDeviceInode::device() const
{
    return m_device;
}

u32 DevTmpFSDeviceInode::encoded_signature() const
{
    return m_encoded_signature;
}

void DevTmpFSDeviceInode::flush_metadata()
{
}

KResult DevTmpFSDeviceInode::add_child(InodeIdentifier, const StringView&, mode_t)
{
    return KResult(-EROFS);
}

KResult DevTmpFSDeviceInode::remove_child(const StringView&)
{
    return KResult(-EROFS);
}

KResult DevTmpFSDeviceInode::chmod(mode_t)
{
    return KResult(-EPERM);
}

KResult DevTmpFSDeviceInode::chown(uid_t, gid_t)
{
    return KResult(-EPERM);
}
}
