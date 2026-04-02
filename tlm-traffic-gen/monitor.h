#ifndef MONITOR_H
#define MONITOR_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <cstdint>
#include "packet.h"

class Monitor : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<Monitor> socket;

    // Statistics
    uint64_t total_packets;
    uint64_t total_bytes;
    sc_core::sc_time start_time;

    SC_CTOR(Monitor)
        : socket("socket"), total_packets(0), total_bytes(0), start_time(sc_core::SC_ZERO_TIME) {
        socket.register_b_transport(this, &Monitor::b_transport);
    }

    void report() const {
        double duration_sec = (sc_core::sc_time_stamp() - start_time).to_seconds();
        double throughput = (duration_sec > 0) ? (total_bytes / duration_sec / 1024.0) : 0.0;

        std::cout << "\n========== Monitor Report ==========\n";
        std::cout << "Total Packets : " << total_packets << "\n";
        std::cout << "Total Bytes   : " << total_bytes << "\n";
        std::cout << "Duration      : " << duration_sec << " s\n";
        std::cout << "Throughput    : " << throughput << " KB/s\n";
        std::cout << "====================================\n";
    }

private:
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
        if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
            if (total_packets == 0) {
                start_time = sc_core::sc_time_stamp();
            }

            Packet* pkt = reinterpret_cast<Packet*>(trans.get_data_ptr());
            total_packets++;
            total_bytes += pkt->size();

            if (total_packets % 10 == 0) {
                SC_REPORT_INFO("[MON]", pkt->summary().c_str());
            }

            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
        }
    }
};

#endif // MONITOR_H
