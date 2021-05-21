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
#include <Kernel/ACPI/AML/Decode/ByteStream.h>
#include <Kernel/ACPI/AML/Decode/Sized/SizedObject.h>

namespace Kernel {

namespace ACPI {
class Package : public SizedObject {
public:
    virtual size_t size() const override { return m_size; };
    size_t inner_size() const;
    size_t encoded_length_size() const;

    virtual ~Package() { }

protected:
    Package(const ByteStream&, ByteBuffer);
    explicit Package(ByteBuffer);
    explicit Package(size_t);

private:
    size_t calculate_length(ByteBuffer) const;
    size_t m_size;
    u8 m_lead_byte;
    bool m_real_package { true };
};

/*
class PackageElement : public RefCounted<PackageElement> {
public:
private:
    String m_name;

};

class PackageElementList : public RefCounted<PackageElementList> {
public:
private:
    RefPtr<PackageElement> m_package_element;
    RefPtr<PackageElementList> m_package_element_list;
};

class ElementsPackage : public RefCounted<ElementsPackage>
    , public Package {
public:
private:
    u8 m_elements_count;

};

class VariableElementsPackage : public RefCounted<VariableElementsPackage>
    , public Package {
public:
private:
};
*/

}
}
