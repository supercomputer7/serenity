/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Edwin Hoksberg <mail@edwinhoksberg.nl>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

struct GPUDeviceProperties {
    int connectors_count;
    int dma_capable;
};

struct GPUConnectorProperties {
    unsigned char doublebuffer_support;
    unsigned char flushing_support;
    unsigned char partial_flushing_support;
    unsigned char refresh_rate_support;
    unsigned max_buffer_bytes;
};

struct GPUConnectorModeSetting {
    int connector_index;
    int horizontal_stride;
    int pixel_clock_in_khz;
    int horizontal_active;
    int horizontal_front_porch_pixels;
    int horizontal_sync_time_pixels;
    int horizontal_blank_pixels;
    int vertical_active;
    int vertical_front_porch_lines;
    int vertical_sync_time_lines;
    int vertical_blank_lines;
    int horizontal_offset;
    int vertical_offset;
};

struct GPUHeadEDID {
    unsigned char* bytes;
    unsigned bytes_size;
};

struct GPUHeadVerticalOffset {
    int head_index;
    int offsetted;
};

struct FBRect {
    int head_index;
    unsigned x;
    unsigned y;
    unsigned width;
    unsigned height;
};

struct FBBufferOffset {
    int buffer_index;
    unsigned offset;
};

struct FBFlushRects {
    int buffer_index;
    unsigned count;
    struct FBRect const* rects;
};

enum ConsoleModes {
    KD_TEXT = 0x00,
    KD_GRAPHICS = 0x01,
};

#ifdef __cplusplus
}
#endif

enum IOCtlNumber {
    TIOCGPGRP,
    TIOCSPGRP,
    TCGETS,
    TCSETS,
    TCSETSW,
    TCSETSF,
    TCFLSH,
    TIOCGWINSZ,
    TIOCSCTTY,
    TIOCSTI,
    TIOCNOTTY,
    TIOCSWINSZ,
    TIOCGPTN,
    GPU_IOCTL_GET_PROPERTIES,
    GPU_IOCTL_SET_HEAD_VERTICAL_OFFSET_BUFFER,
    GPU_IOCTL_GET_HEAD_VERTICAL_OFFSET_BUFFER,
    GPU_IOCTL_FLUSH_HEAD_BUFFERS,
    GPU_IOCTL_FLUSH_HEAD,
    GPU_IOCTL_SET_HEAD_MODE_SETTING,
    GPU_IOCTL_GET_HEAD_MODE_SETTING,
    GPU_IOCTL_SET_SAFE_HEAD_MODE_SETTING,
    GPU_IOCTL_SET_RESPONSIBLE,
    GPU_IOCTL_UNSET_RESPONSIBLE,
    KEYBOARD_IOCTL_GET_NUM_LOCK,
    KEYBOARD_IOCTL_SET_NUM_LOCK,
    KEYBOARD_IOCTL_GET_CAPS_LOCK,
    KEYBOARD_IOCTL_SET_CAPS_LOCK,
    SIOCATMARK,
    SIOCSIFADDR,
    SIOCGIFADDR,
    SIOCGIFHWADDR,
    SIOCGIFNETMASK,
    SIOCSIFNETMASK,
    SIOCGIFBRDADDR,
    SIOCGIFMTU,
    SIOCGIFFLAGS,
    SIOCGIFCONF,
    SIOCADDRT,
    SIOCDELRT,
    SIOCSARP,
    SIOCDARP,
    FIBMAP,
    FIONBIO,
    FIONREAD,
    FIOCLEX,
    FIONCLEX,
    KCOV_SETBUFSIZE,
    KCOV_ENABLE,
    KCOV_DISABLE,
    SOUNDCARD_IOCTL_SET_SAMPLE_RATE,
    SOUNDCARD_IOCTL_GET_SAMPLE_RATE,
    STORAGE_DEVICE_GET_SIZE,
    STORAGE_DEVICE_GET_BLOCK_SIZE,
    VIRGL_IOCTL_CREATE_CONTEXT,
    VIRGL_IOCTL_CREATE_RESOURCE,
    VIRGL_IOCTL_SUBMIT_CMD,
    VIRGL_IOCTL_TRANSFER_DATA,
    KDSETMODE,
    KDGETMODE,
};

