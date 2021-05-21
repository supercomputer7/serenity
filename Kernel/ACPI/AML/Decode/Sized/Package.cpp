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

#include <Kernel/ACPI/AML/Decode/Sized/Package.h>

#define FIRST_6_BITS 0x3F
#define FIRST_4_BITS 0x0F

namespace Kernel {

namespace ACPI {
Package::Package(const ByteStream& aml_stream, ByteBuffer stream)
    : m_size(calculate_length(stream))
    , m_lead_byte(stream[0])
{
    const_cast<ByteStream&>(aml_stream).forward(encoded_length_size());
}

Package::Package(ByteBuffer stream)
    : m_size(calculate_length(stream))
    , m_lead_byte(stream[0])
{
}

Package::Package(size_t size)
    : m_size(size)
    , m_lead_byte(0)
    , m_real_package(false)
{
}
size_t Package::calculate_length(ByteBuffer packed_length) const
{
    if (!(packed_length[0] & (1 << 7) || packed_length[0] & (1 << 6))) {
        return packed_length[0] & FIRST_6_BITS;
    }
    size_t size = packed_length[0] & FIRST_4_BITS;
    for (int index = 1; index < (packed_length[0] >> 6) + 1; index++) {
        size += packed_length[index] << (((index - 1) * 8) + 4);
    }
    return size;
}

size_t Package::inner_size() const
{
    if (!m_real_package)
        return size();
    return size() - encoded_length_size();
};

size_t Package::encoded_length_size() const
{
    if (!m_real_package)
        return 0;
    return (m_lead_byte >> 6) + 1;
}
}
}
