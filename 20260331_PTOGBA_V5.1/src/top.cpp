/**
 * @file top.cpp
 * @brief V5.1 SystemC 顶层模块（按算法/激励方案收敛版）
 * @author Hinton (Silver Maine Coon)
 * @date 2026-04-01
 */

#include <systemc>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <random>
#include <vector>

#include "sim_config.h"
#include "types.h"

using namespace sc_core;

SimulationStats g_stats;

namespace {

struct PacketTask {
    BarrierTask task;
};

inline std::ostream& operator<<(std::ostream& os, const PacketTask& pkt) {
    os << "PacketTask{task_id=" << pkt.task.task_id
       << ", N=" << pkt.task.member_count << "}";
    return os;
}

class SystemMonitor {
public:
    void record_latency(double latency) {
        g_stats.task_latencies.push_back(latency);
    }

    void mark_start(sc_time t) {
        if (g_stats.first_task_start == 0.0 || t.to_double() < g_stats.first_task_start) {
            g_stats.first_task_start = t.to_double();
        }
    }

    void mark_finish(sc_time t) {
        g_stats.last_task_complete = std::max(g_stats.last_task_complete, t.to_double());
    }

    void finalize() {
        std::sort(g_stats.task_latencies.begin(), g_stats.task_latencies.end());
        std::ofstream file("latency_cdf.csv");
        file << "latency_ns,cdf\n";
        for (size_t i = 0; i < g_stats.task_latencies.size(); ++i) {
            const double cdf = (i + 1) / static_cast<double>(g_stats.task_latencies.size());
            file << g_stats.task_latencies[i] << ',' << cdf << "\n";
        }
    }
};

class StimulusGen : public sc_module {
public:
    sc_fifo_out<PacketTask> task_out;

    SC_HAS_PROCESS(StimulusGen);

    StimulusGen(sc_module_name name, const std::vector<BarrierTask>& tasks)
        : sc_module(name), tasks_(tasks) {
        SC_THREAD(run);
    }

private:
    void run() {
        double last_start = 0.0;
        for (const auto& task : tasks_) {
            const double delta = std::max(0.0, task.start_time - last_start);
            wait(delta, SC_NS);
            task_out.write(PacketTask{task});
            last_start = task.start_time;
        }
    }

    const std::vector<BarrierTask>& tasks_;
};

class HostNIC : public sc_module {
public:
    sc_in<bool> clk;
    sc_fifo_in<PacketTask> task_in;
    sc_fifo_out<PacketTask> packet_out;
    sc_signal<int>& tx_depth;

    SC_HAS_PROCESS(HostNIC);

    HostNIC(sc_module_name name, sc_signal<int>& depth_signal)
        : sc_module(name), tx_depth(depth_signal) {
        SC_THREAD(send_packet);
    }

private:
    void send_packet() {
        while (true) {
            wait(clk.posedge_event());

            const int depth_before = task_in.num_available();
            tx_depth.write(depth_before);
            g_stats.max_tx_queue_depth = std::max(g_stats.max_tx_queue_depth, depth_before);

            if (depth_before > 0) {
                PacketTask pkt = task_in.read();
                wait(T_SERIALIZE_NS, SC_NS);
                packet_out.write(pkt);
            }

            tx_depth.write(task_in.num_available());
        }
    }
};

class Arbiter : public sc_module {
public:
    sc_in<bool> clk;
    sc_fifo_in<PacketTask> task_in;
    sc_fifo_out<PacketTask> task_out;
    sc_signal<int>& tx_depth;

    SC_HAS_PROCESS(Arbiter);

    Arbiter(sc_module_name name, sc_signal<int>& depth_signal)
        : sc_module(name), tx_depth(depth_signal) {
        SC_THREAD(arbitrate);
    }

private:
    void arbitrate() {
        while (true) {
            wait(task_in.data_written_event());

            while (task_in.num_available() > 0) {
                wait(clk.posedge_event());
                const int current_tx_depth = tx_depth.read();
                PacketTask wrapper = task_in.read();
                BarrierTask& task = wrapper.task;

                if (task.member_count >= GBA_THRESHOLD) {
                    task.path = PathType::GBA;
                } else if (current_tx_depth > QUEUE_THRESHOLD && task.member_count >= MC_THRESHOLD) {
                    task.path = PathType::MC;
                } else if (task.member_count >= MC_THRESHOLD) {
                    task.path = PathType::MC;
                } else {
                    task.path = PathType::P2P;
                }

                switch (task.path) {
                    case PathType::GBA:
                        ++g_stats.gba_count;
                        break;
                    case PathType::MC:
                        ++g_stats.mc_count;
                        break;
                    case PathType::P2P:
                        ++g_stats.p2p_count;
                        break;
                }

                task_out.write(wrapper);
            }
        }
    }
};

class SwitchRouter : public sc_module {
public:
    sc_fifo_in<PacketTask> packet_in;
    sc_fifo_out<PacketTask> gba_out;
    sc_fifo_out<PacketTask> mc_out;
    sc_fifo_out<PacketTask> p2p_out;

