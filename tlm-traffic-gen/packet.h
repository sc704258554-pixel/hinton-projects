#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <vector>
#include <string>

struct Packet {
    uint32_t id;
    uint64_t timestamp;
    std::vector<uint8_t> payload;

    Packet() : id(0), timestamp(0) {}

    Packet(uint32_t id, uint64_t ts, size_t size)
        : id(id), timestamp(ts), payload(size) {}

    size_t size() const { return payload.size(); }

    std::string summary() const {
        return "Pkt#" + std::to_string(id) + " [" + std::to_string(size()) + " bytes]";
    }
};

#endif // PACKET_H
