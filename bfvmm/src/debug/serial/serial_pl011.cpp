//
// Bareflank Hypervisor
// Copyright (C) 2015 Assured Information Security, Inc.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include <bfgsl.h>
#include <bfarch.h>
#include <bfsupport.h>
#include <debug/serial/serial_pl011.h>

namespace bfvmm
{

serial_pl011::serial_pl011(uintptr_t port) noexcept
{
#ifdef BF_AARCH64
    if (port == DEFAULT_COM_PORT) {
        auto platform_info = get_platform_info();
        port = platform_info->serial_address;
    }
#endif

    m_port = port;

    auto bits = read_32(uartlcr_h_reg);

    bits |= uartlcr_h_fifo_enable;

    write_32(uartimsc_reg, 0);
    write_32(uartlcr_h_reg, bits);

    this->set_baud_rate_divisor(DEFAULT_BAUD_RATE_INT, DEFAULT_BAUD_RATE_FRAC);
    this->set_data_bits(DEFAULT_DATA_BITS);
    this->set_stop_bits(DEFAULT_STOP_BITS);
    this->set_parity_bits(DEFAULT_PARITY_BITS);
}

#ifdef BF_AARCH64
serial_pl011 *
serial_pl011::instance() noexcept
{
    static serial_pl011 serial{};
    return &serial;
}
#endif

// This method has to write UARTLCR_H as well because of a quirk in the PL011
// implementation: UARTIBRD, UARTFBRD, and UARTLCR_H internally form a single
// register that is only updated on a write to UARTLCR_H.
void
serial_pl011::set_baud_rate_divisor(uint32_t int_part, uint32_t frac_part) noexcept
{
    int_part &= 0xFFFF;
    frac_part &= 0x3F;

    auto lcr_h = read_32(uartlcr_h_reg);

    write_32(uartibrd_reg, int_part);
    write_32(uartfbrd_reg, frac_part);
    write_32(uartlcr_h_reg, lcr_h);
}

void
serial_pl011::baud_rate_divisor(uint32_t &int_part, uint32_t &frac_part) const noexcept
{
    int_part = read_32(uartibrd_reg);
    frac_part = read_32(uartfbrd_reg);
}

void
serial_pl011::set_data_bits(data_bits_t bits) noexcept
{
    switch (bits) {
        case char_length_5:
        case char_length_6:
        case char_length_7:
        case char_length_8:
            break;
        default:
            bits = DEFAULT_DATA_BITS;
    }

    auto lcr_h = read_32(uartlcr_h_reg);

    lcr_h &= ~uartlcr_h_wlen_mask;
    lcr_h |= bits & uartlcr_h_wlen_mask;

    write_32(uartlcr_h_reg, lcr_h);
}

serial_pl011::data_bits_t
serial_pl011::data_bits() const noexcept
{
    return static_cast<data_bits_t>(read_32(uartlcr_h_reg) & uartlcr_h_wlen_mask);
}

void
serial_pl011::set_stop_bits(stop_bits_t bits) noexcept
{
    switch (bits) {
        case stop_bits_1:
        case stop_bits_2:
            break;
        default:
            bits = DEFAULT_STOP_BITS;
    }

    auto lcr_h = read_32(uartlcr_h_reg);

    lcr_h &= ~uartlcr_h_stop_mask;
    lcr_h |= bits & uartlcr_h_stop_mask;

    write_32(uartlcr_h_reg, lcr_h);
}

serial_pl011::stop_bits_t
serial_pl011::stop_bits() const noexcept
{
    return static_cast<stop_bits_t>(read_32(uartlcr_h_reg) & uartlcr_h_stop_mask);
}

void
serial_pl011::set_parity_bits(parity_bits_t bits) noexcept
{
    switch (bits) {
        case parity_none:
        case parity_odd:
        case parity_even:
        case parity_mark:
        case parity_space:
            break;
        default:
            bits = DEFAULT_PARITY_BITS;
    }

    auto lcr_h = read_32(uartlcr_h_reg);

    lcr_h &= ~uartlcr_h_parity_mask;
    lcr_h |= bits & uartlcr_h_parity_mask;

    write_32(uartlcr_h_reg, lcr_h);
}

serial_pl011::parity_bits_t
serial_pl011::parity_bits() const noexcept
{
    return static_cast<parity_bits_t>(read_32(uartlcr_h_reg) & uartlcr_h_parity_mask);
}

void
serial_pl011::write(char c) noexcept
{
    while (get_status_full_transmitter())
    { }

    write_32(uartdr_reg, static_cast<uint32_t>(static_cast<unsigned char>(c)));
}

bool
serial_pl011::get_status_full_transmitter() const noexcept
{
    return (read_32(uartfr_reg) & uartfr_tx_full) != 0;
}

uint32_t
serial_pl011::read_32(ptrdiff_t offset) const noexcept
{
    bfignored(offset);
    return 0;

    // auto ptr = reinterpret_cast<uint32_t const volatile *>(port() + static_cast<uintptr_t>(offset));
    // return *ptr;
}

void
serial_pl011::write_32(ptrdiff_t offset, uint32_t data) const noexcept
{
    bfignored(offset);
    bfignored(data);

    // auto ptr = reinterpret_cast<uint32_t volatile *>(port() + static_cast<uintptr_t>(offset));
    // *ptr = data;
}

}
