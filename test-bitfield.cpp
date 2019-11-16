#include <register.hpp>
using namespace mcu;

#include <stdexcept>
#include <vector>
#include <cassert>
#include <iostream>
#include <cstddef>
using namespace std;

bool test_bitfield() {
	// those will fail to compile because they are invalid
	//Bit<uint16_t, 0, 18> bf1;
	//static_assert(false, "Bit size > type capacity.");
	//Bit<uint16_t, 18, 1> bf2;
	//static_assert(false, "Bit position > type capacity.");
	//Bit<uint16_t, 2, 0>bf3;
	//static_assert(false, "Bit size must be >0.");
	//Bit<uint16_t, 15, 2>bf4;
	//static_assert(false, "Bit position+size>type capacity.");

    uint16_t testval=0;
    uint16_t checkval=0;
	Bit<uint16_t, 0, 2> bf{&testval};
	std::vector<uint16_t> val = { 0b00, 0b01, 0b10, 0b11, 0b00 };
	for (auto v : val) {
		bf.set(v);
		assert(bf.get() == v);
		assert(bf.get_raw()==testval);
		checkval = v;
		assert(checkval==testval);
	}

    Bit<uint16_t, 5, 2> bf2{&testval};
    for (auto v : val) {
        bf2.set(v);
        assert(bf2.get() == v);
        assert(bf2.get_raw()==testval);
        checkval = (v<<5);
        assert(checkval==testval);
    }
	try {
		bf.set(0b100); // value too large
		assert(false);
	} catch (std::domain_error &e) {
		// ok
	}
	return true;
}

using MODE=BitArray<uint64_t, 0, 2, 4>;
using CNF=BitArray<uint64_t, 2, 2, 4>;
struct CR {
    CR()=delete;
    CR(uint64_t* const addr):mode{addr},cnf{addr}{}
    MODE mode;
    CNF cnf;
};

bool test_bitfield_array() {
	//BitArray<uint16_t, 0,0,1> bfa1;
	//static_assert(false, "Array len >0 failed.");
	//BitArray<uint16_t, 0,1,0> bfa2;
	//static_assert(false, "Array step >len failed.");
	//BitArray<uint16_t, 0,0,17> bfa3;
	//static_assert(false, "Array step >len failed.");
    uint16_t* const testval=new(uint16_t);
    uint16_t* const testval2=new(uint16_t);
	BitArray<uint16_t, 0, 16, 16> bf(testval);
	BitArray<uint16_t, 0, 2, 2> bfa(testval2);
	std::vector<uint16_t> val = { 0b00, 0b01, 0b10, 0b11, 0b00 };
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

	uint64_t* val3=new(uint64_t);
	CR cr(val3);
	for (auto v : val) {
        for (size_t i = 0; i < 16; ++i) {
            auto prev_cnf=cr.cnf.get(i);
            cr.mode.set(i,v);
            assert(cr.mode.get(i) == v);
            assert(cr.mode[i] == v);
            assert(cr.cnf.get(i)==prev_cnf);
            auto prev_mode=cr.mode.get(i);
            cr.cnf.set(i,v);
            assert(cr.cnf.get(i) == v);
            assert(cr.cnf[i] == v);
            assert(cr.mode.get(i)==prev_mode);

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
//    uint64_t m,val,v1,v2;
//    val=~0;
//    auto idx=8; auto step=4; auto pos0=0; auto len=2;
//    m=mask<uint64_t>(idx * step + pos0, len);
//    v1=val&(~mask<uint64_t>(idx * step + pos0, len));
//    v2=v1|(val << (idx * step + pos0));

	bool ok = true;
	cout << "Starting tests..." << endl;
	ok &= test_bitfield();
	ok &= test_bitfield_array();
	if (ok) {
		cout << "All tests ok." << endl;
	} else {
		cout << "Test suite failed." << endl;
	}

	//example();

	return !ok;
}

void example () {
    // on a microcontroller would be the address of a register
    uint32_t* reg_addr=new uint32_t;

    // simple BitArray example
    using IDR=BitArray<uint16_t, 0, 1,1>;
    IDR* idr(reinterpret_cast<IDR*>(reg_addr));
    idr->set(9, 0b0u); // set IDR bit 9 to 0

    // a more complex example
    union BSRR {
        BitArray<uint32_t, 0, 1, 1> bs;
        BitArray<uint32_t, 16, 1, 1> br;
    };
    BSRR* bsrr(reinterpret_cast<BSRR*>(reg_addr));
    bsrr->bs[5]=0b1u; // set BS bit 5 to 1
    bsrr->br[5]=0b1u; // set BS bit 5 to 1
}
