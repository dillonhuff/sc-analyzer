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

  sc_in_clk clk;
  
  typedef sc_int<32> Data;

  SC_CTOR(Counter) {
    sensitive << clk.pos();

    SC_THREAD(process);
  }

  void process() {
    while (1) {
      wait();
    }
  }
};

int main() {
}
