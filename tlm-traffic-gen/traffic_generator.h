#ifndef TRAFFIC_GENERATOR_H
#define TRAFFIC_GENERATOR_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <random>
#include <chrono>
#include "packet.h"

class TrafficGenerator : public sc_core::sc_module {
public:
    tlm_utils::simple_initiator_socket<TrafficGenerator> socket;

    // Configurable parameters
    uint32_t num_packets = 100;
    size_t min_payload_size = 64;
    size_t max_payload_size = 1500;

    SC_CTOR(TrafficGenerator) : socket("socket"), packet_count(0) {
        SC_THREAD(generate_traffic);
    }

    void configure(uint32_t n, size_t min_sz, size_t max_sz) {
        num_packets = n;
        min_payload_size = min_sz;
        max_payload_size = max_sz;
    }

private:
    uint32_t packet_count;
    std::mt19937 rng{std::random_device{}()};

    void generate_traffic() {
        // Wait for simulation to start
        wait(sc_core::SC_ZERO_TIME);

        for (uint32_t i = 0; i < num_packets; ++i) {
            // Random payload size
            std::uniform_int_distribution<size_t> size_dist(min_payload_size, max_payload_size);
            size_t payload_size = size_dist(rng);

            // Create packet
            Packet pkt(i + 1, sc_core::sc_time_stamp().value(), payload_size);

            // TLM transaction
            tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload();
            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

            trans->set_command(tlm::TLM_WRITE_COMMAND);
            trans->set_address(pkt.id);
            trans->set_data_ptr(reinterpret_cast<unsigned char*>(&pkt));
            trans->set_data_length(sizeof(Packet));
            trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            // Blocking transport
            socket->b_transport(*trans, delay);

            if (trans->get_response_status() == tlm::TLM_OK_RESPONSE) {
                packet_count++;
                if (packet_count % 10 == 0) {
                    SC_REPORT_INFO("[GEN]", ("Sent " + std::to_string(packet_count) + " packets").c_str());
                }
            }

            delete trans;
            wait(1, sc_core::SC_NS);
        }

        SC_REPORT_INFO("[GEN]", ("Done: " + std::to_string(packet_count) + " packets generated").c_str());
    }
};

#endif // TRAFFIC_GENERATOR_H
