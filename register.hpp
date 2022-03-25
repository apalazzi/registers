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
#include <vector>

namespace mcu {
using addr_t=
    uint32_t;  // FIXME: this is architecture-dependent,
using uint_t= unsigned int;

template<typename T>
constexpr int n_bits() {
    return sizeof(T) * CHAR_BIT;
}

template<typename T>
constexpr T mask(const uint_t pos, const uint_t len) {
    return (((static_cast<T>(1u) << len) - 1) << (pos));
}

template<typename T>
constexpr void check_val_size(const T val,
                              const uint_t len) {
    if (static_cast<uint_t>(val) > (1u << len) - 1) {
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

// helper classes to use SFINAE later on index types that
// in fact are a mapping types (that is, they have a
// .at(value) method adapted from cppreference example in
// void_t
template<class, class= std::void_t<>>
struct has_at_member: std::false_type {};

// specialization recognizes types that do support .at():
template<class T>
struct has_at_member<
    T, std::void_t<decltype(std::declval<T&>().at(0))>>
    : std::true_type {};

template<typename base_t, uint_t pos, uint_t len,
         typename VAL= base_t,
         RegisterType RT= RegisterType::read_write>
class Bit {
public:
    Bit()= delete;
    Bit(base_t* const address): raw(address) { check(); }
    Bit(addr_t address)
        : Bit(reinterpret_cast<base_t*>(address)) {}
    ~Bit()= default;
    Bit& operator=(const VAL val) {
        set(val);
        return *this;
    }
    bool operator==(VAL other) { return get() == other; }
    bool operator!=(VAL other) { return get() != other; }
    template<typename U= base_t,
             typename=
                 typename std::enable_if<len == 1, U>::type>
    operator bool() {
        return static_cast<bool>(get());
    }
    template<typename U= base_t,
             typename= typename std::enable_if<
                 len == 1 && can_set(RT), U>::type>
    void set() {
        set(true);
    }
    template<typename U= base_t,
             typename= typename std::enable_if<
                 len == 1 && can_reset(RT), U>::type>
    void reset() {
        set(false);
    }
    template<typename U= base_t,
             typename= typename std::enable_if<
                 len == 1 && can_write(RT), U>::type>
    void flip() {
        set(!get());
    }
    void set(const VAL val, const std::nothrow_t nothrow
             __attribute__((unused))) {
        *raw= ((*raw) & ~mask<base_t>(pos, len)) |
              (static_cast<base_t>(val) << pos);
    }
    void set(const VAL val) {
        check_val_size(static_cast<base_t>(val), len);
        set(val, std::nothrow);
    }
    volatile VAL get() const {
        return static_cast<volatile VAL>(
            (*raw & mask<base_t>(pos, len)) >> pos);
    }
    VAL get_raw() const {  // TODO: find a better name, e.g.
                           // get_full_register_value
        return *raw;
    }
    template<typename U,
             typename= std::enable_if_t<
                 len == sizeof(U) * 8 && pos % len == 0>>
    volatile U* get_register_address() const {
        return reinterpret_cast<volatile U*>(raw) +
               (pos / n_bits<U>());
    }

private:
    volatile base_t* const raw;
    void check() const noexcept {
        static_assert(len > 0, "Bitfield has zero size.");
        static_assert(
            pos + len <= n_bits<base_t>(),
            "Bitfield size exceeds storage type capacity.");
    }
};

template<typename base_t, uint_t pos0, uint_t len,
         uint_t step= len,
         uint_t N=
             (n_bits<base_t>() - pos0 + (step - len)) /
             step,
         typename IDX= uint_t, typename val_t= base_t,
         RegisterType RT= RegisterType::read_write>
class BitArray {
    friend class reference;

public:
    BitArray()= delete;
    BitArray(base_t* const address): raw(address) {
        check();
    }
    BitArray(const addr_t address)
        : BitArray(reinterpret_cast<base_t*>(address)) {}
    void set(const IDX idx, const val_t val,
             const std::nothrow_t nothrow
             __attribute__((unused))) {
        auto [m_idx, b_idx]= get_idx(idx);
        *(raw + m_idx)=
            ((*(raw + m_idx) & ~mask<base_t>(b_idx, len)) |
             (static_cast<base_t>(val) << b_idx));
    }
    void set(const IDX idx, const val_t val) {
        check_index_overflow(idx);
        check_val_size(val, len);
        set(idx, val, std::nothrow);
    }
    val_t get(const IDX idx, const std::nothrow_t nothrow
              __attribute__((unused))) const {
        auto [m_idx, b_idx]= get_idx(idx);
        return (*(raw + m_idx) &
                mask<base_t>(b_idx, len)) >>
               b_idx;
    }
    val_t get(const IDX idx) const {
        check_index_overflow(idx);
        return get(idx, std::nothrow);
    }
    base_t get_register_value() const { return *raw; }

    base_t* get_register_address() const { return raw; }

    class reference {
        friend class BitArray;

    public:
        reference& operator=(const val_t val) {
            bf.set(idx, val);
            return *this;
        }
        reference& operator=(const reference& other) {
            // TODO: check if other is compatible with this
            bf.set(idx, other.bf.get(other.idx));
            return *this;
        }
        template<
            uint_t U= len,
            typename= typename std::enable_if<U == 1>::type>
        operator bool() {
            return static_cast<bool>(bf.get(idx));
        }
        // T operator~() {}; // TODO: bitwise invert
    private:
        reference(const IDX idx_, BitArray& bf_)
            : idx(idx_), bf(bf_) {}
        const IDX idx;
        BitArray& bf;
    };
    val_t operator[](const IDX idx) const {
        return get(idx);
    }
    reference operator[](const IDX idx) {
        return reference(idx, *this);
    }

private:
    volatile base_t* const raw;
    std::tuple<const uint_t, const uint_t> get_idx(
        const IDX idx) const {
        // FIXME: we do some rather strict hypothesis here
        // - one single array element cannot span over
        // multiple base_t types
        // - pos0 is only considered for the first base_t
        const uint_t m_idx=
            (static_cast<uint_t>(idx) * step + pos0) /
            n_bits<base_t>();
        const uint_t n0= (n_bits<base_t>() - pos0) / len;
        if (m_idx == 0) {
            const uint_t b_idx=
                static_cast<uint_t>(idx) * step + pos0;
            return {m_idx, b_idx};
        } else {
            const uint_t b_idx=
                (static_cast<uint_t>(idx) - n0) * step -
                n_bits<base_t>() * (m_idx - 1);
            return {m_idx, b_idx};
        }
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

template<class base_t, uint_t idx0, uint_t step, uint_t N>
class RegisterArray {
public:
    RegisterArray()= delete;
    RegisterArray(const addr_t address) {
        for (uint_t i= 0; i < N; ++i) {
            reg.push_back(base_t{address + i * step});
        }
    }

    base_t& operator[](const uint_t idx) {
        return reg[idx];
    }
    base_t get_register_value(const uint_t idx) const {
        return reg[idx].get_register_value();
    }
    base_t* get_register_address(const uint_t idx) const {
        return reg[idx].get_register_address;
    }

private:
    // TODO: find a more suitable container
    std::vector<base_t> reg;
};
}  // namespace mcu

#endif /* INCLUDE_REGISTER_H_ */
