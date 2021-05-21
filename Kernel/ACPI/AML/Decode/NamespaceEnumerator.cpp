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
#include <Kernel/ACPI/AML/Decode/GlobalScope.h>
#include <Kernel/ACPI/AML/Decode/Method.h>
#include <Kernel/ACPI/AML/Decode/NameStringView.h>
#include <Kernel/ACPI/AML/Decode/NamespaceEnumerator.h>
#include <Kernel/ACPI/AML/Decode/OperationalRegion.h>
#include <Kernel/ACPI/AML/Decode/Scope.h>
#include <Kernel/ACPI/AML/Decode/Value.h>

#define FIRST_6_BITS 0x3F
#define FIRST_4_BITS 0x0F

namespace Kernel {

namespace ACPI {

void NamespaceEnumerator::enumerate(const Scope& scope)
{
    auto stream = scope.derived_stream();
    dbg() << "Scoped Stream (" << scope.name() << "), Size " << stream->size();
    while (!stream->is_pointing_to_end()) {
        switch ((EncodedValue)stream->take_byte()) {
        case EncodedValue::ScopeOp: {

            auto found_scope = Scope::define_by_stream(scope, stream);
            dbg() << "New AML Scope Definition " << found_scope->name() << " (" << found_scope->name().length() << ") , size " << found_scope->size();
            m_scopes_definitions.append(scope);
            enumerate(found_scope);
            break;
        }
        case EncodedValue::MethodOp: {
            auto method = Method::define_by_stream(stream);
            dbg() << "New AML Method Definition " << method->name() << ", Size " << method->size() << ", Method flags " << method->flags();
            m_methods_definitions.append(method);
            //size_t termlist_start = skip_bytes_count + scope->name().length();
            //enumerate(stream->frame_slice(scope->offset(), scope->size(), termlist_start));

            //stream = stream->forward_frame(skip_bytes_count + method->name().length() + sizeof(u8) + method->size());
            break;
        }

            /*

        case EncodedValue::AliasOp: {
            auto& first_namestring = *new NameStringView(stream->forward_frame(1));
            auto& alias_namestring = *new NameStringView(stream->forward_frame(1 + first_namestring.name().length()));
            dbg() << "New AML Alias Definition " << first_namestring.name()[0];
            stream = stream->forward_frame(1 + first_namestring.name().length() + alias_namestring.name().length());
            break;
        }
        */

        case EncodedValue::ExtOpPrefix: {
            u8 extended_value = stream->forward_and_take_byte();
            switch ((ExtendedEncodedValue)extended_value) {
                case ExtendedEncodedValue::OpRegionOp: {
                    auto operational_region = OperationalRegion::define_by_stream(stream);
                    dbg() << "ACPI: New Operational Region definition";
                    break;
                }
            
            default:
                RefPtr<Scope> current_traced_scope = scope;
                klog() << "ACPI Error: Unknown extended term @ byte position " << stream->pointer() << ", value " << String::format("0x%x", extended_value);

                while (!current_traced_scope.is_null()) {
                    klog() << " @ Scope (" << current_traced_scope->name() << "): " << current_traced_scope->offset_in_parent_scope();
                    current_traced_scope = current_traced_scope->parent_scope();
                }
                
                ASSERT_NOT_REACHED();
                break;
            }
            break;
        }
        default:
            dbg() << "ACPI: Unknown term @ " << stream->pointer() << ", value " << String::format("0x%x", stream->take_byte());
            RefPtr<Scope> parent_scope = scope.parent_scope();
            while (!parent_scope.is_null()) {
                dbg() << "ACPI: Scope @ scope " << parent_scope->name();
                parent_scope = parent_scope->parent_scope();
            }
            ASSERT_NOT_REACHED();
            break;
        };
    }
}

NamespaceEnumerator::NamespaceEnumerator(ByteBuffer aml_code)
    : m_bytecode(aml_code)
{
    // We build the ACPI namespace in multiple steps.
    // The first step is to find all scopes, and save their offsets.
    // Next, we need to find all methods in the scopes we found earlier.
    m_byte_pointer = 0;

    dbg() << "aml size " << aml_code.size();
    dbg() << "ACPI: Global Scope generated from AML buffer, size " << aml_code.size();
    auto global_scope = GlobalScope::define(0, aml_code.size(), "^\\", ByteStream::initiate_stream(aml_code));
    enumerate(global_scope);
}

}
}
