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

#include <AK/ByteBuffer.h>
#include <AK/RefCounted.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <AK/Types.h>
#include <AK/Vector.h>
#include <Kernel/PhysicalAddress.h>

namespace Kernel {

namespace ACPI {
class ByteStream : public RefCounted<ByteStream> {
public:
    static NonnullRefPtr<ByteStream> initiate_stream(ByteBuffer);

    u8 take_byte() const;
    u8 take_offseted_byte(size_t offset) const;
    u8 take_byte_and_forward();
    u8 forward_and_take_byte();

    ByteBuffer take_bytes(size_t byte_count) const;
    ByteBuffer take_all_bytes();
    ByteBuffer take_offseted_all_bytes(size_t offset);
    ByteBuffer take_offseted_bytes(size_t offset, size_t byte_count);
    ByteBuffer take_bytes_and_forward(size_t byte_count);

    ByteBuffer forward_and_take_bytes_and_forward(size_t forward_count, size_t byte_count);
    ByteBuffer forward_and_take_bytes(size_t offset, size_t byte_count);

    void forward(size_t byte_count);
    size_t size() const { return m_stream.size(); }
    bool is_pointing_to_end() const { return m_pointer == m_stream.size(); }
    size_t pointer() const { return m_pointer; }

private:
    explicit ByteStream(ByteBuffer);

    size_t m_pointer { 0 };
    ByteBuffer m_stream;
};

}
}