#define TIOCGPGRP TIOCGPGRP
#define TIOCSPGRP TIOCSPGRP
#define TCGETS TCGETS
#define TCSETS TCSETS
#define TCSETSW TCSETSW
#define TCSETSF TCSETSF
#define TCFLSH TCFLSH
#define TIOCGWINSZ TIOCGWINSZ
#define TIOCSCTTY TIOCSCTTY
#define TIOCSTI TIOCSTI
#define TIOCNOTTY TIOCNOTTY
#define TIOCSWINSZ TIOCSWINSZ
#define TIOCGPTN TIOCGPTN
#define GPU_IOCTL_GET_PROPERTIES GPU_IOCTL_GET_PROPERTIES
#define GPU_IOCTL_SET_HEAD_VERTICAL_OFFSET_BUFFER GPU_IOCTL_SET_HEAD_VERTICAL_OFFSET_BUFFER
#define GPU_IOCTL_GET_HEAD_VERTICAL_OFFSET_BUFFER GPU_IOCTL_GET_HEAD_VERTICAL_OFFSET_BUFFER
#define GPU_IOCTL_FLUSH_HEAD_BUFFERS GPU_IOCTL_FLUSH_HEAD_BUFFERS
#define GPU_IOCTL_FLUSH_HEAD GPU_IOCTL_FLUSH_HEAD
#define GPU_IOCTL_SET_HEAD_MODE_SETTING GPU_IOCTL_SET_HEAD_MODE_SETTING
#define GPU_IOCTL_GET_HEAD_MODE_SETTING GPU_IOCTL_GET_HEAD_MODE_SETTING
#define GPU_IOCTL_SET_SAFE_HEAD_MODE_SETTING GPU_IOCTL_SET_SAFE_HEAD_MODE_SETTING
#define GPU_IOCTL_SET_RESPONSIBLE GPU_IOCTL_SET_RESPONSIBLE
#define GPU_IOCTL_UNSET_RESPONSIBLE GPU_IOCTL_UNSET_RESPONSIBLE
#define KEYBOARD_IOCTL_GET_NUM_LOCK KEYBOARD_IOCTL_GET_NUM_LOCK
#define KEYBOARD_IOCTL_SET_NUM_LOCK KEYBOARD_IOCTL_SET_NUM_LOCK
#define KEYBOARD_IOCTL_GET_CAPS_LOCK KEYBOARD_IOCTL_GET_CAPS_LOCK
#define KEYBOARD_IOCTL_SET_CAPS_LOCK KEYBOARD_IOCTL_SET_CAPS_LOCK
#define SIOCATMARK SIOCATMARK
#define SIOCSIFADDR SIOCSIFADDR
#define SIOCGIFADDR SIOCGIFADDR
#define SIOCGIFHWADDR SIOCGIFHWADDR
#define SIOCGIFNETMASK SIOCGIFNETMASK
#define SIOCSIFNETMASK SIOCSIFNETMASK
#define SIOCGIFBRDADDR SIOCGIFBRDADDR
#define SIOCGIFMTU SIOCGIFMTU
#define SIOCGIFFLAGS SIOCGIFFLAGS
#define SIOCGIFCONF SIOCGIFCONF
#define SIOCADDRT SIOCADDRT
#define SIOCDELRT SIOCDELRT
#define SIOCSARP SIOCSARP
#define SIOCDARP SIOCDARP
#define FIBMAP FIBMAP
#define FIONBIO FIONBIO
#define FIONREAD FIONREAD
#define SOUNDCARD_IOCTL_SET_SAMPLE_RATE SOUNDCARD_IOCTL_SET_SAMPLE_RATE
#define SOUNDCARD_IOCTL_GET_SAMPLE_RATE SOUNDCARD_IOCTL_GET_SAMPLE_RATE
#define STORAGE_DEVICE_GET_SIZE STORAGE_DEVICE_GET_SIZE
#define STORAGE_DEVICE_GET_BLOCK_SIZE STORAGE_DEVICE_GET_BLOCK_SIZE
#define VIRGL_IOCTL_CREATE_CONTEXT VIRGL_IOCTL_CREATE_CONTEXT
#define VIRGL_IOCTL_CREATE_RESOURCE VIRGL_IOCTL_CREATE_RESOURCE
#define VIRGL_IOCTL_SUBMIT_CMD VIRGL_IOCTL_SUBMIT_CMD
#define VIRGL_IOCTL_TRANSFER_DATA VIRGL_IOCTL_TRANSFER_DATA
#define KDSETMODE KDSETMODE
#define KDGETMODE KDGETMODE
