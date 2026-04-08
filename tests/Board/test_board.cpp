//
// Created by ECanDo on 2025-08-23.
//
#include "Board/board.h"
#include "test_board.h"
#include <iostream>
#include <vector>
#include <string>

struct IndexTestChar {
	char file;
	char rank;
	int expected;
};

struct IndexTestInt {
	int file;
	int rank;
	int expected;
};

struct PositionTest {
	int index;
	std::string expected;
};

void testRepetitionCheck(TestResult &results) {
	std::string startingFen = "2r2k1r/pNPp1p2/1p5p/5Qp1/3q3P/3P2P1/PP4B1/1K1RR3 b - - 5 23";
	Board board = Board(startingFen);
	const std::vector<std::string> moves = {
		"d4d5", "e1e2", "d5d4", "e2e1",
		"d4d5", "e1e2", "d5d4", "e2e1",
	};

	for (const std::string &moveStr: moves) {
		const Move move = stringToMove(moveStr, board);
		board.makeMove(move);
	}

	constexpr std::vector<uint64_t> searchPath;

	if (!board.isRepetitionInSearch(searchPath)) {
		results.fail++;
		std::cout << "Did not find repetition from position: " << std::endl << startingFen << std::endl;
	} else {
		results.pass++;
	}
}

void testBoard() {
	TestResult testResults = {0, 0};

	// Test getting the index: a1 -> 0; (0, 0) -> 0; h8 -> 63; (7, 7) -> 63
	testGetBoardIndexLetters(testResults);
	testGetBoardIndexNumbers(testResults);

	// Test going the other way
	testGetBoardPosition(testResults);

	// Test Board gen from FEN
	testConvertFenString(testResults);


	std::cout << "\nSummary: " << testResults.pass << "/" << (testResults.pass + testResults.fail)
			<< " tests pass for `Board/board`.\n";
}

void testConvertFenString(TestResult &results) {
	std::vector<std::string> fenTests = {
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
		"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
		"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",

		"7k/3N2qp/b5r1/2p1Q1N1/Pp4PK/7P/1P3p2/6r1 w - - 7 4",
	};

	Board board = Board();
	std::string resultingFEN;

	for (auto &t: fenTests) {
		board.loadFenPosition(t);

		resultingFEN = board.generateFen();

		if (resultingFEN != t) {
			std::cout << "[FAIL] Board.generateFen() does not match provided FEN;\nInput: " << t << "\nOutput: "
					<< resultingFEN << "\n\n";
			board.printBoard();
			results.fail++;
			continue;
		}

		results.pass++;
	}
}

void testGetBoardIndexLetters(TestResult &results) {
	std::vector<IndexTestChar> charIndexTests = {
		// Pass Cases
		{'a', '1', 0}, /* rank 0, file 0 → 0*8+0 = 0 */
		{'a', '2', 8}, /* rank 1, file 0 → 1*8+0 = 8 */
		{'a', '3', 16}, /* rank 2, file 0 → 2*8+0 = 16 */
		{'a', '8', 56}, /* rank 7, file 0 → 7*8+0 = 56 */
		{'b', '1', 1}, /* rank 0, file 1 → 0*8+1 = 1 */
		{'b', '2', 9}, /* rank 1, file 1 → 1*8+1 = 9 */
		{'h', '1', 7}, /* rank 0, file 7 → 0*8+7 = 7 */
		{'h', '8', 63}, /* rank 7, file 7 → 7*8+7 = 63 */

		/* Fail cases */
		{'a', '0', -1},
		{'`', '1', -1},
		{'a', '9', -1},
		{'i', '1', -1}
	};

	for (auto &t: charIndexTests) {
		int got = getBoardIndex(t.file, t.rank);
		if (got == t.expected) {
			results.pass++;
		} else {
			results.fail++;
			std::cout << "[FAIL] Board.getBoardIndex('" << t.file << "', '" << t.rank << "') expected " << t.expected
					<< ", got " << got << "\n";
		}
	}
}

void testGetBoardIndexNumbers(TestResult &results) {
	std::vector<IndexTestInt> intIndexTests = {
		{0, 0, 0}, /* rank*8+file = 0*8+0 = 0 */
		{0, 1, 8}, /* rank*8+file = 1*8+0 = 8 */
		{1, 0, 1}, /* rank*8+file = 0*8+1 = 1 */
		{1, 1, 9}, /* rank*8+file = 1*8+1 = 9 */
		{7, 7, 63}, /* rank*8+file = 7*8+7 = 63 */

		/* Fail cases */
		{-1, 0, -1},
		{0, -1, -1},
		{8, 0, -1},
		{0, 8, -1}
	};

	for (auto &t: intIndexTests) {
		int got = getBoardIndex(t.file, t.rank);
		if (got == t.expected) {
			results.pass++;
		} else {
			results.fail++;
			std::cout << "[FAIL] Board.getBoardIndex(" << t.file << ", " << t.rank << ") expected " << t.expected
					<< ", got " << got << "\n";
		}
	}
}

void testGetBoardPosition(TestResult &results) {
	std::vector<PositionTest> positionTests = {
		// Pass Cases
		{0, "a1"},
		{1, "b1"},
		{2, "c1"},
		{3, "d1"},
		{4, "e1"},
		{5, "f1"},
		{6, "g1"},
		{7, "h1"},
		{8, "a2"},
		{63, "h8"},

		// Fail Cases
		{-1, ""},
		{64, ""}
	};

	for (auto &t: positionTests) {
		std::string got = getBoardPosition(t.index);
		if (got == t.expected) {
			results.pass++;
		} else {
			results.fail++;
			std::cout << "[FAIL] Board.getPosition(" << t.index << ") expected " << t.expected
					<< ", got " << got << "\n";
		}
	}
}
