/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Platform.h>
#include <stddef.h>
#include <sys/cdefs.h>
#include <sys/ioctl.h>

__BEGIN_DECLS

ALWAYS_INLINE int fb_get_properties(int fd, FBProperties* info)
{
    return ioctl(fd, FB_IOCTL_GET_PROPERTIES, info);
}

ALWAYS_INLINE int fb_get_head_properties(int fd, FBHeadProperties* info)
{
    return ioctl(fd, FB_IOCTL_GET_HEAD_PROPERTIES, info);
}

ALWAYS_INLINE int fb_get_mode_setting(int fd, FBHeadModeSetting* info)
{
    FBHeadProperties head_properties;
    if (auto rc = ioctl(fd, FB_IOCTL_GET_HEAD_PROPERTIES, &head_properties); rc < 0)
        return rc;
    info->horizontal_stride = head_properties.mode_setting.horizontal_stride;
    info->pixel_clock_in_khz = head_properties.mode_setting.pixel_clock_in_khz;
    info->horizontal_active = head_properties.mode_setting.horizontal_active;
    info->horizontal_front_porch_pixels = head_properties.mode_setting.horizontal_front_porch_pixels;
    info->horizontal_sync_time_pixels = head_properties.mode_setting.horizontal_sync_time_pixels;
    info->horizontal_blank_pixels = head_properties.mode_setting.horizontal_blank_pixels;
    info->vertical_active = head_properties.mode_setting.vertical_active;
    info->vertical_front_porch_lines = head_properties.mode_setting.vertical_front_porch_lines;
    info->vertical_sync_time_lines = head_properties.mode_setting.vertical_sync_time_lines;
    info->vertical_blank_lines = head_properties.mode_setting.vertical_blank_lines;
    return 0;
}

ALWAYS_INLINE int fb_set_resolution(int fd, FBHeadModeSetting* info)
{
    return ioctl(fd, FB_IOCTL_SET_HEAD_MODE_SETTING, info);
}

ALWAYS_INLINE int fb_get_head_edid(int fd, FBHeadEDID* info)
{
    return ioctl(fd, FB_IOCTL_GET_HEAD_EDID, info);
}

ALWAYS_INLINE int fb_get_head_vertical_offset_buffer(int fd, FBHeadVerticalOffset* vertical_offset)
{
    return ioctl(fd, FB_IOCTL_GET_HEAD_VERTICAL_OFFSET_BUFFER, vertical_offset);
}

ALWAYS_INLINE int fb_set_head_vertical_offset_buffer(int fd, FBHeadVerticalOffset* vertical_offset)
{
    return ioctl(fd, FB_IOCTL_SET_HEAD_VERTICAL_OFFSET_BUFFER, vertical_offset);
}

ALWAYS_INLINE int fb_flush_everything(int fd)
{
    return ioctl(fd, FB_IOCTL_FLUSH_HEAD, 0);
}

ALWAYS_INLINE int fb_flush_buffers(int fd, int index, FBRect const* rects, unsigned count)
{
    FBFlushRects fb_flush_rects;
    fb_flush_rects.buffer_index = index;
    fb_flush_rects.count = count;
    fb_flush_rects.rects = rects;
    return ioctl(fd, FB_IOCTL_FLUSH_HEAD_BUFFERS, &fb_flush_rects);
}

__END_DECLS
