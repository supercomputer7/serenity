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
#include <AK/Types.h>
#include <AK/Vector.h>
#include <Kernel/ACPI/AML/Decode/Terms/TermArg.h>

namespace Kernel {

namespace ACPI {

enum class AMLEvaluationType {
    Integer,
    DataRefObject,
    ComputationalData,
    Buffer,
    String,
    ObjectReference,
    Package,
    ByteData,
    ThermalZone,
    Processor,
    Device,
};

class AMLResult : public RefCounted<AMLResult> {
public:
    AMLEvaluationType evaluation_type() const { return m_evaluation_type; }
    virtual u64 as_integer() { ASSERT_NOT_REACHED(); }
    virtual ByteBuffer as_buffer() { ASSERT_NOT_REACHED(); }
    virtual String as_string() { ASSERT_NOT_REACHED(); }
    virtual u8 as_byte_data() { ASSERT_NOT_REACHED(); }
    virtual RefPtr<NamedObject> as_thermal_zone() { ASSERT_NOT_REACHED(); }
    virtual RefPtr<NamedObject> as_processor() { ASSERT_NOT_REACHED(); }
    virtual RefPtr<NamedObject> as_device() { ASSERT_NOT_REACHED(); }
private:
    AMLEvaluationType m_evaluation_type;
};

class ArgObj : public  {
public:
    explicit TermArg(ByteStream);
    size_t size() const { return m_size; }
    virtual NonnullRefPtr<AMLResult> result();

private:
    size_t m_size;
};

}
}
