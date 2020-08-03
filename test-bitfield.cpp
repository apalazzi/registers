#include "register.hpp"
using namespace mcu;

#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace std;

bool test_bit() {
    // those will fail to compile because they have invalid
    // construction values uint16_t* bf_addr=new(uint16_t);
    // Bit<uint16_t, 0, 18> bf_assert_1(bf_addr);
    // Bit<uint16_t, 18, 1> bf_assert_2(bf_addr);
    // Bit<uint16_t, 2, 0>bf_assert_3(bf_addr);
    // Bit<uint16_t, 15, 2>bf_assert_4(bf_addr);

    uint16_t testval= 0;
    uint16_t checkval= 0;
    Bit<uint16_t, 0, 2> bf{&testval};
    std::vector<uint16_t> val= {0b00, 0b01, 0b10, 0b11,
                                0b00};
    for (auto v: val) {
        bf.set(v);
        assert(bf.get() == v);
        assert(bf.get_raw() == testval);
        checkval= v;
        assert(checkval == testval);
    }

    Bit<uint16_t, 5, 2> bf2{&testval};
    for (auto v: val) {
        bf2.set(v);
        assert(bf2.get() == v);
        assert(bf2.get_raw() == testval);
        checkval= (v << 5);
        assert(checkval == testval);
    }
    try {
        bf.set(0b100);  // value too large
        assert(false);
    } catch (std::domain_error& e) {
        // ok
    }
    return true;
}

using MODE= BitArray<uint32_t, 0, 2, 4>;
using CNF= BitArray<uint32_t, 2, 2, 4>;
struct CR {
    CR()= delete;
    CR(uint32_t* const addr): mode{addr}, cnf{addr} {}
    MODE mode;
    CNF cnf;
};

bool test_bitfield_array() {
    //  uint16_t* bf_addr=new(uint16_t);
    //	BitArray<uint16_t, 0,0,1> bfa_assert_1(bf_addr);
    //	BitArray<uint16_t, 0,2,1> bfa_assert_2(bf_addr);
    //	BitArray<uint16_t, 0,0,17> bfa_assert_3(bf_addr);

    uint16_t* const testval= new (uint16_t);
    uint16_t* const testval2= new (uint16_t);
    BitArray<uint16_t, 0, 16, 16> bf(testval);
    BitArray<uint16_t, 0, 2, 2> bfa(testval2);
    std::vector<uint16_t> val= {0b00, 0b01, 0b10, 0b11,
                                0b00};
    for (auto v: val) {
        for (auto i= 0; i < 8; ++i) {
            bfa.set(i, v);
            assert(bfa.get(i) == v);
            assert(bfa[i] == v);
        }
        for (auto i= 0; i < 8; ++i) {
            bfa[i]= v;
            assert(bfa[i] == v);
        }
    }

    addr_t* val3= new (addr_t);
    CR cr(val3);
    for (auto v: val) {
        for (auto i= 0; i < 8; ++i) {
            auto prev_cnf= cr.cnf.get(i);
            cr.mode.set(i, v);
            assert(cr.mode.get(i) == v);
            assert(cr.mode[i] == v);
            assert(cr.cnf.get(i) == prev_cnf);
            auto prev_mode= cr.mode.get(i);
            cr.cnf.set(i, v);
            assert(cr.cnf.get(i) == v);
            assert(cr.cnf[i] == v);
            assert(cr.mode.get(i) == prev_mode);
        }
        for (auto i= 0; i < 8; ++i) {
            bfa[i]= v;
            assert(bfa[i] == v);
        }
    }

    try {
        bfa.set(0, 0b100);
        assert(false);
    } catch (std::domain_error& e) {
        // ok
    }
    try {
        bfa.set(8, 0b00);
        assert(false);
    } catch (std::out_of_range& e) {
        // ok
    }

    delete testval;
    delete testval2;
    delete val3;
    return true;
}

void example();

int main() {
    bool ok= true;
    cout << "Starting tests..." << endl;
    try {
        ok&= test_bit();
        ok&= test_bitfield_array();
    } catch (exception& e) {
        ok= false;
        cout << "Caught unexpected exception: " << e.what()
             << endl;
    }
    if (ok) {
        cout << "All tests ok." << endl;
    } else {
        cout << "Test suite failed." << endl;
    }

    example();

    return !ok;
}

// Here below there are some more example usages
void example() {
    cout << "Running the example code..." << endl;
    // on a microcontroller would be the address of a
    // register
    uint16_t* const reg_16bit_addr= new uint16_t;

    // simple BitArray example
    using IDR= BitArray<uint16_t, 0, 1, 1>;
    IDR idr(reg_16bit_addr);
    idr.set(9, 0b0u);  // set IDR bit 9 to 0

    // a more complex example
    uint32_t* const reg_32bit_addr= new uint32_t;
    enum class idx_t { one= 1, two= 2, five= 5 };
    BitArray<uint32_t, 0, 1, 1, 32, idx_t> ec_example(reg_32bit_addr);
    ec_example[idx_t::one]=0;
    if (ec_example[idx_t::one]==0) {
        ec_example[idx_t::one]=true;
        int whatever=ec_example.get(idx_t::one);
        cout << whatever << endl;
        ec_example[idx_t::two]=ec_example[idx_t::five];
    }
    delete reg_16bit_addr;
    delete reg_32bit_addr;
    cout << "Done." << endl;
}
