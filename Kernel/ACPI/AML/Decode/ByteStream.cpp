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

#include <Kernel/ACPI/AML/Decode/ByteStream.h>

namespace Kernel {

namespace ACPI {

NonnullRefPtr<ByteStream> ByteStream::initiate_stream(ByteBuffer byte_stream)
{
    return adopt(*new ByteStream(byte_stream));
}
ByteStream::ByteStream(ByteBuffer byte_stream)
    : m_stream(byte_stream)
{
}

u8 ByteStream::take_byte() const
{
    return take_offseted_byte(0);
}

u8 ByteStream::take_byte_and_forward()
{
    return m_stream[m_pointer++];
}
u8 ByteStream::forward_and_take_byte()
{
    return m_stream[++m_pointer];
}

ByteBuffer ByteStream::take_offseted_bytes(size_t offset, size_t byte_count)
{
    return m_stream.slice(m_pointer + offset, byte_count);
}

ByteBuffer ByteStream::take_bytes_and_forward(size_t byte_count)
{
    return forward_and_take_bytes_and_forward(0, byte_count);
}
ByteBuffer ByteStream::take_all_bytes()
{
    return take_bytes(m_stream.size() - m_pointer);
}

u8 ByteStream::take_offseted_byte(size_t offset) const
{
    return m_stream[m_pointer + offset];
}

ByteBuffer ByteStream::take_offseted_all_bytes(size_t offset)
{
    return m_stream.slice(m_pointer + offset, m_stream.size() - (m_pointer + offset));
}

ByteBuffer ByteStream::forward_and_take_bytes_and_forward(size_t forward_count, size_t byte_count)
{
    ByteBuffer new_buffer = m_stream.slice(m_pointer + forward_count, byte_count);
    m_pointer += byte_count + forward_count;
    return new_buffer;
}

ByteBuffer ByteStream::forward_and_take_bytes(size_t offset, size_t byte_count)
{
    ByteBuffer new_buffer = m_stream.slice(m_pointer + offset, byte_count);
    m_pointer += offset;
    return new_buffer;
}

ByteBuffer ByteStream::take_bytes(size_t byte_count) const
{
    return m_stream.slice(m_pointer, byte_count);
}
void ByteStream::forward(size_t byte_count)
{
    m_pointer += byte_count;
}

}
}
