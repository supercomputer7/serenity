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

#include <AK/RefCounted.h>
#include <AK/Types.h>
#include <AK/Vector.h>
#include <Kernel/PhysicalAddress.h>

namespace Kernel {

namespace ACPI {
    enum class OpCode {
        Zero = 0x0,
        One = 0x1,
        Alias = 0x6,
        Name = 0x8,
        BytePrefix = 0xA,
        WordPrefix = 0xB,
        DWORDPrefix = 0xC,
        StringPrefix = 0xD,
        QWordPrefix = 0xE,
        Scope = 0x10,
        Buffer = 0x11,
        Package = 0x12,
        VarPackage = 0x13,
        Method = 0x14,
        External = 0x15,
        DualNamePrefix = 0x2E,
        MultiNamePrefix = 0x2F,
        ExtOpPrefix = 0x5B,
        Local0 = 0x60,
        Local1 = 0x61,
        Local2 = 0x62,
        Local3 = 0x63,
        Local4 = 0x64,
        Local5 = 0x65,
        Local6 = 0x66,
        Local7 = 0x67,
        Arg0 = 0x68,
        Arg1 = 0x69,
        Arg2 = 0x6A,
        Arg3 = 0x6B,
        Arg4 = 0x6C,
        Arg5 = 0x6D,
        Arg6 = 0x6E,
        Store = 0x70,
        RefOf = 0x71,
        Add = 0x72,
        Concat = 0x73,
        Subtract = 0x74,
        Increment = 0x75,
        Decrement = 0x76,
        Multiply = 0x77,
        Divide = 0x78,
        ShiftLeft = 0x79,
        ShiftRight = 0x7A,
        And = 0x7B,
        Nand = 0x7C,
        Or = 0x7D,
        Nor = 0x7E,
        Xor = 0x7F,
        Not = 0x80,
        FindSetLiftBit = 0x81,
        FindSetRightBit = 0x82,
        DerefOf = 0x83,
        ConcatRes = 0x84,
        Mod = 0x85,
        Notify = 0x86,
        SizeOf = 0x87,
        Index = 0x88,
        Match = 0x89,
        CreateDWordField = 0x8A,
        CreateWordField = 0x8B,
        CreateByteField = 0x8C,
        CreateBitField = 0x8D,
        ObjectType = 0x8E,
        CreateQWordField = 0x8F,
        Land = 0x90,
        Lor = 0x91,
        Lnot = 0x92,
        LEqual = 0x93,
        LGreater = 0x94,
        LLess = 0x95,
        ToBuffer = 0x96,
        ToDecimalString = 0x97,
        ToHexString = 0x98,
        ToInteger = 0x99,
        ToString = 0x9C,
        CopyObject = 0x9D,
        Mid = 0x9E,
        Continue = 0x9F,
        If = 0xA0,
        Else = 0xA1,
        While = 0xA2,
        Noop = 0xA3,
        Return = 0xA4,
        Break = 0xA5,
        BreakPoint = 0xCC,
        Ones = 0xFF
    }
    enum class ExtendedOpcode {
        Mutex = 0x1, 
        Event = 0x2,
        CondRefOf = 0x12,
        CreateField = 0x13,
        LoadTable = 0x1F,
        Load = 0x20,
        Stall = 0x21,
        Sleep = 0x22,
        Acquire = 0x23,
        Signal = 0x24,
        Wait = 0x25,
        Reset = 0x26,
        Release = 0x27,
        FromBCD = 0x28,
        ToBCD = 0x29,
        Revision = 0x30,
        Debug = 0x31,
        Fatal = 0x32,
        Timer = 0x33,
        OpRegion = 0x80,
        Field = 0x81,
        Device = 0x82,
        Processor = 0x83,
        PowerRes = 0x84,
        ThermalZone = 0x85,
        IndexField = 0x86,
        BankField = 0x87,
        DataRegion = 0x88,
    }
}
