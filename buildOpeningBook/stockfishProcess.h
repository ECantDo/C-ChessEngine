//
// Created by ECanDo on 2025-12-10.
//

#ifndef CHESSENGINE_STOCKFISHPROCESS_H
#define CHESSENGINE_STOCKFISHPROCESS_H

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

class StockfishProcess {
private:
    HANDLE hChildStdinWr = NULL;
    HANDLE hChildStdoutRd = NULL;
    PROCESS_INFORMATION piProcInfo{};
    std::string lineBuffer;  // Accumulate partial lines

    // Non-blocking read with timeout
    bool readWithTimeout(std::string &output, DWORD timeoutMs = 100) {
        DWORD startTime = GetTickCount();
        char buffer[4096];

        while (GetTickCount() - startTime < timeoutMs) {
            DWORD available = 0;
            if (!PeekNamedPipe(hChildStdoutRd, NULL, 0, NULL, &available, NULL)) {
                return false;
            }

            if (available > 0) {
                DWORD bytesRead = 0;
                DWORD toRead = (available < sizeof(buffer) - 1) ? available : sizeof(buffer) - 1;

                if (ReadFile(hChildStdoutRd, buffer, toRead, &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    output.append(buffer, bytesRead);
                    return true;
                }
            }

            Sleep(10);  // Small sleep to avoid busy-waiting
        }

        return false;
    }

    // Read until we get a specific line (like "uciok" or "readyok")
    bool waitForResponse(const std::string &expectedResponse, DWORD timeoutMs = 5000) {
        DWORD startTime = GetTickCount();
        std::string accumulated;

        while (GetTickCount() - startTime < timeoutMs) {
            std::string chunk;
            if (readWithTimeout(chunk, 100)) {
                accumulated += chunk;

                // Check if we got the expected response
                if (accumulated.find(expectedResponse) != std::string::npos) {
                    return true;
                }
            }
        }

        std::cerr << "Timeout waiting for: " << expectedResponse << std::endl;
        return false;
    }

public:
    bool start(const std::string &path, bool verbose = false) {
        SECURITY_ATTRIBUTES saAttr{};
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStdinRd = NULL;
        HANDLE hChildStdoutWr = NULL;

        // Create pipes
        if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
            std::cerr << "CreatePipe failed for stdout" << std::endl;
            return false;
        }

        if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
            std::cerr << "CreatePipe failed for stdin" << std::endl;
            return false;
        }

        // Ensure parent handles are not inherited
        SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA siStartInfo{};
        siStartInfo.cb = sizeof(STARTUPINFOA);
        siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        siStartInfo.wShowWindow = SW_HIDE;  // Hide console window
        siStartInfo.hStdInput = hChildStdinRd;
        siStartInfo.hStdOutput = hChildStdoutWr;
        siStartInfo.hStdError = hChildStdoutWr;

        // Make a mutable copy of the path
        std::string command = path;

        BOOL success = CreateProcessA(
                NULL,
                &command[0],  // Must be mutable
                NULL,
                NULL,
                TRUE,         // Inherit handles
                CREATE_NO_WINDOW,
                NULL,
                NULL,
                &siStartInfo,
                &piProcInfo
        );

        // Close child's ends of pipes (parent doesn't need them)
        CloseHandle(hChildStdinRd);
        CloseHandle(hChildStdoutWr);

        if (!success) {
            std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
            return false;
        }

        if (verbose) std::cout << "Stockfish process started. Initializing UCI..." << std::endl;

        // Initialize UCI protocol
        send("uci");
        if (!waitForResponse("uciok", 5000)) {
            std::cerr << "Failed to initialize UCI" << std::endl;
            return false;
        }

        if (verbose) std::cout << "UCI initialized successfully." << std::endl;

        // Wait for engine to be ready
        send("isready");
        if (!waitForResponse("readyok", 5000)) {
            std::cerr << "Engine not ready" << std::endl;
            return false;
        }

        if (verbose) std::cout << "Stockfish ready." << std::endl;
        return true;
    }

    void send(const std::string &cmd) {
        std::string msg = cmd + "\n";
        DWORD written;
        WriteFile(hChildStdinWr, msg.c_str(), (DWORD) msg.size(), &written, NULL);
        FlushFileBuffers(hChildStdinWr);  // Ensure data is sent immediately
    }

    // Read complete lines (split by \n)
    std::vector<std::string> readLines(DWORD timeoutMs = 100) {
        std::vector<std::string> lines;
        std::string chunk;

        if (readWithTimeout(chunk, timeoutMs)) {
            lineBuffer += chunk;
        }

        // Split by newlines
        size_t pos;
        while ((pos = lineBuffer.find('\n')) != std::string::npos) {
            std::string line = lineBuffer.substr(0, pos);
            lineBuffer.erase(0, pos + 1);

            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty()) {
                lines.push_back(line);
            }
        }

        return lines;
    }

    void stop() {
        if (piProcInfo.hProcess) {
            send("quit");
            WaitForSingleObject(piProcInfo.hProcess, 2000);

            // Force kill if still running
            DWORD exitCode;
            if (GetExitCodeProcess(piProcInfo.hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
                TerminateProcess(piProcInfo.hProcess, 1);
            }

            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);
        }

        if (hChildStdinWr) CloseHandle(hChildStdinWr);
        if (hChildStdoutRd) CloseHandle(hChildStdoutRd);

        hChildStdinWr = NULL;
        hChildStdoutRd = NULL;
    }

    ~StockfishProcess() {
        stop();
    }
};

