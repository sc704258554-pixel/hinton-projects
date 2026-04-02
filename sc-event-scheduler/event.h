#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <cstdint>

enum class EventPriority { LOW, NORMAL, HIGH };

struct Event {
    uint32_t id;
    std::string name;
    uint64_t trigger_time;  // nanoseconds
    EventPriority priority;

    Event() : id(0), trigger_time(0), priority(EventPriority::NORMAL) {}

    Event(uint32_t id, const std::string& name, uint64_t time, EventPriority prio = EventPriority::NORMAL)
        : id(id), name(name), trigger_time(time), priority(prio) {}

    bool operator<(const Event& other) const {
        if (trigger_time != other.trigger_time) {
            return trigger_time > other.trigger_time;  // min-heap
        }
        return static_cast<int>(priority) < static_cast<int>(other.priority);
    }
};

#endif // EVENT_H
