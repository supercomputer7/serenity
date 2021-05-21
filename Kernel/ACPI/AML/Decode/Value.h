/*
 * Copyright (c) 2020, Liav A. <liavalb@hotmail.co.il>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AK/NonnullRefPtrVector.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>
#include <AK/String.h>
#include <AK/Types.h>
#include <AK/Vector.h>
#include <Kernel/ACPI/AML/Terms/TermArgumentList.h>


namespace Kernel {

namespace ACPI {

enum class EncodedValue {
    ZeroOp = 0x00,
    OneOp = 0x01,
    AliasOp = 0x06,
    NameOp = 0x08,
    BytePrefix = 0x0A,
    WordPredix = 0x0B,
    DWordPrefix = 0x0C,
    StringPreifx = 0x0D,
    QWordPrefix = 0x0E,
    ScopeOp = 0x10,
    BufferOp = 0x11,
    PackageOp = 0x12,
    VarPackageOp = 0x13,
    MethodOp = 0x14,
    ExternalOp = 0x15,
    DualNamePrefix = 0x2E,
    MultiNamePrefix = 0x2F,
    ExtOpPrefix = 0x5B,
    RootChar = 0x5C,
    ParentPrefixChar = 0x5E,
    NameChar = 0x5F,
    Local0Op = 0x60,
    Local1Op = 0x61,
    Local2Op = 0x62,
    Local3Op = 0x63,
    Local4Op = 0x64,
    Local5Op = 0x65,
    Local6Op = 0x66,
    Local7Op = 0x67,
    Arg0Op = 0x68,
    Arg1Op = 0x69,
    Arg2Op = 0x6A,
    Arg3Op = 0x6B,
    Arg4Op = 0x6C,
    Arg5Op = 0x6D,
    Arg6Op = 0x6E,
    StoreOp = 0x70,
    RefOfOp = 0x71,
    AddOp = 0x72,
    ConcatOp = 0x73,
    SubtractOp = 0x74,
    IncrementOp = 0x75,
    DecrementOp = 0x76,
    MultiplyOp = 0x77,
    DivideOp = 0x78,
    ShiftLeftOp = 0x79,
    ShiftRightOp = 0x7A,
    AndOp = 0x7B,
    NandOp = 0x7C,
    OrOp = 0x7D,
    NorOp = 0x7E,
    XorOp = 0x7F,
    NotOp = 0x80,
    FindSetLeftBitOp = 0x81,
    FindSetRightBitOp = 0x82,
    DerefOfOp = 0x83,
    ConcatResOp = 0x84,
    ModOp = 0x85,
    NotifyOp = 0x86,
    SizeOfOp = 0x87,
    IndexOp = 0x88,
    MatchOp = 0x89,
    CreateDWordFieldOp = 0x8A,
    CreateWordFieldOp = 0x8B,
    CreateByteFieldOp = 0x8C,
    CreateBitFieldOp = 0x8D,
    ObjectTypeOp = 0x8E,
    CreateQWordFieldOp = 0x8F,
    LandOp = 0x90,
    LorOp = 0x91,
    LnotOp = 0x92, // Look at ExtendedLogicalValue enum
    LEqualOp = 0x93,
    LGreaterOp = 0x94,
    LLessOp = 0x95,
    ToBufferOp = 0x96,
    ToDecimalStringOp = 0x97,
    ToHexStringOp = 0x98,
    ToIntegerOp = 0x99,
    ToStringOp = 0x9C,
    CopyObjectOp = 0x9D,
    MidOp = 0x9E,
    ContinueOp = 0xF,
    IfOp = 0xA0,
    ElseOp = 0xA1,
    WhileOp = 0xA2,
    NoopOp = 0xA3,
    ReturnOp = 0xA4,
    BreakOp = 0xA5,
    BreakPointOp = 0xCC,
    OnesOp = 0xFF
};

enum class ExtendedLogicalValue {
    LNotEqualOp = 0x93,
    LLessEqualOp = 0x94,
    LGreaterEqualOp = 0x95,
};

enum class ExtendedEncodedValue {
    MutexOp = 0x01,
    EventOp = 0x02,
    CondRefOfOp = 0x012,
    CreateFieldOp = 0x13,
    LoadTableOp = 0x1F,
    LoadOp = 0x20,
    StallOp = 0x21,
    SleepOp = 0x22,
    AcquireOp = 0x23,
    SignalOp = 0x24,
    WaitOp = 0x25,
    ResetOp = 0x26,
    ReleaseOp = 0x27,
    FromBCDOp = 0x28,
    ToBCD = 0x29,
    RevisionOp = 0x30,
    DebugOp = 0x31,
    FatalOp = 0x32,
    TimerOp = 0x33,
    OpRegionOp = 0x80,
    FieldOp = 0x81,
    DeviceOp = 0x82,
    ProcessorOp = 0x83,
    PowerResOp = 0x84,
    ThermalZoneOp = 0x85,
    IndexFieldOp = 0x86,
    BankFieldOp = 0x87,
    DataRegionOp = 0x88,
};

}
}
