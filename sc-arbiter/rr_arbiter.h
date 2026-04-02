#ifndef RR_ARBITER_H
#define RR_ARBITER_H

#include <systemc>
#include <cstdint>

class RoundRobinArbiter : public sc_core::sc_module {
public:
    sc_core::sc_in<bool> clk;
    sc_core::sc_in<bool> rst;
    sc_core::sc_in<uint8_t> req;      // 4-bit request vector
    sc_core::sc_out<uint8_t> grant;   // 4-bit grant vector (one-hot)
    
    SC_CTOR(RoundRobinArbiter) : m_last_grant(0), m_priority_ptr(0) {
        SC_METHOD(arbitrate);
        sensitive << clk.pos();
    }
    
    uint8_t get_last_grant() const { return m_last_grant; }

private:
    uint8_t m_last_grant;    // 上次授权的请求
    uint8_t m_priority_ptr;  // 优先级指针
    
    void arbitrate() {
        if (rst.read()) {
            m_last_grant = 0;
            m_priority_ptr = 0;
            grant.write(0);
            return;
        }
        
        uint8_t req_val = req.read();
        if (req_val == 0) {
            grant.write(0);
            return;
        }
        
        // Round-robin: 从 priority_ptr 开始搜索
        for (int i = 0; i < 4; ++i) {
            int idx = (m_priority_ptr + i) % 4;
            if (req_val & (1 << idx)) {
                uint8_t g = (1 << idx);
                grant.write(g);
                m_last_grant = idx;
                m_priority_ptr = (idx + 1) % 4;  // 下次从这个之后开始
                return;
            }
        }
        
        grant.write(0);
    }
};

#endif // RR_ARBITER_H
