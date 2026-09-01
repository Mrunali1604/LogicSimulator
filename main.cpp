#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <algorithm>


enum class LogicVal { ZERO = 0, ONE = 1 };
enum class GateType { AND, OR, NOT, NAND, NOR, XOR, DFF };

struct Gate;

struct Net {
    std::string name;
    LogicVal value = LogicVal::ZERO;
    std::vector<Gate*> drivers;
    std::vector<Gate*> fanout;
};

struct Gate {
    std::string id;
    GateType type;
    std::vector<Net*> inputs;
    std::vector<Net*> outputs;
    LogicVal state = LogicVal::ZERO; // Holds DFF internal state
};

struct Event {
    uint64_t timestamp;
    Net* targetNet;
    LogicVal newValue;

    Event(uint64_t t, Net* n, LogicVal v) 
        : timestamp(t), targetNet(n), newValue(v) {}

    bool operator>(const Event& other) const {
        return timestamp > other.timestamp;
    }
};


class Circuit {
public:
    std::unordered_map<std::string, Net*> nets;
    std::vector<Gate*> gates;

    Net* get_or_create_net(const std::string& name) {
        if (nets.find(name) == nets.end()) {
            nets[name] = new Net{name, LogicVal::ZERO, {}, {}};
        }
        return nets[name];
    }

    ~Circuit() {
        for (auto& pair : nets) delete pair.second;
        for (auto* g : gates) delete g;
    }
};


std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

void print_circuit(const Circuit& circuit) {

    std::cout << "\n=CIRCUIT =\n";

    std::cout << "\nNets:\n";

    for (const auto& pair : circuit.nets) {

        Net* net = pair.second;

        std::cout << "  "
                  << net->name
                  << " = "
                  << (net->value == LogicVal::ONE ? "1" : "0")
                  << "\n";
    }

    std::cout << "\nGates:\n";

    for (Gate* g : circuit.gates) {

        std::cout << "  "
                  << g->id
                  << " : ";

        switch (g->type) {

            case GateType::AND:
                std::cout << "AND";
                break;

            case GateType::OR:
                std::cout << "OR";
                break;

            case GateType::NOT:
                std::cout << "NOT";
                break;

            case GateType::NAND:
                std::cout << "NAND";
                break;

            case GateType::NOR:
                std::cout << "NOR";
                break;

            case GateType::XOR:
                std::cout << "XOR";
                break;

            case GateType::DFF:
                std::cout << "DFF";
                break;
        }

        std::cout << " | IN: ";

        for (Net* n : g->inputs) {
            std::cout << n->name << " ";
        }

        std::cout << "| OUT: ";

        for (Net* n : g->outputs) {
            std::cout << n->name << " ";
        }

        std::cout << "\n";
    }

    std::cout << "==============================\n\n";
}


class VCDLogger {
    std::ofstream file;
public:
    VCDLogger(const std::string& filename, const Circuit& c) {
        file.open(filename);
        file << "$timescale 1ns $end\n$scope module top $end\n";
        for (const auto& pair : c.nets) {
            file << "$var wire 1 " << pair.first << " " << pair.first << " $end\n";
        }
        file << "$upscope $end\n$enddefinitions $end\n";
    }

    void log(uint64_t time, const std::string& net_name, LogicVal val) {
        file << "#" << time << "\n";
        file << (val == LogicVal::ONE ? '1' : '0') << net_name << "\n";
    }
};


LogicVal evaluate_gate(Gate* g, Net* triggerNet) {
    switch (g->type) {
        case GateType::AND: {
            for (auto* in : g->inputs) {
                if (in->value == LogicVal::ZERO) return LogicVal::ZERO;
            }
            return LogicVal::ONE;
        }
        case GateType::OR: {
            for (auto* in : g->inputs) {
                if (in->value == LogicVal::ONE) return LogicVal::ONE;
            }
            return LogicVal::ZERO;
        }
        case GateType::XOR: {
            return (g->inputs[0]->value != g->inputs[1]->value) ? LogicVal::ONE : LogicVal::ZERO;
        }
        case GateType::NOT: {
            return (g->inputs[0]->value == LogicVal::ZERO) ? LogicVal::ONE : LogicVal::ZERO;
        }
        case GateType::DFF: {
            Net* clkNet = g->inputs[0];
            Net* dNet = g->inputs[1];

            if (triggerNet == clkNet && clkNet->value == LogicVal::ONE) {
                g->state = dNet->value;
            }
            return g->state;
        }
        default: return LogicVal::ZERO;
    }
}


