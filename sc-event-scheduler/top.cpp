#include <systemc>
#include <iostream>
#include "event.h"
#include "event_scheduler.h"

int sc_main(int argc, char* argv[]) {
    std::cout << "====================================\n";
    std::cout << "   SystemC Event Scheduler v1.0\n";
    std::cout << "====================================\n\n";

    // Instantiate scheduler
    EventScheduler scheduler("scheduler");

    // Schedule multiple events with different times and priorities
    Event e1(1, "Task-A (HIGH)", 100, EventPriority::HIGH);
    Event e2(2, "Task-B (NORMAL)", 200, EventPriority::NORMAL);
    Event e3(3, "Task-C (LOW)", 150, EventPriority::LOW);
    Event e4(4, "Task-D (NORMAL)", 100, EventPriority::NORMAL);
    Event e5(5, "Task-E (HIGH)", 50, EventPriority::HIGH);

    // Schedule all events
    scheduler.schedule(e5);  // Earliest + HIGH
    scheduler.schedule(e1);  // 100ns + HIGH
    scheduler.schedule(e4);  // 100ns + NORMAL (after e1)
    scheduler.schedule(e3);  // 150ns + LOW
    scheduler.schedule(e2);  // 200ns + NORMAL

    std::cout << "[Main] Scheduled " << scheduler.pending_count() << " events\n\n";

    // Run simulation
    sc_core::sc_start();

    return 0;
}
