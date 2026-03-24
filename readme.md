# Eeternal

![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)
![Protocol](https://img.shields.io/badge/Protocol-UCI-green)
![Status](https://img.shields.io/badge/Status-In%20Development-orange)
![Strength](https://img.shields.io/badge/Strength-2000%2B%20Lichess-yellow)

> A UCI-compatible chess engine built from scratch in C++

Eeternal is a personal project born out of a love for chess and low-level programming. Everything — from move generation
to evaluation — was written by hand, with no external chess libraries. An earlier attempt existed in Java, but this is
the real one.
Special thanks to the incredibly helpful people in the Stockfish Discord community — this project would have taken a lot
longer without them.

---

## Features

### Search

| Technique                          | Description                                                                                        |
|------------------------------------|----------------------------------------------------------------------------------------------------|
| Alpha-Beta                         | Core negamax search with pruning                                                                   |
| Aspiration Windows                 | Narrows the search window around the previous score to reduce search space                         |
| Futility Pruning                   | Skips moves near the leaves that have no realistic chance of raising alpha                         |
| Iterative Deepening                | Searches incrementally deeper each iteration, reusing results from shallower searches              |
| Killer Moves                       | Remembers quiet moves that caused cutoffs at each ply to try them earlier next time                |
| Late Move Reductions (LMR)         | Reduces depth for moves ordered later, which are unlikely to be best                               |
| Null Move Pruning                  | Skips a move to detect positions so good they cause a beta cutoff regardless                       |
| Quiescence Search                  | Extends search on captures to avoid the horizon effect and evaluate only quiet positions           |
| Razoring                           | Drops into quiescence search early when static eval is far below alpha at low depths               |
| Reverse Futility Pruning           | Prunes nodes where static eval exceeds beta by a margin, assuming the position is already too good |
| Search Extensions                  | Extends search depth in critical situations such as check or pawn promotions                       |
| Search-based Legal Move Generation | Move generation only generates pseudo-legal moves, and search is responsible for checking legality |

### Evaluation

| Technique | Description                                     |
|-----------|-------------------------------------------------|
| NNUE      | Efficiently Updatable Neural Network Evaluation |

#### Eeternal Version 2

768 → 64 Architecture

#### Eeternal Version 3

768 → 128 Architecture

### Move Generation

| Technique                   | Description                                                                                              |
|-----------------------------|----------------------------------------------------------------------------------------------------------|
| Magic Bitboards             | Fast sliding piece attack lookup using magic numbers                                                     |
| Legal Move Generation       | Handles pins, checks, en passant, and castling correctly                                                 |
| Pseudolegal Move Generation | Same as legal move generation, but doesn't worry about putting the king in check, or leaving it in check |

### Infrastructure

| Feature             | Description                                           |
|---------------------|-------------------------------------------------------|
| Transposition Table | Clustered hash table with depth-preferred replacement |
| Zobrist Hashing     | Incrementally updated position hashing                |
| UCI Protocol        | Compatible with any UCI-supporting chess GUI          |

---

## UCI

Eeternal communicates via the [Universal Chess Interface (UCI)](https://backscattering.de/chess/uci/) protocol, making
it compatible with popular GUIs like:

- [Arena](http://www.playwitharena.de/)
- [Cute Chess](https://cutechess.com/)
- [Lucas Chess](https://lucaschess.pythonanywhere.com/)
- [BankSia](https://banksiagui.com/)

### Additional Supported Commands

Commands that are supported by my engine, but are not a typical UCI command.

| UCI Command | Description                                                                  |
|-------------|------------------------------------------------------------------------------|
| d           | For "display", prints out the current board state as an ASCII representation |
| eval        | Gives the current board state evaluation; also gives the FEN                 |

### Supported settings

From the command `setoption` command from the standard UCI interface.
Typically the command looks like `setoption name <Option Name> value <Option Value>`

| Option Name | Type   | Default Value | Value Ranges | Description                                                                            |
|-------------|--------|---------------|--------------|----------------------------------------------------------------------------------------|
| Hash        | spin   | 256           | 4 - 4096     | The size of the Transposition Table, in MB                                             | 
| Threads     | spin   | 1             | 1 - 1        | The number of threads to use (backend support, but not implemented properly currently) |
| EvalFile    | string | quantized.bin | N/A          | The file to use for the NNUE                                                           |

---

## How It Works — A Brief Overview

Chess engines work by searching through millions of possible future positions and evaluating who is winning. Eeternal
does this using **Alpha-Beta search**, a smarter version of the brute-force minimax algorithm that prunes branches that
can't possibly affect the result.

On top of that, several heuristics help it search *smarter* rather than just deeper:

- **Null move pruning** assumes that if skipping your turn still leaves you in a great position, the position is
  probably a cutoff and can be pruned early.
- **LMR** bets that moves ordered later in the list are less likely to be good, and searches them at reduced depth.
- **Quiescence search** prevents the engine from stopping mid-capture sequence, which would give it a wildly inaccurate
  picture of the position.

Evaluation is handled by a small **NNUE** — a neural network trained on millions of positions that can be updated
incrementally as pieces move, keeping it extremely fast.
---

## Version History

| Version     | Highlights                                                                                    | \*ELO : Blitz |
|-------------|-----------------------------------------------------------------------------------------------|---------------|
| Eeternal-V3 | Better NNUE integration, 128 HL neurons                                                       | _not tested_  |
| Eeternal-V2 | NNUE integration, 64 HL neurons; all above features implemented; All mostly working (untuned) | 2100          |
| Eeternal-V1 | Pure HCE (Hand Crafted Evaluation); all above features implemented; with some of them broken  | 1800          |
| Java Bot    | Pure HCE, and very little of the above features                                               | 1500          |

\*Approximations based on LiChess games

## Acknowledgements

- The [**Stockfish Discord**](https://discord.gg/GWDRS3kU6R) community — for answering endless questions patiently
- [Chessprogramming Wiki](https://www.chessprogramming.org/) — an invaluable reference
- [bullet](https://github.com/jw1912/bullet) — NNUE training framework used to train Eeternal's networks

---

*Eeternal is a personal project and is not affiliated with Stockfish or any other engine.*