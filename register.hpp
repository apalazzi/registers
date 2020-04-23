/*
 Copyright 2017 Andrea Palazzi <palazziandrea@yahoo.it>

 This file is part of mcu++.

 halarm is free software: you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later
 version.

 halarm is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty
 of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 the GNU General Public License for more details.

 You should have received a copy of the GNU General Public
 License along with the sources.  If not, see
 <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_REGISTER_HPP_
#define INCLUDE_REGISTER_HPP_

#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace mcu {
using addr_t=
    uint32_t;  // FIXME: this is architecture-dependent,
using uint_t= unsigned int;

template<typename T>
constexpr int n_bits() {
    return sizeof(T) * CHAR_BIT;
}

// template<typename T> constexpr max_N(const uint_t pos0,
// const uint_t step) { 	return (n_bits<T>() - pos0) /
// step;
//}

template<typename T>
T mask(const uint_t pos, const uint_t len) {
    return (((static_cast<T>(1) << len) - 1) << (pos));
}

template<typename T>
void check_val_size(const T val, const uint_t len) {
    if (val >= (static_cast<T>(1) << len)) {
        throw(std::domain_error(
            "Value exceeds storage size."));
    }
}

enum class RegisterType {  // TODO: find a better name
    read,
    write,
    read_write,
    read_clear_w0,
    read_clear_w1
};

constexpr bool can_read(RegisterType rt) {
    return (rt == RegisterType::read ||
            rt == RegisterType::read_write ||
            rt == RegisterType::read_clear_w0);
}

constexpr bool can_write(RegisterType rt) {
    return (rt == RegisterType::write ||
            rt == RegisterType::read_write);
}

constexpr bool can_set(RegisterType rt) {
    return (can_write(rt) ||
            rt == RegisterType::read_clear_w1);
}

constexpr bool can_reset(RegisterType rt) {
    return (can_write(rt) ||
            rt == RegisterType::read_clear_w0);
}

template<typename T, uint_t pos, uint_t len,
         typename VAL= T,
         RegisterType RT= RegisterType::read_write>
class Bit {
public:
    Bit()= delete;
    Bit(addr_t address)
        : raw(reinterpret_cast<T*>(address)) {
        check();
    }
    Bit(T* const address): raw(address) { check(); }
    ~Bit()= default;
    Bit& operator=(const VAL val) {
        set(val);
        return *this;
    }
    bool operator==(VAL other) { return get() == other; }
    bool operator!=(VAL other) { return get() != other; }
    template<typename U= T,
             typename=
                 typename std::enable_if<len == 1, U>::type>
    operator bool() {
        return static_cast<bool>(get());
    }
    template<typename U= T,
             typename= typename std::enable_if<
                 len == 1 && can_set(RT), U>::type>
    void set() {
        set(true);
    }
    template<typename U= T,
             typename= typename std::enable_if<
                 len == 1 && can_reset(RT), U>::type>
    void reset() {
        set(false);
    }
    template<typename U= T,
             typename= typename std::enable_if<
                 len == 1 && can_write(RT), U>::type>
    void flip() {
        set(!get());
    }

    // TODO: add SFINAE here
    void set(const VAL val, const std::nothrow_t nothrow
             __attribute__((unused))) {
        *raw&= (~mask<T>(pos, len));
        *raw|= (static_cast<T>(val) << pos);
    }
    void set(const VAL val) {
        check_val_size(static_cast<T>(val), len);
        set(val, std::nothrow);
    }
    VAL get() const {
        return static_cast<VAL>(
            (*raw & mask<T>(pos, len)) >> pos);
    }

private:
    volatile T* const raw;
    void check() const noexcept {
        static_assert(len > 0, "Bitfield has zero size.");
        static_assert(
            pos + len <= sizeof(T) * 8,
            "Bitfield size exceeds storage type capacity.");
    }
};

template<typename base_t, uint_t pos0, uint_t len,
         uint_t step= len,
         uint_t N=
             (n_bits<base_t>() - pos0 + (step - len)) /
             step,
         typename IDX= uint_t,
         RegisterType RT= RegisterType::read_write>
class BitArray {
    friend class reference;

public:
    BitArray()= delete;
    BitArray(const addr_t address)
        : raw(reinterpret_cast<base_t*>(address)) {
        check();
    }
    BitArray(addr_t* const address): raw(address) {
        check();
    }
    void set(const IDX idx, const base_t val,
             const std::nothrow_t nothrow
             __attribute__((unused))) {
        auto [m_idx, b_idx]= get_idx(idx);
        base_t m= mask<base_t>(b_idx, len);
        *(raw + m_idx)&= (~m);
        *(raw + m_idx)|= (val << b_idx);
    }

    void set(const IDX idx, const base_t val) {
        check_index_overflow(idx);
        check_val_size(val, len);
        set(idx, val, std::nothrow);
    }
    base_t get(const IDX idx, const std::nothrow_t nothrow
               __attribute__((unused))) const {
        auto [m_idx, b_idx]= get_idx(idx);

        return (*(raw + m_idx) &
                mask<base_t>(b_idx, len)) >>
               b_idx;
    }
    base_t get(const IDX idx) const {
        check_index_overflow(idx);
        return get(idx, std::nothrow);
    }

    class reference {
        friend class BitArray;

    public:
        reference& operator=(const base_t val) {
            bf.set(idx, val);
            return *this;
        }
        reference& operator=(const reference& other) {
            // TODO: check if other is compatible with this
            bf.set(idx, other.bf.test(other.idx));
            return *this;
        }
        operator base_t() const { return bf.get(idx); }
        // T operator~() {}; // TODO: bitwise invert
    private:
        reference(const IDX idx_, BitArray& bf_)
            : idx(idx_), bf(bf_) {}
        const IDX idx;
        BitArray& bf;
    };
    base_t operator[](
        const IDX idx) const {
        return get(idx);
    }
    reference operator[](const IDX idx) {
        return reference(idx, *this);
    }
    template<typename U= base_t,
             typename=
                 typename std::enable_if<len == 1, U>::type>
    operator bool() {
        return static_cast<bool>(get());
    }

private:
    volatile base_t* const raw;
    std::tuple<const uint_t, const uint_t> get_idx(
        const IDX idx) const {
        const uint_t m_idx=
            (static_cast<uint_t>(idx) * step + pos0) /
            n_bits<base_t>();
        const uint_t b_idx=
            static_cast<uint_t>(idx) * step + pos0 -
			n_bits<base_t>() * m_idx;
        return {m_idx, b_idx};
    }
    constexpr void check_index_overflow(
        const IDX idx) const {
        if (static_cast<uint_t>(idx) >= N) {
            throw(std::out_of_range(
                "BitfieldArray: idx exceeds type "
                "capacity."));
        }
    }
    constexpr void check() const noexcept {
        static_assert(len > 0,
                      "BitfieldArray len should be >0.");
        static_assert(
            step >= len,
            "BitfieldArray step should be > len.");
        static_assert(
            pos0 + len <= sizeof(base_t) * CHAR_BIT,
            "BitfieldArray pos0+len exceeds type "
            "capacity.");
    }
};

}  // namespace mcu

#endif /* INCLUDE_REGISTER_H_ */