    SC_HAS_PROCESS(SwitchRouter);

    explicit SwitchRouter(sc_module_name name)
        : sc_module(name) {
        SC_THREAD(route);
    }

private:
    void route() {
        while (true) {
            wait(packet_in.data_written_event());
            while (packet_in.num_available() > 0) {
                PacketTask wrapper = packet_in.read();
                switch (wrapper.task.path) {
                    case PathType::GBA:
                        gba_out.write(wrapper);
                        break;
                    case PathType::MC:
                        mc_out.write(wrapper);
                        break;
                    case PathType::P2P:
                        p2p_out.write(wrapper);
                        break;
                }
            }
        }
    }
};

class GBAEngine : public sc_module {
public:
    sc_fifo_in<PacketTask> packet_in;

    SC_HAS_PROCESS(GBAEngine);

    GBAEngine(sc_module_name name, SystemMonitor& monitor)
        : sc_module(name), monitor_(monitor) {
        for (int i = 0; i < GBA_TOTAL_SLOTS; ++i) {
            free_slots_.push(i);
        }
        SC_THREAD(dispatcher);
        SC_THREAD(slot_release_worker);
    }

private:
    struct ActiveTask {
        PacketTask pkt;
        int slot_id;
        sc_time release_time;
    };

    struct WaitingCompare {
        bool operator()(const PacketTask& a, const PacketTask& b) const {
            if (a.task.member_count != b.task.member_count) {
                return a.task.member_count < b.task.member_count;
            }
            return a.task.start_time > b.task.start_time;
        }
    };

    struct ActiveCompare {
        bool operator()(const ActiveTask& a, const ActiveTask& b) const {
            return a.release_time > b.release_time;
        }
    };

    void dispatcher() {
        while (true) {
            if (packet_in.num_available() == 0 && waiting_packets_.empty()) {
                wait(packet_in.data_written_event() | slot_release_event_);
            }

            while (packet_in.num_available() > 0) {
                waiting_packets_.push(packet_in.read());
            }

            while (!waiting_packets_.empty() && !free_slots_.empty()) {
                PacketTask next = waiting_packets_.top();
                waiting_packets_.pop();

                const sc_time start = sc_time_stamp();
                monitor_.mark_start(start);

                int slot_id = free_slots_.front();
                free_slots_.pop();
                ++active_slots_;
                g_stats.gba_slots_peak = std::max(g_stats.gba_slots_peak, active_slots_);

                const sc_time release_time = start
                    + sc_time(T_LINK_OWD_NS + T_PIPELINE_NS + T_GBA_PROC_NS + static_cast<int>(next.task.tail_latency) + T_LINK_OWD_NS, SC_NS);

                active_tasks_.push(ActiveTask{next, slot_id, release_time});
                slot_release_event_.notify(release_time - sc_time_stamp());
            }
        }
    }

    void slot_release_worker() {
        while (true) {
            wait(slot_release_event_);
            const sc_time now = sc_time_stamp();

            while (!active_tasks_.empty() && active_tasks_.top().release_time <= now) {
                ActiveTask done = active_tasks_.top();
                active_tasks_.pop();

                free_slots_.push(done.slot_id);
                --active_slots_;
                monitor_.record_latency((done.release_time - (done.release_time - sc_time(T_LINK_OWD_NS + T_PIPELINE_NS + T_GBA_PROC_NS + static_cast<int>(done.pkt.task.tail_latency) + T_LINK_OWD_NS, SC_NS))).to_double());
                monitor_.mark_finish(done.release_time);
            }

            if (!active_tasks_.empty()) {
                const sc_time delay = active_tasks_.top().release_time - sc_time_stamp();
                if (delay > SC_ZERO_TIME) {
                    slot_release_event_.notify(delay);
                } else {
                    slot_release_event_.notify(SC_ZERO_TIME);
                }
            }
        }
    }

