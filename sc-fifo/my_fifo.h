#ifndef MY_FIFO_H
#define MY_FIFO_H

#include <systemc>
#include <queue>
#include <cstdint>

class ScFifo : public sc_core::sc_module {
public:
    sc_core::sc_in<bool> clk;
    sc_core::sc_in<bool> rst;
    
    sc_core::sc_in<bool> wr_en;
    sc_core::sc_in<uint32_t> wr_data;
    sc_core::sc_out<bool> full;
    
    sc_core::sc_in<bool> rd_en;
    sc_core::sc_out<uint32_t> rd_data;
    sc_core::sc_out<bool> empty;
    
    SC_CTOR(ScFifo) : m_depth(4), m_count(0) {
        SC_METHOD(process);
        sensitive << clk.pos();
    }
    
    size_t get_count() const { return m_count; }
    bool is_full() const { return m_count >= m_depth; }
    bool is_empty() const { return m_count == 0; }

private:
    std::queue<uint32_t> m_buffer;
    size_t m_depth;
    size_t m_count;
    
    void process() {
        if (rst.read()) {
            while (!m_buffer.empty()) m_buffer.pop();
            m_count = 0;
            full.write(false);
            empty.write(true);
            rd_data.write(0);
            return;
        }
        
        // Write
        if (wr_en.read() && !is_full()) {
            m_buffer.push(wr_data.read());
            m_count++;
        }
        
        // Read
        if (rd_en.read() && !is_empty()) {
            uint32_t data = m_buffer.front();
            m_buffer.pop();
            m_count--;
            rd_data.write(data);
        }
        
        // Update status
        full.write(is_full());
        empty.write(is_empty());
    }
};

#endif // MY_FIFO_H
