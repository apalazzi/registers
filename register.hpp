/*
 Copyright 2017 Andrea Palazzi <palazziandrea@yahoo.it>

 This file is part of halarm.

 halarm is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 halarm is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with the sources.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_REGISTER_HPP_
#define INCLUDE_REGISTER_HPP_

#include <cstdint>
#include <cstddef>
#include <stdexcept>

namespace mcu {
using addr_t=uint32_t; // FIXME: this is architecture-dependent, move it somewhere else.
using uint_t=unsigned int;
template<typename T> T mask(const uint_t pos, const uint_t len) {
    return (((static_cast<T>(1) << len) - 1) << (pos));
};

template<typename T> void check_val_size(const T val,
                                         const uint_t len) {
    if (val >= (static_cast<T>(1) << len)) {
        throw(std::domain_error("Value exceeds Bitfield size."));
    }
}

enum class RegisterType { // TODO: find a better name
    read, write, read_write
};

template<typename T, uint_t pos, uint_t len, typename VAL = T,
    RegisterType RT = RegisterType::read_write> class Bit {
public:
    Bit() = delete;
    Bit(addr_t address) :  // FIXME: use addr_t instead
        raw(reinterpret_cast<T*>(address)) {
        check();
    }
    Bit(T *const address) :
        raw(address) {
        check();
    }
    ~Bit() = default;
    Bit& operator=(const T val) {
        set(val);
        return *this;
    }
    template<typename U=T, typename = typename std::enable_if<len==1, U>::type> operator bool() {
            return static_cast<bool>(get());
    }

    void set(const VAL val,
             const std::nothrow_t nothrow __attribute__((unused))) {
        *raw &= (~mask<T>(pos, len));
        *raw |= (static_cast<T>(val) << pos);
    }
    void set(const VAL val) {
        check_val_size(static_cast<T>(val), len);
        set(val, std::nothrow);
    }
    VAL get() const {
        return (*raw & mask<T>(pos, len)) >> pos;
    }
private:
    volatile T *const raw;
    void check() const noexcept {
        static_assert(len>0, "Bitfield has zero size.");
        static_assert(pos+len<=sizeof(T)*8, "Bitfield size exceeds storage type capacity.");
    }
};

//
//template<typename T, uint32_t pos, uint32_t len, typename VAL,
//    RegisterType RT>
//class Bit<typename T, uint32_t pos, 1, typename VAL,
//RegisterType RT> {
//    public:
//    operator bool() const {return get();}
//    bool operator!() const {return !get();}
//};

template<typename T, uint_t pos0, uint_t len, uint_t step,
    RegisterType RT = RegisterType::read_write, typename IDX = int> class BitArray {
    friend class reference;
public:
    BitArray() = delete;
    BitArray(const addr_t address) :
        raw(reinterpret_cast<T*>(address)) { // FIXME: use addr_t instead?
        check();
    }
    BitArray(addr_t *const address) :
        raw(address) {
        check();
    }
    void set(const uint_t idx,
             const T val,
             const std::nothrow_t nothrow __attribute__((unused))) {
        T m=mask<T>(idx * step + pos0, len);
        *raw &= (~m);
        *raw |= (val << (idx * step + pos0));
    }

    void set(const uint_t idx, const T val) {
        check_index_overflow(idx);
        check_val_size(val, len);
        set(idx, val, std::nothrow);
    }
    T get(const uint_t idx,
          const std::nothrow_t nothrow __attribute__((unused))) const {
        return (*raw & mask<T>(idx * step + pos0, len))
            >> (idx * step + pos0);
    }
    T get(const uint_t idx) const {
        check_index_overflow(idx);
        return get(idx, std::nothrow);
    }

    class reference {
        friend class BitArray;
    public:
        reference& operator=(const T val) {
            bf.set(idx, val);
            return *this;
        }
        reference& operator=(const reference &other) {
            // TODO: check if other is compatible with this
            bf.set(idx, other.bf.test(other.idx));
            return *this;
        }
        operator T() const {
            return bf.get(idx);
        }
        // T operator~() {}; // TODO: bitwise invert
    private:
        reference(const uint_t idx_, BitArray &bf_) :
            idx(idx_), bf(bf_) {
        }
        const uint_t idx;
        BitArray &bf;
    };
    T operator[](const IDX idx) const { // FIXME: constexpr?
        return get(static_cast<uint_t>(idx));
    }
    reference operator[](const IDX idx) {
        return reference(static_cast<uint_t>(idx), *this);
    }

private:
    volatile T *const raw;
    constexpr void check_index_overflow(const uint_t idx) const {
        if (pos0 + step * idx >= sizeof(T) * 8) {
            throw(std::out_of_range("BitfieldArray: idx exceeds type capacity."));
        }
    }
    constexpr void check() const noexcept {
        static_assert(len>0, "BitfieldArray len should be >0.");
        static_assert(step>=len, "BitfieldArray step should be > len.");
        static_assert(pos0+len<=sizeof(T)*8, "BitfieldArray pos0+len exceeds type capacity.");
    }
};

} // namespace

#endif /* INCLUDE_REGISTER_H_ */