    SystemMonitor& monitor_;
    std::queue<int> free_slots_;
    std::priority_queue<PacketTask, std::vector<PacketTask>, WaitingCompare> waiting_packets_;
    std::priority_queue<ActiveTask, std::vector<ActiveTask>, ActiveCompare> active_tasks_;
    sc_event slot_release_event_;
    int active_slots_ = 0;
};

class MCEngine : public sc_module {
public:
    sc_fifo_in<PacketTask> packet_in;

    SC_HAS_PROCESS(MCEngine);

    MCEngine(sc_module_name name, SystemMonitor& monitor)
        : sc_module(name), monitor_(monitor) {
        SC_THREAD(process_packet);
    }

private:
    void process_packet() {
        while (true) {
            wait(packet_in.data_written_event());
            while (packet_in.num_available() > 0) {
                PacketTask wrapper = packet_in.read();
                const sc_time start = sc_time_stamp();
                monitor_.mark_start(start);

                outstanding_ += wrapper.task.member_count;
                g_stats.mc_outstanding_peak = std::max(g_stats.mc_outstanding_peak, outstanding_);

                wait(T_LINK_OWD_NS, SC_NS);
                wait(T_PIPELINE_NS, SC_NS);
                wait(T_HOST_STACK_NS, SC_NS);
                wait(wrapper.task.member_count * T_MC_DUP_NS, SC_NS);
                wait(T_PIPELINE_NS, SC_NS);
                wait(T_LINK_OWD_NS, SC_NS);

                outstanding_ -= wrapper.task.member_count;
                monitor_.record_latency((sc_time_stamp() - start).to_double());
                monitor_.mark_finish(sc_time_stamp());
            }
        }
    }

    SystemMonitor& monitor_;
    int outstanding_ = 0;
};

class P2PEngine : public sc_module {
public:
    sc_fifo_in<PacketTask> packet_in;

    SC_HAS_PROCESS(P2PEngine);

    P2PEngine(sc_module_name name, SystemMonitor& monitor)
        : sc_module(name), monitor_(monitor) {
        SC_THREAD(process_packet);
    }

private:
    void process_packet() {
        while (true) {
            wait(packet_in.data_written_event());
            while (packet_in.num_available() > 0) {
                PacketTask wrapper = packet_in.read();
                const sc_time start = sc_time_stamp();
                monitor_.mark_start(start);

                wait(T_LINK_OWD_NS, SC_NS);
                wait(T_PIPELINE_NS, SC_NS);
                wait(T_HOST_STACK_NS, SC_NS);
                wait(std::max(1, wrapper.task.member_count - 1) * T_SERIALIZE_NS, SC_NS);
                wait(T_PIPELINE_NS, SC_NS);
                wait(T_LINK_OWD_NS, SC_NS);

                monitor_.record_latency((sc_time_stamp() - start).to_double());
                monitor_.mark_finish(sc_time_stamp());
            }
        }
    }

    SystemMonitor& monitor_;
};

int generate_zipf_n(std::mt19937& rng, double alpha, int min_n, int max_n) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    while (true) {
        const int k = min_n + static_cast<int>(rng() % (max_n - min_n + 1));
        const double u = dist(rng);
        const double pk = 1.0 / std::pow(static_cast<double>(k), alpha);
        if (u <= pk) {
            return k;
        }
    }
}

std::vector<BarrierTask> build_tasks(std::mt19937& rng) {
    std::uniform_real_distribution<double> arrival_jitter(0.0, 20.0);
    std::uniform_real_distribution<double> tail_jitter(0.0, 5000.0);

    std::vector<BarrierTask> tasks;
    tasks.reserve(TOTAL_TASKS);
    int task_id = 0;

    for (int i = 0; i < LARGE_GROUPS; ++i) {
        BarrierTask task{};
        task.task_id = task_id++;
        task.member_count = generate_zipf_n(rng, 1.0, 60, 100);
        task.state = TaskState::PENDING;
        task.start_time = arrival_jitter(rng);
        task.tail_latency = tail_jitter(rng);
        task.priority = task.member_count;
        tasks.push_back(task);
    }

    for (int i = 0; i < SMALL_GROUPS; ++i) {
        BarrierTask task{};
        task.task_id = task_id++;
        task.member_count = generate_zipf_n(rng, 1.0, 2, 10);
        task.state = TaskState::PENDING;
        task.start_time = arrival_jitter(rng);
        task.tail_latency = tail_jitter(rng);
        task.priority = task.member_count;
        tasks.push_back(task);
    }

    BarrierTask bad_apple{};
    bad_apple.task_id = task_id++;
    bad_apple.member_count = 2;
    bad_apple.state = TaskState::PENDING;
    bad_apple.start_time = arrival_jitter(rng);
    bad_apple.tail_latency = TAIL_LATENCY_BAD_APPLE_NS;
    bad_apple.priority = bad_apple.member_count;
    tasks.push_back(bad_apple);

    std::sort(tasks.begin(), tasks.end(), [](const BarrierTask& a, const BarrierTask& b) {
        if (a.start_time != b.start_time) {
            return a.start_time < b.start_time;
        }
        return a.task_id < b.task_id;
    });

    return tasks;
}

