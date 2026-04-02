#include <systemc>
#include <iostream>
#include "my_fifo.h"

int sc_main(int argc, char* argv[]) {
    std::cout << "====================================\n";
    std::cout << "   SystemC FIFO Testbench v1.0\n";
    std::cout << "====================================\n\n";

    // Signals
    sc_core::sc_clock clk("clk", 10, sc_core::SC_NS);
    sc_core::sc_signal<bool> rst;
    sc_core::sc_signal<bool> wr_en;
    sc_core::sc_signal<uint32_t> wr_data;
    sc_core::sc_signal<bool> full;
    sc_core::sc_signal<bool> rd_en;
    sc_core::sc_signal<uint32_t> rd_data;
    sc_core::sc_signal<bool> empty;

    // Instantiate FIFO
    ScFifo fifo("fifo");
    
    fifo.clk(clk);
    fifo.rst(rst);
    fifo.wr_en(wr_en);
    fifo.wr_data(wr_data);
    fifo.full(full);
    fifo.rd_en(rd_en);
    fifo.rd_data(rd_data);
    fifo.empty(empty);

    // Test sequence
    sc_core::sc_start(sc_core::SC_ZERO_TIME);

    // Reset
    std::cout << "[TB] Applying reset...\n";
    rst.write(true);
    sc_core::sc_start(20, sc_core::SC_NS);
    rst.write(false);
    sc_core::sc_start(10, sc_core::SC_NS);

    // Write 4 items (fill FIFO)
    std::cout << "\n[TB] Writing 4 items...\n";
    for (uint32_t i = 1; i <= 4; ++i) {
        wr_en.write(true);
        wr_data.write(i * 100);
        sc_core::sc_start(10, sc_core::SC_NS);
        std::cout << "[TB] Wrote: " << i * 100 << " (count: " << fifo.get_count() << ")\n";
    }
    wr_en.write(false);

    // Check full flag
    if (full.read()) {
        std::cout << "[TB] FIFO is full (expected)\n";
    }

    // Read 4 items (empty FIFO)
    std::cout << "\n[TB] Reading 4 items...\n";
    for (uint32_t i = 1; i <= 4; ++i) {
        rd_en.write(true);
        sc_core::sc_start(10, sc_core::SC_NS);
        std::cout << "[TB] Read: " << rd_data.read() << " (count: " << fifo.get_count() << ")\n";
    }
    rd_en.write(false);

    // Check empty flag
    if (empty.read()) {
        std::cout << "[TB] FIFO is empty (expected)\n";
    }

    std::cout << "\n====================================\n";
    std::cout << "   Test completed successfully!\n";
    std::cout << "====================================\n";

    return 0;
}