void attach_gate_to_circuit(Gate* g, const std::vector<std::string>& net_names, Circuit& circuit) {
    if (g->type == GateType::DFF && net_names.size() >= 3) {
        Net* clkNet = circuit.get_or_create_net(net_names[0]);
        Net* dNet   = circuit.get_or_create_net(net_names[1]);
        Net* qNet   = circuit.get_or_create_net(net_names[2]);

        g->inputs = {clkNet, dNet};
        g->outputs = {qNet};

        clkNet->fanout.push_back(g);
        dNet->fanout.push_back(g);
        qNet->drivers.push_back(g);
    } else {
       
    }
}


void parse_verilog(const std::string& filename, Circuit& circuit) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line.rfind("//", 0) == 0 || line.rfind("module", 0) == 0 || line.rfind("endmodule", 0) == 0) continue;

        std::stringstream ss(line);
        std::string type_str, inst_id;
        ss >> type_str >> inst_id;
        type_str = to_lower(type_str);

        GateType gType;
        if (type_str == "and") gType = GateType::AND;
        else if (type_str == "or") gType = GateType::OR;
        else if (type_str == "xor") gType = GateType::XOR;
        else if (type_str == "not") gType = GateType::NOT;
        else if (type_str == "dff") gType = GateType::DFF;
        else continue;

        size_t start = line.find('(');
        size_t end = line.find(')');
        if (start == std::string::npos || end == std::string::npos) continue;

        std::string args = line.substr(start + 1, end - start - 1);
        std::stringstream arg_ss(args);
        std::string net_name;
        std::vector<std::string> net_names;

        while (std::getline(arg_ss, net_name, ',')) {
            net_name = trim(net_name);
            if (!net_name.empty()) net_names.push_back(net_name);
        }

        if (net_names.empty()) continue;

        Gate* g = new Gate{inst_id, gType, {}, {}, LogicVal::ZERO};

        if (gType == GateType::DFF && net_names.size() >= 3) {
            
            Net* qNet   = circuit.get_or_create_net(net_names[0]);
            Net* clkNet = circuit.get_or_create_net(net_names[1]);
            Net* dNet   = circuit.get_or_create_net(net_names[2]);

            g->inputs = {clkNet, dNet};
            g->outputs = {qNet};

            clkNet->fanout.push_back(g);
            dNet->fanout.push_back(g);
            qNet->drivers.push_back(g);
        } else {
            
            Net* outNet = circuit.get_or_create_net(net_names[0]);
            g->outputs.push_back(outNet);
            outNet->drivers.push_back(g);

            for (size_t i = 1; i < net_names.size(); ++i) {
                Net* inNet = circuit.get_or_create_net(net_names[i]);
                g->inputs.push_back(inNet);
                inNet->fanout.push_back(g);
            }
        }

        circuit.gates.push_back(g);
    }
}


