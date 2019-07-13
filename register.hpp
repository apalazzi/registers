/*
 Copyright 2019 Andrea Palazzi <palazziandrea@yahoo.it>

 Thid is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This software is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with halarm.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_REGISTER_HPP_
#define INCLUDE_REGISTER_HPP_

#include <cstdint>
#include <cstddef>
#include <stdexcept>

namespace mcu {

auto mask = [](const uint32_t pos, const uint32_t len) {
	return (((1 << len) - 1) << (pos));
};

template<typename T> constexpr void check_val_size(const T val, const uint32_t len) {
	if (val >= (1u << len)) {
		throw(std::domain_error("Value exceeds Bitfield size."));
	}
}

template<typename T, size_t pos, size_t len> struct Bitfield {
public:
	Bitfield() {
		static_assert(len>0, "Bitfield has zero size.");
		static_assert(pos+len<=sizeof(T)*8, "Bitfield size exceeds storage type capacity.");
	}
	~Bitfield() = default;
	Bitfield& operator=(const T val) {
		set(val);
		return *this;
	}

	void set(const T val,
			const std::nothrow_t nothrow __attribute__((unused))) {
		raw &= (!mask(pos, len));
		raw |= (val << pos);
	}
	void set(const T val) {
		check_val_size(val, len);
		set(val, std::nothrow);
	}
	T get() const {
		return (raw & mask(pos, len)) >> pos;
	}
private:
	volatile T raw;
};

template<typename T, size_t pos0, size_t len, size_t step> struct BitfieldArray {
	friend class reference;
public:
	BitfieldArray() {
		static_assert(len>0, "BitfieldArray len should be >0.");
		static_assert(step>=len, "BitfieldArray step should be > len.");
		static_assert(pos0+len<=sizeof(T)*8, "BitfieldArray pos0+len exceeds type capacity.");
	}
	void set(const size_t idx, const T val,
			const std::nothrow_t nothrow __attribute__((unused))) {
		raw &= (~mask(idx * step + pos0, len));
		raw |= (val << (idx * step + pos0));
	}
	;

	void set(const size_t idx, const T val) {
		check_index_overflow(idx);
		check_val_size(val, len);
		set(idx, val, std::nothrow);
	}
	T get(const size_t idx,
			const std::nothrow_t nothrow __attribute__((unused))) const {
		return (raw & mask(idx * step + pos0, len)) >> (idx * step + pos0);
	}
	T get(const size_t idx) const {
		check_index_overflow(idx);
		return get(idx, std::nothrow);
	}

	class reference {
		friend class BitfieldArray;
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

	private:
		reference(const size_t idx_, BitfieldArray &bf_) :
				idx(idx_), bf(bf_) {
		}

		const size_t idx;
		BitfieldArray &bf;
	};
	T operator[](const size_t idx) const {
		return get(idx);
	}

	reference operator[](const size_t idx) {
		return reference(idx, *this);
	}

private:
	volatile T raw;
	void check_index_overflow(const size_t idx) const {
		if (pos0 + step * idx >= sizeof(T) * 8) {
			throw(std::out_of_range("BitfieldArray: idx exceeds type capacity."));
		}
	}
};
} // namespace mcu

#endif /* INCLUDE_REGISTER_H_ */
