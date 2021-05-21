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
#include <AK/String.h>
#include <AK/Types.h>
#include <AK/Vector.h>
#include <Kernel/ACPI/AML/Decode/ByteStream.h>
#include <Kernel/ACPI/AML/Decode/NameStringView.h>
#include <Kernel/ACPI/AML/Decode/NamedObject.h>
#include <Kernel/ACPI/AML/Decode/Sized/Package.h>
#include <Kernel/PhysicalAddress.h>

namespace Kernel {

namespace ACPI {

class Method : public RefCounted<Method>
    , public Package
    , public NamedObject {
public:
    static NonnullRefPtr<Method> define_by_stream(const ByteStream& aml_stream)
    {
        return adopt(*new Method(aml_stream));
    }
    u8 flags() const { return m_flags; }

    NonnullRefPtr<ByteStream> derived_stream() const
    {
        return m_derived_byte_stream;
    }
    virtual ~Method() { }
private:
    explicit Method(const ByteStream& aml_stream)
        : Package(const_cast<ByteStream&>(aml_stream).forward_and_take_bytes(1, 5))
        , NamedObject(const_cast<ByteStream&>(aml_stream).take_offseted_all_bytes(encoded_length_size()))
        , m_flags(aml_stream.take_offseted_byte(encoded_length_size() + raw_name_length()))
        , m_derived_byte_stream(ByteStream::initiate_stream(const_cast<ByteStream&>(aml_stream).take_offseted_bytes(encoded_length_size() + raw_name_length() + sizeof(u8), inner_size() - raw_name_length() - sizeof(u8))))
    {
        const_cast<ByteStream&>(aml_stream).forward(size());
    }

    u8 m_flags;
    NonnullRefPtr<ByteStream> m_derived_byte_stream;
};

}
}