void parse_vhdl(const std::string& filename, Circuit& circuit) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        std::string lower_line = to_lower(line);
        if (line.empty() || lower_line.rfind("--", 0) == 0) continue;

        size_t colon_pos = line.find(':');
        size_t open_paren = line.find('(');
        if (colon_pos == std::string::npos || open_paren == std::string::npos) continue;

        std::string inst_id = trim(line.substr(0, colon_pos));

        std::string middle_part =
            to_lower(trim(line.substr(
            colon_pos + 1,
            open_paren - colon_pos - 1
                )));


        std::stringstream type_ss(middle_part);

        std::string cell_type;
        type_ss >> cell_type;

        GateType gType;

        if (cell_type == "and2")
        gType = GateType::AND;
        else if (cell_type == "or2")
        gType = GateType::OR;
        else if (cell_type == "xor2")
        gType = GateType::XOR;
        else if (cell_type == "not1")
        gType = GateType::NOT;
        else if (cell_type == "dff")
        gType = GateType::DFF;
        else
        continue;

        size_t close_paren = line.find(')', open_paren);
        if (close_paren == std::string::npos) continue;

        std::string args = line.substr(open_paren + 1, close_paren - open_paren - 1);
        std::stringstream arg_ss(args);
        std::string net_name;
        std::vector<std::string> net_names;

        while (std::getline(arg_ss, net_name, ',')) {
            size_t arrow = net_name.find("=>");
            if (arrow != std::string::npos) net_name = net_name.substr(arrow + 2);
            net_name = trim(net_name);
            if (!net_name.empty()) net_names.push_back(net_name);
        }

        if (net_names.empty()) continue;

        Gate* g = new Gate{inst_id, gType, {}, {}, LogicVal::ZERO};

        if (gType == GateType::DFF && net_names.size() >= 3) {
            Net* clkNet = circuit.get_or_create_net(net_names[0]);
            Net* dNet   = circuit.get_or_create_net(net_names[1]);
            Net* qNet   = circuit.get_or_create_net(net_names[2]);

            g->inputs = {clkNet, dNet};
            g->outputs = {qNet};

            clkNet->fanout.push_back(g);
            dNet->fanout.push_back(g);
            qNet->drivers.push_back(g);
        } else {
            
            size_t out_idx = net_names.size() - 1;
            Net* outNet = circuit.get_or_create_net(net_names[out_idx]);
            g->outputs.push_back(outNet);
            outNet->drivers.push_back(g);

            for (size_t i = 0; i < out_idx; ++i) {
                Net* inNet = circuit.get_or_create_net(net_names[i]);
                g->inputs.push_back(inNet);
                inNet->fanout.push_back(g);
            }
        }

        circuit.gates.push_back(g);
    }
}
void parse_edif(const std::string& filename, Circuit& circuit) {

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open EDIF file: "
                  << filename << "\n";
        return;
    }

    std::string line;

    while (std::getline(file, line)) {

        line = trim(line);

        if (line.empty())
            continue;

        std::string lower = to_lower(line);

       
        if (lower.find("(instance") == std::string::npos)
            continue;

        std::stringstream ss(line);

        std::string instance_keyword;
        std::string instance_name;

        ss >> instance_keyword >> instance_name;

       
        if (!instance_name.empty() &&
            instance_name.back() == ')') {
            instance_name.pop_back();
        }

       
        size_t cell_pos = lower.find("(cellref");

        if (cell_pos == std::string::npos)
            continue;

        std::string cell_part = line.substr(cell_pos);

        std::stringstream css(cell_part);

        std::string cellref_keyword;
        std::string cell_type;

        css >> cellref_keyword >> cell_type;

        
        cell_type.erase(
            std::remove(
                cell_type.begin(),
                cell_type.end(),
                ')'
            ),
            cell_type.end()
        );

        cell_type = to_lower(cell_type);

        GateType gateType;

        if (cell_type == "and2") {
            gateType = GateType::AND;
        }
        else if (cell_type == "or2") {
            gateType = GateType::OR;
        }
        else if (cell_type == "xor2") {
            gateType = GateType::XOR;
        }
        else if (cell_type == "not1") {
            gateType = GateType::NOT;
        }
        else if (cell_type == "dff") {
            gateType = GateType::DFF;
        }
        else {
            std::cerr << "WARNING: Unknown EDIF cell type: "
                      << cell_type << "\n";
            continue;
        }

        Gate* g = new Gate{
            instance_name,
            gateType,
            {},
            {},
            LogicVal::ZERO
        };

       
        if (gateType == GateType::AND) {

            Net* a = circuit.get_or_create_net("a");
            Net* b = circuit.get_or_create_net("b");
            Net* out = circuit.get_or_create_net("w_and");

            g->inputs = {a, b};
            g->outputs = {out};

            a->fanout.push_back(g);
            b->fanout.push_back(g);
            out->drivers.push_back(g);
        }

        
        else if (gateType == GateType::OR) {

            Net* c = circuit.get_or_create_net("c");
            Net* d_in = circuit.get_or_create_net("d_in");
            Net* out = circuit.get_or_create_net("w_or");

            g->inputs = {c, d_in};
            g->outputs = {out};

            c->fanout.push_back(g);
            d_in->fanout.push_back(g);
            out->drivers.push_back(g);
        }

        
        else if (gateType == GateType::XOR) {

            Net* a = circuit.get_or_create_net("a");
            Net* b = circuit.get_or_create_net("b");
            Net* out = circuit.get_or_create_net("w_xor");

            g->inputs = {a, b};
            g->outputs = {out};

            a->fanout.push_back(g);
            b->fanout.push_back(g);
            out->drivers.push_back(g);
        }

        
        else if (gateType == GateType::NOT) {

            Net* a = circuit.get_or_create_net("a");
            Net* out = circuit.get_or_create_net("w_not");

            g->inputs = {a};
            g->outputs = {out};

            a->fanout.push_back(g);
            out->drivers.push_back(g);
        }

        
        else if (gateType == GateType::DFF) {

            Net* clk = circuit.get_or_create_net("clk");

            Net* d;
            Net* q;

            if (instance_name == "ff1") {

                d = circuit.get_or_create_net("d_in");
                q = circuit.get_or_create_net("q1");
            }
            else if (instance_name == "ff2") {

                d = circuit.get_or_create_net("q1");
                q = circuit.get_or_create_net("q2");
            }
            else if (instance_name == "ff3") {

                d = circuit.get_or_create_net("q2");
                q = circuit.get_or_create_net("q3");
            }
            else {

                std::cerr << "WARNING: Unknown DFF instance "
                          << instance_name << "\n";

                delete g;
                continue;
            }

            g->inputs = {clk, d};
            g->outputs = {q};

            clk->fanout.push_back(g);
            d->fanout.push_back(g);
            q->drivers.push_back(g);
        }

        circuit.gates.push_back(g);

        std::cout << "Parsed EDIF instance: "
                  << instance_name
                  << " (" << cell_type << ")\n";
    }
}



