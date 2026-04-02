#ifndef EVENT_SCHEDULER_H
#define EVENT_SCHEDULER_H

#include <systemc>
#include <queue>
#include <vector>
#include "event.h"

class EventScheduler : public sc_core::sc_module {
public:
    sc_core::sc_event schedule_event;

    SC_CTOR(EventScheduler) : current_time(0) {
        SC_THREAD(schedule_events);
    }

    void schedule(const Event& event) {
        event_queue.push(event);
        SC_REPORT_INFO("[Scheduler]", ("Scheduled: " + event.name).c_str());
        // Don't notify during elaboration - will be picked up in next wait cycle
    }

    size_t pending_count() const { return event_queue.size(); }

private:
    std::priority_queue<Event> event_queue;
    uint64_t current_time;

    void schedule_events() {
        // Wait for simulation to start
        wait(sc_core::SC_ZERO_TIME);

        while (true) {
            if (event_queue.empty()) {
                wait(schedule_event);
            }

            while (!event_queue.empty()) {
                Event next = event_queue.top();
                uint64_t delay = next.trigger_time > current_time ?
                                 next.trigger_time - current_time : 0;

                if (delay > 0) {
                    wait(delay, sc_core::SC_NS);
                    current_time = next.trigger_time;
                }

                event_queue.pop();

                SC_REPORT_INFO("[Scheduler]", ("Triggered: " + next.name).c_str());

                // Small delay between events
                wait(1, sc_core::SC_NS);
                current_time += 1;
            }
        }
    }
};

#endif // EVENT_SCHEDULER_H
