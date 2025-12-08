#include "Board/board.h"
#include "Board/move.h"
#include "Search/search.h"
#include "Board/zobrist_hash.h"

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

        if (ss >> tok && tok == "moves") {
            while (ss >> tok) {
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

    long movetime = -1;     // exact time to use (ms)
    long depth = -1;     // depth limit
    long nodes = -1;     // node limit

    long wtime = -1, btime = -1;   // remaining time (ms)
    long winc = 0, binc = 0;    // increments (ms)

    {
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

        // Ensure minimum thinking time
        if (timeLimit < 20) timeLimit = 20;
    } else {
        // No time controls given — default to depth search
        if (depth <= 0)
            depth = 6; // fallback
    }

    if (timeLimit > 50) {
        timeLimit -= 20; // Allow for 20ms of outputting time
    }

    if (timeLimit <= 0) {
        timeLimit = 30;  // 30 ms fallback
    }

    //---------------------------------------------------------
    // Now you have:
    //   timeLimit  (ms)  — guaranteed non-negative
    //   depth      (ply) — maybe -1 if no depth limit
    //   nodes      (cnt) — maybe -1 if no node limit
    //---------------------------------------------------------

    auto start = std::chrono::steady_clock::now();

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

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    //---------------------------------------------------------
    // Output info + bestmove (same as before)
    //---------------------------------------------------------

    std::string score;
    if (abs(bm.score) >= MATE_SCORE - 1000) {
        int matePly = MATE_SCORE - abs(bm.score);
        int mateMoves = (matePly + 1) / 2;
        score = bm.score > 0 ?
                std::format(" score mate {}", mateMoves) :
                std::format(" score mate -{}", mateMoves);
    } else {
        score = std::format(" score cp {}", bm.score);
    }

    std::cout << "info depth " << bm.depth
              << " time " << elapsed
              << " nodes " << bm.nodes
              << score
              << " pv ";
    for (Move &m: bm.pv) {
        std::cout << moveToString(m) << ' ';
    }
    std::cout << std::endl << std::flush;

    std::cout << "bestmove " << moveToString(bm.bestMove) << '\n' << std::flush;
}

//-------------------------------------------------------------
// UCI main loop
//-------------------------------------------------------------
int main() {
    Zobrist::init();
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;

    while (std::getline(std::cin, line)) {

        if (line.empty()) continue;

        if (line.rfind("go", 0) == 0) { // Keep at the top, the most common input
            startSearch(line);
        } else if (line == "uci") {
            std::cout << "id name ECanBot-V6.2\n" << std::flush;
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
        } else if (line == "stop") {
            stopSearch = true;
        } else if (line == "quit") {
            break;
        } else if (line == "d") {
            currentBoard.printBoard();
            std::cout << std::flush;
        } else if (line == "eval") {
            std::cout << "Evaluation: " << evaluate(currentBoard) << std::endl;
            std::cout << "FEN: " << currentBoard.generateFen() << std::endl << std::flush;
        }
    }

    return 0;
}
