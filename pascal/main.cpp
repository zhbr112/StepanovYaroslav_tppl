#include <iostream>
#include <fstream>
#include <sstream>
#include "interpreter.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_file>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filepath << "'" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    Pascal::Interpreter interp;
    try {
        auto variables = interp.interpret(code);
        
        if (variables.empty()) {
            std::cout << "No variables defined." << std::endl;
        } else {
            std::cout << "Variables:" << std::endl;
            for (const auto& pair : variables) {
                std::cout << pair.first << " = " << pair.second << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}