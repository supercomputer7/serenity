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

#include <Kernel/ACPI/DynamicParser.h>
#include <Kernel/ACPI/Parser.h>
#include <Kernel/VM/TypedMapping.h>
#include <Kernel/ACPI/AML/Decode/NamespaceEnumerator.h>

namespace Kernel {
namespace ACPI {

DynamicParser::DynamicParser(PhysicalAddress rsdp)
    : IRQHandler(9)
    , Parser(rsdp)
{
    klog() << "ACPI: Dynamic Parsing Enabled";
    if (dsdt().is_null()) {
        klog() << "ACPI: FADT is not pointing DSDT to a valid address (?)";
        return;
    }
    build_namespace();
}

void DynamicParser::handle_irq(const RegisterState&)
{
    // FIXME: Implement IRQ handling of ACPI signals!
    ASSERT_NOT_REACHED();
}

void DynamicParser::enable_aml_interpretation()
{
    // FIXME: Implement AML Interpretation
    ASSERT_NOT_REACHED();
}
void DynamicParser::enable_aml_interpretation(File&)
{
    // FIXME: Implement AML Interpretation
    ASSERT_NOT_REACHED();
}
void DynamicParser::enable_aml_interpretation(u8*, u32)
{
    // FIXME: Implement AML Interpretation
    ASSERT_NOT_REACHED();
}
void DynamicParser::disable_aml_interpretation()
{


    ASSERT_NOT_REACHED();
}
void DynamicParser::try_acpi_shutdown()
{
    // This code was written in accordance to the ACPI 6.3 specification, page 821.
    // Call _PTS control method with argument 5 - to indicate the platform we're going to shutdown

    // For compatibility reasons, if ACPI version is less then 5.0A, call _GTS method with argument 5

    // If not Hardware-Reduced ACPI, write SLP_TYPa | SLP_ENa to PM1a_CNT register

    // write SLP_TYPb | SLP_ENb to PM1a_CNT register
    ASSERT_NOT_REACHED();
}

ByteBuffer DynamicParser::extract_aml_from_table(PhysicalAddress aml_table, size_t table_length)
{
    auto sdt = map_typed<Structures::AMLTable>(aml_table, PAGE_SIZE + table_length);
    return ByteBuffer::copy(sdt->aml_code ,table_length - sizeof(sdt->h));
}

void DynamicParser::build_namespaced_data_from_buffer(ByteBuffer aml_stream)
{
    new NamespaceEnumerator(aml_stream);
}

void DynamicParser::build_namespace()
{
    klog() << "ACPI: Loading data from DSDT";
    size_t dsdt_length = map_typed<Structures::SDTHeader>(dsdt())->length;
    build_namespaced_data_from_buffer(extract_aml_from_table(dsdt(), dsdt_length));
    
    find_tables("SSDT", [&](PhysicalAddress ssdt, size_t table_length, u8 revision) {
        klog() << "Load SSDT " << ssdt << ", Length " << table_length << " Revision " << revision;
        build_namespaced_data_from_buffer(extract_aml_from_table(dsdt(), dsdt_length));
    });
}

}
}
