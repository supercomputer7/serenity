/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Graphics/DisplayConnector.h>
#include <Kernel/Graphics/GraphicsManagement.h>

namespace Kernel {

DisplayConnector::DisplayConnector()
    : CharacterDevice(28, GraphicsManagement::the().allocate_minor_device_number())
{
}

ErrorOr<Memory::Region*> DisplayConnector::mmap(Process&, OpenFileDescription&, Memory::VirtualRange const&, u64, int, bool)
{
    return Error::from_errno(ENOTSUP);
}

ErrorOr<size_t> DisplayConnector::read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t)
{
    return Error::from_errno(ENOTIMPL);
}
ErrorOr<size_t> DisplayConnector::write(OpenFileDescription&, u64 offset, const UserOrKernelBuffer& framebuffer_data, size_t length)
{
    MutexLocker locker(m_control_lock);
    if (console_mode()) {
        return Error::from_errno(EBUSY);
    }
    return write_to_first_surface(offset, framebuffer_data, length);
}

void DisplayConnector::will_be_destroyed()
{
    GraphicsManagement::the().detach_display_connector({}, *this);
    Device::will_be_destroyed();
}

void DisplayConnector::after_inserting()
{
    Device::after_inserting();
    GraphicsManagement::the().attach_new_display_connector({}, *this);
}

bool DisplayConnector::console_mode() const
{
    VERIFY(m_control_lock.is_locked());
    return m_console_mode;
}

void DisplayConnector::set_display_mode(Badge<GraphicsManagement>, DisplayMode mode)
{
    MutexLocker locker(m_control_lock);
    m_console_mode = mode == DisplayMode::Console ? true : false;
}

