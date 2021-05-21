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

#include <Kernel/ACPI/AML/Decode/Scope.h>

namespace Kernel {

namespace ACPI {

Scope::Scope(const Scope& parent_scope, const ByteStream& aml_stream)
    : Package(const_cast<ByteStream&>(aml_stream).forward_and_take_bytes(1, 5))
    , NamedObject(const_cast<ByteStream&>(aml_stream).take_offseted_all_bytes(encoded_length_size()))
    , m_derived_byte_stream(ByteStream::initiate_stream(const_cast<ByteStream&>(aml_stream).take_offseted_bytes(encoded_length_size() + raw_name_length(), inner_size() - raw_name_length())))
    , m_parent_scope(parent_scope)
    , m_offset_in_parent_scope(aml_stream.pointer() - 1)
{
    const_cast<ByteStream&>(aml_stream).forward(Package::size());
}

size_t Scope::size() const
{
    return 1 + Package::size();
}

Scope::Scope(size_t offset_in_parent_scope, size_t size, String name, const ByteStream& stream)
    : Package(size)
    , NamedObject(name)
    , m_derived_byte_stream(stream)
    , m_offset_in_parent_scope(offset_in_parent_scope)
{
}

}
}
