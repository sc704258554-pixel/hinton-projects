#include <systemc>
#include <iostream>
#include "traffic_generator.h"
#include "monitor.h"

int sc_main(int argc, char* argv[]) {
    // Parse arguments
    uint32_t num_packets = 100;
    if (argc > 1) {
        num_packets = std::stoul(argv[1]);
    }

    std::cout << "====================================\n";
    std::cout << "   TLM Traffic Generator v1.0\n";
    std::cout << "   Packets: " << num_packets << "\n";
    std::cout << "====================================\n\n";

    // Instantiate modules
    TrafficGenerator generator("generator");
    Monitor monitor("monitor");

    // Connect
    generator.socket.bind(monitor.socket);

    // Configure generator
    generator.configure(num_packets, 64, 1500);

    // Run simulation
    sc_core::sc_start();

    // Final report
    monitor.report();

    return 0;
}