// Helper function to get best move
inline std::string getBestmove(StockfishProcess &sf, const std::string &fen, int depth, bool verbose = false) {
    sf.send("position fen " + fen);
    sf.send("go depth " + std::to_string(depth));

    if (verbose) std::cout << "Calculating best move (depth " << depth << ")..." << std::endl;

    while (true) {
        auto lines = sf.readLines(100);

        for (const auto &line: lines) {
            // Debug output
            if (verbose && line.find("info depth") == 0) {
                std::cout << "  " << line << std::endl;
            }

            if (line.find("bestmove") == 0) {
                // Extract move from "bestmove e2e4" or "bestmove e2e4 ponder e7e5"
                size_t spacePos = line.find(' ', 9);
                if (spacePos != std::string::npos) {
                    return line.substr(9, spacePos - 9);
                }
                return line.substr(9);
            }
        }

        Sleep(50);  // Don't busy-wait
    }
}

struct PVLine {
    int multipv;
    int scoreCp;
    std::string move;
};

// Helper function to get top moves with MultiPV
inline std::vector<PVLine>
getTopMoves(StockfishProcess &sf, const std::string &fen, int depth, int multipvCount, bool verbose = false) {
    // Set MultiPV option
    sf.send("setoption name MultiPV value " + std::to_string(multipvCount));
    sf.send("isready");

    // Wait for readyok
    while (true) {
        auto lines = sf.readLines(100);
        bool ready = false;
        for (const auto &line: lines) {
            if (line == "readyok") {
                ready = true;
                break;
            }
        }
        if (ready) break;
        Sleep(10);
    }

    sf.send("position fen " + fen);
    sf.send("go depth " + std::to_string(depth));

    if (verbose) std::cout << "Calculating top " << multipvCount << " moves (depth " << depth << ")..." << std::endl;

    std::vector<PVLine> results;
    results.resize(multipvCount);  // Pre-allocate

    for (auto &pv: results) {
        pv.multipv = 0;
        pv.scoreCp = 0;
    }

    while (true) {
        auto lines = sf.readLines(100);

        for (const auto &line: lines) {
            // Parse info lines with multipv
            if (line.find("info") == 0 && line.find("multipv") != std::string::npos) {
                int multipv = 0, scoreCp = 0, depth_curr = 0;
                char moveStr[256] = "";

                // Parse the line - format: "info depth X ... multipv Y score cp Z ... pv MOVE ..."
                std::istringstream iss(line);
                std::string token;
                bool foundMultipv = false, foundScore = false, foundPv = false;

                while (iss >> token) {
                    if (token == "depth") {
                        iss >> depth_curr;
                    } else if (token == "multipv") {
                        iss >> multipv;
                        foundMultipv = true;
                    } else if (token == "score") {
                        std::string scoreType;
                        iss >> scoreType;
                        if (scoreType == "cp") {
                            iss >> scoreCp;
                            foundScore = true;
                        } else if (scoreType == "mate") {
                            int mateIn;
                            iss >> mateIn;
                            // Convert mate score to centipawns (high value)
                            scoreCp = (mateIn > 0) ? 10000 - mateIn : -10000 - mateIn;
                            foundScore = true;
                        }
                    } else if (token == "pv") {
                        iss >> moveStr;
                        foundPv = true;
                        break;
                    }
                }

                // Only update if we're at the target depth and have all required info
                if (foundMultipv && foundScore && foundPv && depth_curr == depth && multipv > 0 &&
                    multipv <= multipvCount) {
                    results[multipv - 1].multipv = multipv;
                    results[multipv - 1].scoreCp = scoreCp;
                    results[multipv - 1].move = moveStr;

                    if (verbose)
                        std::cout << "  PV " << multipv << ": " << moveStr << " (cp: " << scoreCp << ")" << std::endl;
                }
            }

            if (line.find("bestmove") == 0) {
                if (verbose) std::cout << "Analysis complete." << std::endl;

                // Filter out uninitialized entries
                std::vector<PVLine> validResults;
                for (const auto &pv: results) {
                    if (pv.multipv > 0 && !pv.move.empty()) {
                        validResults.push_back(pv);
                    }
                }

                return validResults;
            }
        }

        Sleep(50);
    }
}

#endif //CHESSENGINE_STOCKFISHPROCESS_H