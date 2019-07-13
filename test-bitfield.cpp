#include <register.hpp>
using namespace mcu;

#include <stdexcept>
#include <vector>
#include <cassert>
#include <iostream>
using namespace std;

bool test_bitfield() {
	// those will fail to compile because they are invalid
	//Bitfield<uint16_t, 0, 18> bf1;
	//static_assert(false, "Bitfield size > type capacity.");
	//Bitfield<uint16_t, 18, 1> bf2;
	//static_assert(false, "Bitfield position > type capacity.");
	//Bitfield<uint16_t, 2, 0>bf3;
	//static_assert(false, "Bitfield size must be >0.");
	//Bitfield<uint16_t, 15, 2>bf4;
	//static_assert(false, "Bitfield position+size>type capacity.");

	Bitfield<uint16_t, 0, 2> bf;
	std::vector<uint16_t> val = { 0b00, 0b01, 0b10, 0b11 };
	for (auto v : val) {
		bf.set(v);
		assert(bf.get() == v);
	}
	try {
		bf.set(0b100); // value too large
		assert(false);
	} catch (std::domain_error &e) {
		// ok
	}
	return true;
}

bool test_bitfield_array() {
	//BitfieldArray<uint16_t, 0,0,1> bfa1;
	//static_assert(false, "Array len >0 failed.");
	//BitfieldArray<uint16_t, 0,1,0> bfa2;
	//static_assert(false, "Array step >len failed.");
	//BitfieldArray<uint16_t, 0,0,17> bfa3;
	//static_assert(false, "Array step >len failed.");
	BitfieldArray<uint16_t, 0, 16, 16> bf;
	BitfieldArray<uint16_t, 0, 2, 2> bfa;
	std::vector<uint16_t> val = { 0b00, 0b01, 0b10, 0b11 };
	for (auto v : val) {
		for (size_t i = 0; i < 8; ++i) {
			bfa.set(i, v);
			assert(bfa.get(i) == v);
			assert(bfa[i] == v);
		}
		for (size_t i = 0; i < 8; ++i) {
			bfa[i] = v;
			assert(bfa[i] == v);
		}
	}
	try {
		bfa.set(0, 0b100);
		assert(false);
	} catch (std::domain_error &e) {
		// ok
	}
	try {
		bfa.set(8, 0b00);
		assert(false);
	} catch (std::out_of_range &e) {
		// ok
	}
	return true;
}

void example();

int main() {
	bool ok = true;
	cout << "Starting tests..." << endl;
	ok &= test_bitfield();
	ok &= test_bitfield_array();
	if (ok) {
		cout << "All tests ok." << endl;
	} else {
		cout << "Test suite failed." << endl;
	}

	example();

	return !ok;
}

void example () {
    // on a microcontroller would be the address of a register
    uint32_t* reg_addr=new uint32_t;

    // simple BitfieldArray example
    using IDR=BitfieldArray<uint16_t, 0, 1,1>;
    IDR* idr(reinterpret_cast<IDR*>(reg_addr));
    idr->set(9, 0b0u); // set IDR bit 9 to 0

    // a more complex example
    union BSRR {
        BitfieldArray<uint32_t, 0, 1, 1> bs;
        BitfieldArray<uint32_t, 16, 1, 1> br;
    };
    BSRR* bsrr(reinterpret_cast<BSRR*>(reg_addr));
    bsrr->bs[5]=0b1u; // set BS bit 5 to 1
    bsrr->br[5]=0b1u; // set BS bit 5 to 1
}
