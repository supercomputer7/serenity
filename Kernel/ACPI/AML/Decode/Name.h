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
#include <Kernel/ACPI/AML/Decode/NameStringView.h>
#include <Kernel/ACPI/AML/Decode/NamedObject.h>
#include <Kernel/ACPI/AML/Decode/Scope.h>

namespace Kernel {

namespace ACPI {

class Name : public RefCounted<Name>
    , public SizedObject
    , public NamedObject {
public:
    static NonnullRefPtr<Name> define_by_stream(const Scope& parent_scope, const ByteStream& aml_stream)
    {
        return adopt(*new Name(parent_scope, aml_stream));
    }

    NonnullRefPtr<ByteStream> derived_stream() const
    {
        return m_derived_byte_stream;
    }

    RefPtr<Scope> parent_scope() const
    {
        return m_parent_scope;
    }


private:
    Scope(const Scope& parent_scope, const ByteStream& aml_stream);

    NonnullRefPtr<ByteStream> m_derived_byte_stream;
    RefPtr<Scope> m_parent_scope;
    size_t m_offset_in_parent_scope;
};

}
}