ErrorOr<void> DisplayConnector::ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg)
{
    if (request != FB_IOCTL_GET_HEAD_EDID) {
        // Allow anyone to query the EDID. Eventually we'll publish the current EDID on /sys
        // so it doesn't really make sense to require the video pledge to query it.
        TRY(Process::current().require_promise(Pledge::video));
    }

    // TODO: We really should have ioctls for destroying resources as well
    switch (request) {
    // Regular "framebuffer" commands
    case FB_IOCTL_GET_PROPERTIES: {
        auto user_properties = static_ptr_cast<FBProperties*>(arg);
        FBProperties properties {};
        properties.flushing_support = flush_support();
        properties.doublebuffer_support = double_framebuffering_capable();
        //properties.partial_flushing_support = partial_flushing_support();
        properties.partial_flushing_support = false;
        properties.refresh_rate_support = refresh_rate_support();
        
        return copy_to_user(user_properties, &properties);
    }
    case FB_IOCTL_GET_HEAD_PROPERTIES: {
        auto user_head_properties = static_ptr_cast<FBHeadProperties*>(arg);
        FBHeadProperties head_properties {};
        TRY(copy_from_user(&head_properties, user_head_properties));

        Resolution current_resolution = TRY(get_resolution());
        head_properties.refresh_rate = 0;
        head_properties.pitch = current_resolution.pitch;
        head_properties.width = current_resolution.width;
        head_properties.height = current_resolution.height;
        head_properties.buffer_length = current_resolution.height * current_resolution.pitch;
        head_properties.offset = 0;
        return copy_to_user(user_head_properties, &head_properties);
    }
    case FB_IOCTL_GET_HEAD_EDID: {
        auto user_head_edid = static_ptr_cast<FBHeadEDID*>(arg);
        FBHeadEDID head_edid {};
        TRY(copy_from_user(&head_edid, user_head_edid));

        auto edid_bytes = TRY(get_edid());
        if (head_edid.bytes != nullptr) {
            // Only return the EDID if a buffer was provided. Either way,
            // we'll write back the bytes_size with the actual size
            if (head_edid.bytes_size < edid_bytes.size()) {
                head_edid.bytes_size = edid_bytes.size();
                TRY(copy_to_user(user_head_edid, &head_edid));
                return Error::from_errno(EOVERFLOW);
            }
            TRY(copy_to_user(head_edid.bytes, (void const*)edid_bytes.data(), edid_bytes.size()));
        }
        head_edid.bytes_size = edid_bytes.size();
        return copy_to_user(user_head_edid, &head_edid);
    }
    case FB_IOCTL_SET_HEAD_RESOLUTION: {
        auto user_resolution = static_ptr_cast<FBHeadResolution const*>(arg);
        auto head_resolution = TRY(copy_typed_from_user(user_resolution));

        if (head_resolution.refresh_rate < 0)
            return Error::from_errno(EINVAL);
        if (head_resolution.width < 0)
            return Error::from_errno(EINVAL);
        if (head_resolution.height < 0)
            return Error::from_errno(EINVAL);
        Resolution requested_resolution;
        requested_resolution.bpp = 32;
        if (refresh_rate_support())
            requested_resolution.refresh_rate = head_resolution.refresh_rate;
        requested_resolution.width = head_resolution.width;
        requested_resolution.height = head_resolution.height;
        
        TRY(set_resolution(requested_resolution));
        return {};
    }
    /*
    case FB_IOCTL_SET_HEAD_VERTICAL_OFFSET_BUFFER: {
        auto user_head_vertical_buffer_offset = static_ptr_cast<FBHeadVerticalOffset const*>(arg);
        auto head_vertical_buffer_offset = TRY(copy_typed_from_user(user_head_vertical_buffer_offset));

        if (head_vertical_buffer_offset.offsetted < 0 || head_vertical_buffer_offset.offsetted > 1)
            return Error::from_errno(EINVAL);
        TRY(set_head_buffer(head_vertical_buffer_offset.head_index, head_vertical_buffer_offset.offsetted));
        return {};
    }
    case FB_IOCTL_GET_HEAD_VERTICAL_OFFSET_BUFFER: {
        auto user_head_vertical_buffer_offset = static_ptr_cast<FBHeadVerticalOffset*>(arg);
        FBHeadVerticalOffset head_vertical_buffer_offset {};
        TRY(copy_from_user(&head_vertical_buffer_offset, user_head_vertical_buffer_offset));

        head_vertical_buffer_offset.offsetted = TRY(vertical_offsetted(head_vertical_buffer_offset.head_index));
        return copy_to_user(user_head_vertical_buffer_offset, &head_vertical_buffer_offset);
    }
    case FB_IOCTL_FLUSH_HEAD_BUFFERS: {
        if (!partial_flushing_support())
            return Error::from_errno(ENOTSUP);
        auto user_flush_rects = static_ptr_cast<FBFlushRects const*>(arg);
        auto flush_rects = TRY(copy_typed_from_user(user_flush_rects));
        if (Checked<unsigned>::multiplication_would_overflow(flush_rects.count, sizeof(FBRect)))
            return Error::from_errno(EFAULT);
        SpinlockLocker locker(m_flushing_lock);
        if (flush_rects.count > 0) {
            for (unsigned i = 0; i < flush_rects.count; i++) {
                FBRect user_dirty_rect;
                TRY(copy_from_user(&user_dirty_rect, &flush_rects.rects[i]));
                TRY(flush_rectangle(flush_rects.buffer_index, user_dirty_rect));
            }
        }
        return {};
    };
    
    case FB_IOCTL_FLUSH_HEAD: {
        if (!flush_support())
            return Error::from_errno(ENOTSUP);
        // Note: We accept a FBRect, but we only really care about the head_index value.
        auto user_rect = static_ptr_cast<FBRect const*>(arg);
        auto rect = TRY(copy_typed_from_user(user_rect));
        TRY(flush_head_buffer());
        return {};
    }
    */
    /*// VirtIO 3D acceleration commands
    case VIRGL_IOCTL_TRANSFER_DATA: {
        auto user_transfer_descriptor = static_ptr_cast<VirGLTransferDescriptor const*>(arg);
        auto transfer_descriptor = TRY(copy_typed_from_user(user_transfer_descriptor));
        if (transfer_descriptor.direction == VIRGL_DATA_DIR_GUEST_TO_HOST) {
            if (transfer_descriptor.offset_in_region + transfer_descriptor.num_bytes > NUM_TRANSFER_REGION_PAGES * PAGE_SIZE) {
                return EOVERFLOW;
            }
            auto target = m_transfer_buffer_region->vaddr().offset(transfer_descriptor.offset_in_region).as_ptr();
            return copy_from_user(target, transfer_descriptor.data, transfer_descriptor.num_bytes);
        } else if (transfer_descriptor.direction == VIRGL_DATA_DIR_HOST_TO_GUEST) {
            if (transfer_descriptor.offset_in_region + transfer_descriptor.num_bytes > NUM_TRANSFER_REGION_PAGES * PAGE_SIZE) {
                return EOVERFLOW;
            }
            auto source = m_transfer_buffer_region->vaddr().offset(transfer_descriptor.offset_in_region).as_ptr();
            return copy_to_user(transfer_descriptor.data, source, transfer_descriptor.num_bytes);
        } else {
            return EINVAL;
        }
    }
    case VIRGL_IOCTL_SUBMIT_CMD: {
        MutexLocker locker(m_graphics_adapter.operation_lock());
        auto user_command_buffer = static_ptr_cast<VirGLCommandBuffer const*>(arg);
        auto command_buffer = TRY(copy_typed_from_user(user_command_buffer));
        m_graphics_adapter.submit_command_buffer(m_kernel_context_id, [&](Bytes buffer) {
            auto num_bytes = command_buffer.num_elems * sizeof(u32);
            VERIFY(num_bytes <= buffer.size());
            MUST(copy_from_user(buffer.data(), command_buffer.data, num_bytes));
            return num_bytes;
        });
        return {};
    }
    case VIRGL_IOCTL_CREATE_RESOURCE: {
        auto user_spec = static_ptr_cast<VirGL3DResourceSpec const*>(arg);
        VirGL3DResourceSpec spec = TRY(copy_typed_from_user(user_spec));

        Protocol::Resource3DSpecification const resource_spec = {
            .target = static_cast<Protocol::Gallium::PipeTextureTarget>(spec.target),
            .format = spec.format,
            .bind = spec.bind,
            .width = spec.width,
            .height = spec.height,
            .depth = spec.depth,
            .array_size = spec.array_size,
            .last_level = spec.last_level,
            .nr_samples = spec.nr_samples,
            .flags = spec.flags
        };
        MutexLocker locker(m_graphics_adapter.operation_lock());
        auto resource_id = m_graphics_adapter.create_3d_resource(resource_spec).value();
        m_graphics_adapter.attach_resource_to_context(resource_id, m_kernel_context_id);
        m_graphics_adapter.ensure_backing_storage(resource_id, *m_transfer_buffer_region, 0, NUM_TRANSFER_REGION_PAGES * PAGE_SIZE);
        spec.created_resource_id = resource_id;
        // FIXME: We should delete the resource we just created if we fail to copy the resource id out
        return copy_to_user(static_ptr_cast<VirGL3DResourceSpec*>(arg), &spec);
    }
    */
    }
    return EINVAL;
}

}
