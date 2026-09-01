# Multi-Format Digital Logic Simulator

A simple, C++ event-driven logic simulator that reads circuit netlists in **Verilog**, **VHDL**, or **EDIF** format, runs custom test vectors, and exports waveform traces for visualization in GTKWave.

---

## Features

* **Multi-Format Parser:** Loads circuit topologies from standard Verilog (`.v`), VHDL (`.vhd`), and EDIF (`.edf`) structural files.
* **Logic Gates Supported:** `AND`, `OR`, `XOR`, `NOT`, and edge-triggered `DFF` (D-Flip-Flop).
* **Race-Condition Protection:** D-Flip-Flops evaluate strictly on active clock rising edges to ensure correct sequential propagation in shift registers.
* **Dynamic Stimulus Loading:** Reads input test signals line-by-line from `stimulus.txt` without needing to recompile the C++ code.
* **Waveform Visualization:** Outputs industry-standard `.vcd` files compatible with GTKWave or VS Code extension viewers.

---

##  Requirements & Setup

* **Compiler:** `g++` with C++20 support (MSYS2 UCRT64 recommended on Windows).
* **Waveform Viewer:** [GTKWave](http://gtkwave.sourceforge.net/) (optional, for graphical signal inspection).

---

##  How to Run

### 1. Compile the Project

Open your MSYS2 / bash terminal in the project directory and build the simulator:

```bash
g++ -std=c++20 main.cpp -o logic_sim.exe

# Test with Verilog
./logic_sim.exe circuit.v stimulus.txt

# Test with VHDL
./logic_sim.exe circuit.vhd stimulus.txt

# Test with EDIF
./logic_sim.exe circuit.edf stimulus.txt

gtkwave output.vcd

LogicSimulator/
├── main.cpp          # Main engine, parsers, logic evaluator, and VCD logger
├── circuit.v         # Sample Verilog netlist
├── circuit.vhd       # Sample VHDL netlist
├── circuit.edf       # Sample EDIF netlist
├── stimulus.txt      # Input signal timing vectors
└── README.md         # Project documentation
