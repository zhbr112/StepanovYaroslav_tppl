#define COW_INTERPRETER_H

#include <vector>
#include <string>
#include <map>
#include <stack>
#include <iostream>
#include <stdexcept>

class CowInterpreter {
public:
    CowInterpreter(std::istream& input = std::cin, std::ostream& output = std::cout)
        : in(input), out(output), ptr(0), reg(0), reg_loaded(false) {
        memory.push_back(0);
    }

    void run(const std::string& source) {
        reset();
        parse(source);
        
        for (size_t pc = 0; pc < instructions.size(); ++pc) {
            execute(instructions[pc], pc);
        }
    }

    int getValueAt(size_t index) const {
        if (index < memory.size()) return memory[index];
        return 0;
    }

    size_t getPointer() const { return ptr; }
    int getRegister() const { return reg; }
    bool isRegisterLoaded() const { return reg_loaded; }

private:
    enum OpCode {
        moo = 0, mOo, moO, mOO, Moo, MOo, MoO, MOO, OOO, MMM, OOM, oom, NOP
    };

    std::vector<int> memory;
    size_t ptr;
    int reg;
    bool reg_loaded;
    
    std::istream& in;
    std::ostream& out;

    std::vector<OpCode> instructions;
    std::map<size_t, size_t> jump_table;

    void reset() {
        memory.clear();
        memory.push_back(0);
        ptr = 0;
        reg = 0;
        reg_loaded = false;
        instructions.clear();
        jump_table.clear();
    }

    void parse(const std::string& source) {
        std::stack<size_t> loops;
        
        for (size_t i = 0; i + 2 < source.length(); ) {
            std::string sub = source.substr(i, 3);
            OpCode op = NOP;

            if (sub == "moo") op = moo;
            else if (sub == "mOo") op = mOo;
            else if (sub == "moO") op = moO;
            else if (sub == "mOO") op = mOO;
            else if (sub == "Moo") op = Moo;
            else if (sub == "MOo") op = MOo;
            else if (sub == "MoO") op = MoO;
            else if (sub == "MOO") op = MOO;
            else if (sub == "OOO") op = OOO;
            else if (sub == "MMM") op = MMM;
            else if (sub == "OOM") op = OOM;
            else if (sub == "oom") op = oom;
            else {
                i++; continue;
            }

            if (op == MOO) {
                loops.push(instructions.size());
            } else if (op == moo) {
                if (loops.empty()) throw std::runtime_error("Unmatched moo");
                size_t start = loops.top();
                loops.pop();
                size_t end = instructions.size();
                jump_table[start] = end;
                jump_table[end] = start;
            }

            instructions.push_back(op);
            i += 3;
        }

        if (!loops.empty()) throw std::runtime_error("Unmatched MOO");
    }

    void ensure_memory() {
        if (ptr >= memory.size()) {
            memory.resize(ptr + 1, 0);
        }
    }

    void execute(OpCode op, size_t& pc) {
        ensure_memory();
        
        switch (op) {
            case MOO:
                if (memory[ptr] == 0) {
                    pc = jump_table[pc]; 
                }
                break;
            case moo:
                pc = jump_table[pc] - 1;
                break;        
            case mOo:
                if (ptr == 0) throw std::runtime_error("Memory Underflow");
                ptr--;
                break;
            case moO:
                ptr++;
                ensure_memory();
                break;
            case mOO:
                {
                    int code = memory[ptr];
                    if (code == 0 || code == 7) throw std::runtime_error("Cannot exec loop via mOO");
                    if (code == 3) throw std::runtime_error("Recursion forbidden");
                    if (code >= 0 && code <= 11) {
                        size_t dummy_pc = 0;
                        execute(static_cast<OpCode>(code), dummy_pc);
                    }
                }
                break;
            case Moo:
                if (memory[ptr] == 0) {
                    char c = 0;
                    if (in.get(c)) memory[ptr] = static_cast<unsigned char>(c);
                    else memory[ptr] = 0;
                } else {
                    out << static_cast<char>(memory[ptr]);
                }
                break;
            case MOo:
                memory[ptr]--;
                break;
            case MoO:
                memory[ptr]++;
                break;
            case OOO:
                memory[ptr] = 0;
                break;
            case MMM:
                if (reg_loaded) {
                    memory[ptr] = reg;
                    reg = 0;
                    reg_loaded = false;
                } else {
                    reg = memory[ptr];
                    reg_loaded = true;
                }
                break;
            case OOM:
                out << memory[ptr];
                break;
            case oom:
                {
                    int val = 0;
                    if (in >> val) memory[ptr] = val;
                }
                break;
            default:   		// LCOV_EXCL_LINE
                break; 		// LCOV_EXCL_LINE
        }
    }
};
