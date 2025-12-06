#include "Board/board.h"
#include "Board/move.h"
#include "Search/search.h"

#include <iostream>
#include <string>
#include <sstream>
#include <atomic>

std::atomic<bool> stopSearch{false};
Board currentBoard;   // Global board state stored between commands

//-------------------------------------------------------------
// Parse "position ..." command
//-------------------------------------------------------------
void setPosition(const std::string &line) {
    std::stringstream ss(line);
    std::string tok;

    ss >> tok; // "position"
    ss >> tok;

    if (tok == "startpos") {
        currentBoard = Board();  // Should initialize startpos
        if (ss >> tok && tok == "moves") {
            while (ss >> tok) {
                currentBoard.makeMove(stringToMove(tok, currentBoard));
            }
        }
    } else if (tok == "fen") {
        std::string fen, part;
        fen.clear();

        for (int i = 0; i < 6 && ss >> part; i++) {
            if (!fen.empty()) fen += " ";
            fen += part;
        }

        currentBoard = Board(fen);

        if (ss >> tok && tok == "moves") {
            while (ss >> tok) {
                currentBoard.makeMove(stringToMove(tok, currentBoard));
            }
        }
    }
}

//-------------------------------------------------------------
// Begin search (dummy for now)
//-------------------------------------------------------------
void startSearch(const std::string &goCmd) {
    stopSearch = false;

    long movetime = 0;
    long depth = 0;

    {
        std::stringstream ss(goCmd);
        std::string tok;
        ss >> tok; // "go"
        while (ss >> tok) {
            if (tok == "movetime") ss >> movetime;
            if (tok == "depth") ss >> depth;
        }
    }

    // Placeholder info line (GUI expects some output)
    std::cout << "info depth 1 time 0 nodes 1 score cp 0 pv e2e4\n" << std::flush;

    BestMove bm = selectMove(currentBoard, movetime);

    std::cout << "bestmove " << moveToString(bm.bestMove) << "\n" << std::flush;
}

//-------------------------------------------------------------
// UCI main loop
//-------------------------------------------------------------
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;

    while (std::getline(std::cin, line)) {

        if (line.empty()) continue;

        if (line == "uci") {
            std::cout << "id name ECanBot\n" << std::flush;
            std::cout << "id author ECanDo\n" << std::flush;

            // Future options:
            // std::cout << "option name Hash type spin default 16 min 1 max 4096\n";

            std::cout << "uciok\n" << std::flush;
        } else if (line == "isready") {
            std::cout << "readyok\n" << std::flush;
        } else if (line.rfind("setoption", 0) == 0) {
            // TODO: handle engine options
        } else if (line == "ucinewgame") {
            currentBoard = Board();
        } else if (line.rfind("position", 0) == 0) {
            setPosition(line);
        } else if (line.rfind("go", 0) == 0) {
            startSearch(line);
        } else if (line == "stop") {
            stopSearch = true;
        } else if (line == "quit") {
            break;
        }
    }

    return 0;
}
