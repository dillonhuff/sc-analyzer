// Now: include matchlib, instantiate a component and
// then start to reconstruct channel structure?

// What is the goal?
// - Explore how to use functional coverage extension on real design
// - Get infrastructure for stimulus generation working
// - Add probes to monitor read and write times to channels?

// Once we have a record of read and write times do what?

// Separate issue: Try to find places in the design where a signal
// could be dropped? Need to look at the NVIDIA bug specs

#include <systemc.h>

SC_MODULE(Counter) {
 public:

  typedef sc_int<32> Data;

  Data data;
  
  sc_in_clk clk;
  sc_in<bool> rst;
  sc_out<Data> out;


  SC_CTOR(Counter) {
    sensitive << clk.pos();

    SC_THREAD(process);
  }

  void process() {

    data = 0;
    out.write(data);

    wait();
    
    while (1) {
      out.write(data);
      data = data + 1;
      wait();
    }
  }
};

int main() {
}
