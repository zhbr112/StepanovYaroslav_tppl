#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "CowInterpreter.h"

void printUsage(const char* progName) {
    std::cout << "COW Language Interpreter\n";
    std::cout << "------------------------\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " <path_to_cow_file>\n\n";
    std::cout << "Description:\n";
    std::cout << "  This utility executes programs written in the COW esoteric programming language.\n";
    std::cout << "  The file extension is typically .cow, but any text file is accepted.\n\n";
    std::cout << "Options:\n";
    std::cout << "  <path_to_cow_file>  Absolute or relative path to the source code file.\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << progName << " hello_world.cow\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string filePath = argv[1];

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filePath << "'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();
    file.close();

    if (sourceCode.empty()) {
        std::cerr << "Warning: The source file is empty.\n";
        return 0;
    }

    try {
        CowInterpreter interpreter; 
        interpreter.run(sourceCode);
        
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\nruntime error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