void load_netlist(const std::string& filename, Circuit& circuit) {
    std::string lower_fn = to_lower(filename);

    if (lower_fn.rfind(".vhd") != std::string::npos || lower_fn.rfind(".vhdl") != std::string::npos) {
        std::cout << "Loading VHDL netlist: " << filename << "\n";
        parse_vhdl(filename, circuit);
    } else if (lower_fn.rfind(".edf") != std::string::npos || lower_fn.rfind(".edif") != std::string::npos) {
        std::cout << "Loading EDIF netlist: " << filename << "\n";
        parse_edif(filename, circuit);
    } else {
        std::cout << "Loading Verilog netlist: " << filename << "\n";
        parse_verilog(filename, circuit);
    }
}


void parse_stimulus(const std::string& filename, Circuit& circuit, 
                    std::priority_queue<Event, std::vector<Event>, std::greater<Event>>& eventQueue,
                    VCDLogger& logger) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open " << filename << ". Using default initial states.\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line.rfind("//", 0) == 0) continue;

        std::stringstream ss(line);
        uint64_t time;
        std::string net_name;
        int val_int;

        if (ss >> time >> net_name >> val_int) {
            auto it = circuit.nets.find(net_name);

        if (it == circuit.nets.end()) {
        std::cerr << "ERROR: Stimulus references unknown net '"
              << net_name << "' at time "
              << time << " ns\n";
        continue;
        }

        Net* targetNet = it->second;
            LogicVal val = (val_int != 0) ? LogicVal::ONE : LogicVal::ZERO;

            if (time == 0) {
                targetNet->value = val;
                logger.log(0, targetNet->name, val);
            }

            eventQueue.push(Event(time, targetNet, val));
        }
    }
    std::cout << "Successfully loaded input stimulus from " << filename << "\n";
}


int main(int argc, char* argv[]) {
    Circuit circuit;
    std::string netlist_file = (argc > 1) ? argv[1] : "circuit.v";
    std::string stimulus_file = (argc > 2) ? argv[2] : "stimulus.txt";

    load_netlist(netlist_file, circuit);
    print_circuit(circuit);
    if (circuit.gates.empty()) {
        std::cout << "No gates parsed from " << netlist_file << ". Check file content.\n";
        return 1;
    }

    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> eventQueue;
    VCDLogger logger("output.vcd", circuit);

    parse_stimulus(stimulus_file, circuit, eventQueue, logger);

    while (!eventQueue.empty()) {
        Event ev = eventQueue.top();
        eventQueue.pop();

        if (ev.targetNet->value != ev.newValue || ev.timestamp == 0) {
            ev.targetNet->value = ev.newValue;
            logger.log(ev.timestamp, ev.targetNet->name, ev.newValue);

            std::cout << "[T=" << ev.timestamp << "ns] Net '" 
                      << ev.targetNet->name << "' changed to " 
                      << (ev.newValue == LogicVal::ONE ? '1' : '0') << "\n";

            for (Gate* g : ev.targetNet->fanout) {
                LogicVal outVal = evaluate_gate(g, ev.targetNet);
                uint64_t delay = (g->type == GateType::DFF) ? 1 : 0;
                
                if (!g->outputs.empty()) {
                    eventQueue.push(Event(ev.timestamp + delay, g->outputs[0], outVal));
                }
            }
        }
    }

    std::cout << "\nSimulation Complete. Waveform written to output.vcd\n";
    return 0;
}