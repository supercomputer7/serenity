/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Edwin Hoksberg <mail@edwinhoksberg.nl>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/cdefs.h>

__BEGIN_DECLS

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

struct FBProperties {
    unsigned char doublebuffer_support;
    unsigned char flushing_support;
    unsigned char partial_flushing_support;
    // Note: This can indicate to userland if the underlying hardware requires
    // a defined refresh rate being supplied when modesetting the screen resolution.
    // Paravirtualized hardware don't need such setting and can safely ignore this.
    unsigned char refresh_rate_support;
    unsigned char hardware_3d_acceleration_command_set;
};

struct FBHeadModeSetting {
    int horizontal_stride;
    int pixel_clock_in_khz;
    int horizontal_active;
    int horizontal_sync_start;
    int horizontal_sync_end;
    int horizontal_total;
    int vertical_active;
    int vertical_sync_start;
    int vertical_sync_end;
    int vertical_total;
};

struct FBHeadProperties {
    FBHeadModeSetting mode_setting;
    unsigned offset;
};

struct FBHeadEDID {
    unsigned char* bytes;
    unsigned bytes_size;
};

struct FBHeadVerticalOffset {
    int offsetted;
};

struct FBRect {
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

__END_DECLS

enum IOCtlNumber {
    TIOCSETMODE,
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
    FB_IOCTL_GET_PROPERTIES,
    FB_IOCTL_GET_HEAD_PROPERTIES,
    FB_IOCTL_SET_HEAD_MODE_SETTING,
    FB_IOCTL_GET_HEAD_EDID,
    FB_IOCTL_SET_HEAD_VERTICAL_OFFSET_BUFFER,
    FB_IOCTL_GET_HEAD_VERTICAL_OFFSET_BUFFER,
    FB_IOCTL_FLUSH_HEAD_BUFFERS,
    FB_IOCTL_FLUSH_HEAD,
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
#define FB_IOCTL_GET_PROPERTIES FB_IOCTL_GET_PROPERTIES
#define FB_IOCTL_GET_HEAD_PROPERTIES FB_IOCTL_GET_HEAD_PROPERTIES
#define FB_IOCTL_GET_HEAD_EDID FB_IOCTL_GET_HEAD_EDID
#define FB_IOCTL_SET_HEAD_MODE_SETTING FB_IOCTL_SET_HEAD_MODE_SETTING
#define FB_IOCTL_SET_HEAD_VERTICAL_OFFSET_BUFFER FB_IOCTL_SET_HEAD_VERTICAL_OFFSET_BUFFER
#define FB_IOCTL_GET_HEAD_VERTICAL_OFFSET_BUFFER FB_IOCTL_GET_HEAD_VERTICAL_OFFSET_BUFFER
#define FB_IOCTL_FLUSH_HEAD_BUFFERS FB_IOCTL_FLUSH_HEAD_BUFFERS
#define FB_IOCTL_FLUSH_HEAD FB_IOCTL_FLUSH_HEAD
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
