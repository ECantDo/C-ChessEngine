#include "Board/board.h"
#include "Board/move.h"
#include "Search/search.h"
#include "Board/zobrist_hash.h"
#include "Evaluation/evaluation.h"
#include "../tests/MoveGeneration/test_move_generation.h"

#include <iostream>
#include <string>
#include <sstream>
#include <atomic>
#include <chrono>

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

        currentBoard.gameHistory.clear();

        if (ss >> tok && tok == "moves") {
            while (ss >> tok) {
                currentBoard.gameHistory.push_back(currentBoard.zobristHash);
                currentBoard.makeMove(stringToMove(tok, currentBoard));
            }
        }
    }
}

//-------------------------------------------------------------
//
//-------------------------------------------------------------
void startSearch(const std::string &goCmd) {
    stopSearch = false;

    bool isPeft = false;

    long movetime = -1;     // exact time to use (ms)
    long depth = -1;     // depth limit
    long nodes = -1;     // node limit

    long wtime = -1, btime = -1;   // remaining time (ms)
    long winc = 0, binc = 0;    // increments (ms)


    std::stringstream ss(goCmd);
    std::string tok;
    ss >> tok; // "go"

    while (ss >> tok) {
        if (tok == "movetime") ss >> movetime;
        else if (tok == "depth") ss >> depth;
        else if (tok == "nodes") ss >> nodes;

        else if (tok == "wtime") ss >> wtime;
        else if (tok == "btime") ss >> btime;
        else if (tok == "winc") ss >> winc;
        else if (tok == "binc") ss >> binc;
        else if (tok == "perft") {
            ss >> depth;
            isPeft = true;
        }
    }

    //---------------------------------------------------------
    // If no limits were explicitly given, derive a time limit
    //---------------------------------------------------------
    long timeLimit = 0;

    if (movetime > 0) {
        timeLimit = movetime;
        depth = 30; // No need in going any higher than 30 tbh
    } else if (wtime >= 0 && btime >= 0) {
        // Allocate time based on whose move it is
        long remaining = (currentBoard.turn == 1 ? wtime : btime);
        long increment = (currentBoard.turn == 1 ? winc : binc);

        // Basic time allocation: use 1/30 of remaining + 80% of increment (allow for some overhead)
        timeLimit = remaining / 30 + (long) (increment * 0.8);

        // Safety clamp: never more than 80% of remaining
        if (timeLimit > remaining * 4 / 5)
            timeLimit = remaining * 4 / 5;

        depth = 30;
    } else {
        // No time controls given — default to depth search
        if (depth <= 0)
            depth = 6; // fallback
    }

    if (timeLimit > 50) {
        timeLimit -= 20; // Allow for 20ms of outputting time
    }

    //---------------------------------------------------------
    // Now you have:
    //   timeLimit  (ms)  — guaranteed non-negative
    //   depth      (ply) — maybe -1 if no depth limit
    //   nodes      (cnt) — maybe -1 if no node limit
    //---------------------------------------------------------

    if (!isPeft) {
        // Pass depth or time-based stopping to your search
        BestMove bm = selectMove(currentBoard, depth, timeLimit);
        if (bm.bestMove == 0) {
            std::vector<Move> moves;
            generateLegalMoves(currentBoard, moves);
            if (!moves.empty()) {
                bm.bestMove = moves[0];
                std::cerr << "WARNING: Search returned null move, using fallback: "
                          << moveToString(bm.bestMove) << std::endl;
            } else {
                std::cout << "bestmove (none)\n" << std::flush;
                return;  /* Early return */
            }
        }
        std::cout << "bestmove " << moveToString(bm.bestMove) << '\n' << std::flush;
    } else {
        auto start = std::chrono::high_resolution_clock::now();
        perftDivide(currentBoard, depth);        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Took " << duration.count() << " ms\n";
    }
}

//-------------------------------------------------------------
// UCI main loop
//-------------------------------------------------------------
int main() {
    Zobrist::init();
    globalTT.clear();

    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;

    while (std::getline(std::cin, line)) {

        if (line.empty()) continue;

        if (line.rfind("go", 0) == 0) { // Keep at the top, the most common input
            startSearch(line);
        } else if (line == "uci") {
            std::cout << "id name ECanBot-V9.1_QuiescenceSearch\n" << std::flush;
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
            globalTT.clear();
        } else if (line.rfind("position", 0) == 0) {
            try {
                setPosition(line);
            } catch (std::invalid_argument &e) {
                std::cerr << e.what() << std::endl;
            }
        } else if (line == "stop") {
            stopSearch = true;
        } else if (line == "quit") {
            break;
        } else if (line == "d") {
            currentBoard.printBoard();
            std::cout << currentBoard.generateFen() << std::endl << std::flush;
        } else if (line == "eval") {
            std::cout << "Evaluation: " << evaluateBoard(currentBoard) << std::endl;
            std::cout << "FEN: " << currentBoard.generateFen() << std::endl << std::flush;
        } else if (line == "debug") {
            rootDebugAlphaBeta(currentBoard, 6);
        }
    }

    return 0;
}
