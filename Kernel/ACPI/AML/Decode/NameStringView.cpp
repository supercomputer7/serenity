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

#include <Kernel/ACPI/AML/Decode/NameStringView.h>
#include <Kernel/ACPI/AML/Decode/Value.h>

namespace Kernel {

namespace ACPI {

String NameStringView::parse_namestring(ByteBuffer name_string)
{
    if (name_string[0] == (u8)EncodedValue::DualNamePrefix) {
        return StringView(name_string.slice_view(1, 8));
    }

    if (name_string[0] == (u8)EncodedValue::MultiNamePrefix) {
        return StringView(name_string.slice_view(2, name_string[1] * 4));
    }

    if (name_string[0] == '\\') {
        return parse_namestring(name_string.slice_view(1, name_string.size() - 1));
    }

    auto sliced_name_string = StringView(name_string.slice_view(0, 4));
    auto name_trimmed_with_null = sliced_name_string.find_first_of((char)0x0);
    if (name_trimmed_with_null.has_value()) {
        m_null_terminated = true;
        if (name_trimmed_with_null.value() == 0)
            return "\\";
        return sliced_name_string.substring_view(0, name_trimmed_with_null.value());
    }
    return sliced_name_string.substring_view(0, 4);
}

NameStringView::NameStringView(ByteBuffer name_string)
    : m_name(parse_namestring(name_string))
{
}

NameStringView::NameStringView(String name)
    : m_name(name)
{
}

}
}
