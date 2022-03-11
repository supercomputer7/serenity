/*
 * Copyright (c) 2021, Sahan Fernando <sahan.h.fernando@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/DistinctNumeric.h>
#include <Kernel/Devices/CharacterDevice.h>
#include <Kernel/Graphics/VirtIOGPU/FramebufferDevice.h>
#include <Kernel/Graphics/VirtIOGPU/Protocol.h>

namespace Kernel::Graphics::VirtIOGPU {

enum class VirGLCommand : u32 {
    NOP = 0,
    CREATE_OBJECT = 1,
    BIND_OBJECT,
    DESTROY_OBJECT,
    SET_VIEWPORT_STATE,
    SET_FRAMEBUFFER_STATE,
    SET_VERTEX_BUFFERS,
    CLEAR,
    DRAW_VBO,
    RESOURCE_INLINE_WRITE,
    SET_SAMPLER_VIEWS,
    SET_INDEX_BUFFER,
    SET_CONSTANT_BUFFER,
    SET_STENCIL_REF,
    SET_BLEND_COLOR,
    SET_SCISSOR_STATE,
    BLIT,
    RESOURCE_COPY_REGION,
    BIND_SAMPLER_STATES,
    BEGIN_QUERY,
    END_QUERY,
    GET_QUERY_RESULT,
    SET_POLYGON_STIPPLE,
    SET_CLIP_STATE,
    SET_SAMPLE_MASK,
    SET_STREAMOUT_TARGETS,
    SET_RENDER_CONDITION,
    SET_UNIFORM_BUFFER,

    SET_SUB_CTX,
    CREATE_SUB_CTX,
    DESTROY_SUB_CTX,
    BIND_SHADER,
    SET_TESS_STATE,
    SET_MIN_SAMPLES,
    SET_SHADER_BUFFERS,
    SET_SHADER_IMAGES,
    MEMORY_BARRIER,
    LAUNCH_GRID,
    SET_FRAMEBUFFER_STATE_NO_ATTACH,
    TEXTURE_BARRIER,
    SET_ATOMIC_BUFFERS,
    SET_DBG_FLAGS,
    GET_QUERY_RESULT_QBO,
    TRANSFER3D,
    END_TRANSFERS,
    COPY_TRANSFER3D,
    SET_TWEAKS,
    CLEAR_TEXTURE,
    PIPE_RESOURCE_CREATE,
    PIPE_RESOURCE_SET_TYPE,
    GET_MEMORY_INFO,
    SEND_STRING_MARKER,
    MAX_COMMANDS
};

union ClearType {
    struct {
        u32 depth : 1;
        u32 stencil : 1;
        u32 color0 : 1;
        u32 color1 : 1;
        u32 color2 : 1;
        u32 color3 : 1;
        u32 color4 : 1;
        u32 color5 : 1;
        u32 color6 : 1;
        u32 color7 : 1;
    } flags;
    u32 value;
};

class VirtIODisplayConnector : public DisplayConnector {
public:
    static NonnullRefPtr<VirtIODisplayConnector> must_create(VirtIOGPU::GraphicsAdapter& graphics_adapter);

private:
    struct Scanout {
        RefPtr<Graphics::VirtIOGPU::FramebufferDevice> framebuffer;
        RefPtr<Console> console;
        Protocol::DisplayInfoResponse::Display display_info {};
        Optional<EDID::Parser> edid;
    };

    u32 get_pending_events();
    void clear_pending_events(u32 event_bitmask);

    // 3D Command stuff
    ContextID create_context();
    void attach_resource_to_context(ResourceID resource_id, ContextID context_id);
    void submit_command_buffer(ContextID, Function<size_t(Bytes)> buffer_writer);
    Protocol::TextureFormat get_framebuffer_format() const { return Protocol::TextureFormat::VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM; }

    auto& operation_lock() { return m_operation_lock; }
    ResourceID allocate_resource_id();
    ContextID allocate_context_id();

    PhysicalAddress start_of_scratch_space() const { return m_scratch_space->physical_page(0)->paddr(); }
    AK::BinaryBufferWriter create_scratchspace_writer()
    {
        return { Bytes(m_scratch_space->vaddr().as_ptr(), m_scratch_space->size()) };
    }
    void synchronous_virtio_gpu_command(PhysicalAddress buffer_start, size_t request_size, size_t response_size);
    void populate_virtio_gpu_request_header(Protocol::ControlHeader& header, Protocol::CommandType ctrl_type, u32 flags = 0);

    void query_display_information();
    void query_display_edid(Optional<ScanoutID>);
    ResourceID create_2d_resource(Protocol::Rect rect);
    ResourceID create_3d_resource(Protocol::Resource3DSpecification const& resource_3d_specification);
    void delete_resource(ResourceID resource_id);
    void ensure_backing_storage(ResourceID resource_id, Memory::Region const& region, size_t buffer_offset, size_t buffer_length);
    void detach_backing_storage(ResourceID resource_id);
    void set_scanout_resource(ScanoutID scanout, ResourceID resource_id, Protocol::Rect rect);
    void transfer_framebuffer_data_to_host(ScanoutID scanout, ResourceID resource_id, Protocol::Rect const& rect);
    void flush_displayed_image(ResourceID resource_id, Protocol::Rect const& dirty_rect);
    ErrorOr<Optional<EDID::Parser>> query_edid(u32 scanout_id);

    Optional<ScanoutID> m_default_scanout;
    size_t m_num_scanouts { 0 };
    Scanout m_scanouts[VIRTIO_GPU_MAX_SCANOUTS];

    VirtIO::Configuration const* m_device_configuration { nullptr };
    ResourceID m_resource_id_counter { 0 };
    ContextID m_context_id_counter { 0 };
    RefPtr<GPU3DDevice> m_3d_device;
    bool m_has_virgl_support { false };

    void flush_dirty_rectangle(ScanoutID, ResourceID, Protocol::Rect const& dirty_rect);

    template<typename F>
    IterationDecision for_each_framebuffer(F f)
    {
        for (auto& scanout : m_scanouts) {
            if (!scanout.framebuffer)
                continue;
            IterationDecision decision = f(*scanout.framebuffer, *scanout.console);
            if (decision != IterationDecision::Continue)
                return decision;
        }
        return IterationDecision::Continue;
    }

    RefPtr<Console> default_console()
    {
        if (m_default_scanout.has_value())
            return m_scanouts[m_default_scanout.value().value()].console;
        return {};
    }
    auto& display_info(ScanoutID scanout) const
    {
        VERIFY(scanout.value() < VIRTIO_GPU_MAX_SCANOUTS);
        return m_scanouts[scanout.value()].display_info;
    }
    auto& display_info(ScanoutID scanout)
    {
        VERIFY(scanout.value() < VIRTIO_GPU_MAX_SCANOUTS);
        return m_scanouts[scanout.value()].display_info;
    }

    explicit VirtIODisplayConnector(VirtIOGPU::GraphicsAdapter& graphics_adapter);
    
    NonnullRefPtr<VirtIOGPU::GraphicsAdapter> m_graphics_adapter;
    // Context used for kernel operations (e.g. flushing resources to scanout)
    ContextID m_kernel_context_id;
    // Memory management for backing buffers
    NonnullOwnPtr<Memory::Region> m_virgl3d_uploader_buffer_region;
    constexpr static size_t NUM_TRANSFER_REGION_PAGES = 256;
};

}
