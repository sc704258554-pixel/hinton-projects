#include <systemc>
#include <iostream>
#include <iomanip>
#include <bitset>
#include "rr_arbiter.h"

int sc_main(int argc, char* argv[]) {
    std::cout << "====================================\n";
    std::cout << "   Round-Robin Arbiter Test\n";
    std::cout << "====================================\n\n";

    // Signals
    sc_core::sc_clock clk("clk", 10, sc_core::SC_NS);
    sc_core::sc_signal<bool> rst;
    sc_core::sc_signal<uint8_t> req;
    sc_core::sc_signal<uint8_t> grant;

    // Instantiate Arbiter
    RoundRobinArbiter arbiter("arbiter");
    
    arbiter.clk(clk);
    arbiter.rst(rst);
    arbiter.req(req);
    arbiter.grant(grant);

    // Initialize
    sc_core::sc_start(sc_core::SC_ZERO_TIME);

    // Reset
    std::cout << "[TB] Applying reset...\n";
    rst.write(true);
    sc_core::sc_start(20, sc_core::SC_NS);
    rst.write(false);
    sc_core::sc_start(10, sc_core::SC_NS);

    // Test 1: Single request
    std::cout << "\n[Test 1] Single request (req=0b0001)\n";
    req.write(0b0001);
    sc_core::sc_start(10, sc_core::SC_NS);
    std::cout << "  Grant: 0b" << std::bitset<4>(grant.read()) 
              << " (expected: 0b0001)\n";

    // Test 2: Multiple requests
    std::cout << "\n[Test 2] Multiple requests (req=0b0101)\n";
    req.write(0b0101);
    sc_core::sc_start(10, sc_core::SC_NS);
    uint8_t g1 = grant.read();
    std::cout << "  Grant 1: 0b" << std::bitset<4>(g1) << "\n";
    
    sc_core::sc_start(10, sc_core::SC_NS);
    uint8_t g2 = grant.read();
    std::cout << "  Grant 2: 0b" << std::bitset<4>(g2) << "\n";
    
    if (g1 != g2 && (g1 & g2) == 0) {
        std::cout << "  ✅ Round-robin working (different grants)\n";
    }

    // Test 3: All requests
    std::cout << "\n[Test 3] All requests (req=0b1111)\n";
    req.write(0b1111);
    uint8_t last_grant = 0;
    for (int i = 0; i < 4; ++i) {
        sc_core::sc_start(10, sc_core::SC_NS);
        uint8_t g = grant.read();
        std::cout << "  Round " << (i+1) << ": Grant 0b" << std::bitset<4>(g);
        if (g == last_grant) {
            std::cout << " ⚠️ Same as last";
        } else {
            std::cout << " ✅";
        }
        std::cout << "\n";
        last_grant = g;
    }

    // Test 4: No requests
    std::cout << "\n[Test 4] No requests (req=0b0000)\n";
    req.write(0b0000);
    sc_core::sc_start(10, sc_core::SC_NS);
    std::cout << "  Grant: 0b" << std::bitset<4>(grant.read()) 
              << " (expected: 0b0000)\n";

    std::cout << "\n====================================\n";
    std::cout << "   All tests completed!\n";
    std::cout << "====================================\n";

    return 0;
}