class Top : public sc_module {
public:
    sc_clock clk;
    sc_fifo<PacketTask> task_queue;
    sc_fifo<PacketTask> nic_queue;
    sc_fifo<PacketTask> gba_queue;
    sc_fifo<PacketTask> mc_queue;
    sc_fifo<PacketTask> p2p_queue;
    sc_fifo<PacketTask> task_queue_from_nic;
    sc_signal<int> tx_depth;

    StimulusGen stim;
    Arbiter arbiter;
    HostNIC nic;
    SwitchRouter router;
    SystemMonitor monitor;
    GBAEngine gba;
    MCEngine mc;
    P2PEngine p2p;

    Top(sc_module_name name, const std::vector<BarrierTask>& tasks)
        : sc_module(name),
          clk("clk", 1, SC_NS),
          task_queue("task_queue", TOTAL_TASKS),
          nic_queue("nic_queue", TOTAL_TASKS),
          gba_queue("gba_queue", TOTAL_TASKS),
          mc_queue("mc_queue", TOTAL_TASKS),
          p2p_queue("p2p_queue", TOTAL_TASKS),
          task_queue_from_nic("task_queue_from_nic", TOTAL_TASKS),
          stim("stim", tasks),
          arbiter("arbiter", tx_depth),
          nic("nic", tx_depth),
          router("router"),
          gba("gba", monitor),
          mc("mc", monitor),
          p2p("p2p", monitor) {
        stim.task_out(task_queue);

        arbiter.clk(clk);
        arbiter.task_in(task_queue);
        arbiter.task_out(nic_queue);

        nic.clk(clk);
        nic.task_in(nic_queue);
        nic.packet_out(task_queue_from_nic);

        router.packet_in(task_queue_from_nic);
        router.gba_out(gba_queue);
        router.mc_out(mc_queue);
        router.p2p_out(p2p_queue);

        gba.packet_in(gba_queue);
        mc.packet_in(mc_queue);
        p2p.packet_in(p2p_queue);
    }
};

}  // namespace

int sc_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::mt19937 rng(42);
    const auto tasks = build_tasks(rng);
    Top top("top", tasks);

    sc_start(10, SC_MS);

    if (g_stats.task_latencies.empty()) {
        std::cerr << "仿真失败：未产生任何时延样本\n";
        return 1;
    }

    top.monitor.finalize();

    const size_t p99_idx = std::min(g_stats.task_latencies.size() - 1,
                                    static_cast<size_t>(g_stats.task_latencies.size() * 0.99));
    const size_t p999_idx = std::min(g_stats.task_latencies.size() - 1,
                                     static_cast<size_t>(g_stats.task_latencies.size() * 0.999));
    const double jct = g_stats.last_task_complete - g_stats.first_task_start;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "samples: " << g_stats.task_latencies.size() << "\n";
    std::cout << "JCT: " << jct << " ns\n";
    std::cout << "P99: " << g_stats.task_latencies[p99_idx] << " ns\n";
    std::cout << "P99.9: " << g_stats.task_latencies[p999_idx] << " ns\n";
    std::cout << "Max TX Queue Depth: " << g_stats.max_tx_queue_depth << "\n";
    std::cout << "GBA: " << g_stats.gba_count << ", MC: " << g_stats.mc_count
              << ", P2P: " << g_stats.p2p_count << "\n";
    std::cout << "GBA Slots Peak: " << g_stats.gba_slots_peak
              << ", MC Outstanding Peak: " << g_stats.mc_outstanding_peak << "\n";

    return 0;
}
