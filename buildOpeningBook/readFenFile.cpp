//
// Created by ECanDo on 2025-12-10.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#include "readFenFile.h"

// Trim whitespace from both ends
std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// Reads a FEN file and stores FEN strings
// Ignores lines that start with '#' or '//'
std::vector<std::string> readFENFile(const std::string &filename) {
    std::vector<std::string> fens;
    std::ifstream file(filename, std::ios::in);

    if (!file.is_open()) {
        // Try with different path interpretations
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        std::cerr << "Make sure the file exists and the path is correct." << std::endl;
        std::cerr << "Tried paths:" << std::endl;
        std::cerr << "  - " << filename << std::endl;

        // Try without ./ prefix
        if (filename.find("./") == 0) {
            std::string altPath = filename.substr(2);
            std::cerr << "  - " << altPath << std::endl;
            file.open(altPath, std::ios::in);
        }

        if (!file.is_open()) {
            return fens;
        } else {
            std::cerr << "Successfully opened with alternative path." << std::endl;
        }
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        // Trim whitespace
        line = trim(line);

        // Skip empty lines
        if (line.empty()) continue;

        // Skip comments starting with # or //
        if (line[0] == '#') continue;
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/') continue;

        // Basic FEN validation - should have at least spaces and pieces
        if (line.find(' ') == std::string::npos) {
            std::cerr << "Warning: Line " << lineNumber << " doesn't look like a FEN (no spaces): "
                      << line << std::endl;
            continue;
        }

        // Line is a FEN
        fens.push_back(line);
    }

    file.close();

    std::cout << "Successfully loaded " << fens.size() << " FEN positions from "
              << filename << std::endl;

    return fens;
}