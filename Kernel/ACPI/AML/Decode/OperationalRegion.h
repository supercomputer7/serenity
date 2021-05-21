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
#include <Kernel/ACPI/AML/Decode/ByteStream.h>
#include <Kernel/ACPI/AML/Decode/NamedObject.h>

namespace Kernel {

namespace ACPI {

enum class RegionSpace : u8 {
    SystemMemory = 0,
    SystemIO,
    PCIConfig,
    EmbeddedControl,
    SMBus,
    SystemCMOS,
    PCIBarTarget,
    IPMI,
    GPIO,
    GenericSerialBus,
    PCC
};

/*
            case ExtendedEncodedValue::OpRegionOp: {
                auto& namestring = *new NameStringView(stream->forward_frame(1));
                u8 region_space = stream[namestring.name().length() + 1];
                dbg() << "New AML Operational Region Definition, " << namestring.name() << " Space is " << region_space;
                auto region_offset_encoding_length = TermArg(stream->forward_frame(namestring.name().length() + 2)).size();
                dbg() << "Region Offset Encoding Length " << region_offset_encoding_length;
                auto region_length_encoding_length = TermArg(stream->forward_frame(region_offset_encoding_length + namestring.name().length() + 2)).size();
                dbg() << "Region Length Encoding Length " << region_length_encoding_length;
                stream = stream->forward_frame(sizeof(extended_value) + namestring.name().length() + sizeof(region_space) + region_offset_encoding_length + region_length_encoding_length);
                break;
            }*/

class OperationalRegion : public RefCounted<OperationalRegion>
    , public SizedObject
    , public NamedObject {

public:
    static NonnullRefPtr<OperationalRegion> define_by_stream(const ByteStream& aml_stream)
    {
        return adopt(*new OperationalRegion(aml_stream));
    }
    RegionSpace region_space() const { return m_region_space; }
    virtual size_t size() const override;

    virtual ~OperationalRegion() { }
private:
    explicit OperationalRegion(const ByteStream& aml_stream);

    RegionSpace m_region_space;
};

}
}
