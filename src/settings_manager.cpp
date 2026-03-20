//
// Created by ecando on 3/20/26.
//

#include "settings_manager.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "NNUE/nnue_eval.h"
#include "Search/search.h"
#include "Search/transposition_table.h"

void printOptions() {
	std::cout
			<< "option name Hash type spin default 256 min 4 max 4096" << std::endl
			<< "option name Threads type spin default 1 max 1" << std::endl
			<< "option name EvalFile type string default quantised.bin" << std::endl;
}

void setOptionHandler(const std::string &line) {
	std::stringstream ss(line);
	std::string tok, name, value;
	ss >> tok; // "setoption"
	ss >> tok; // "name"
	ss >> name;
	ss >> tok; // "value"
	ss >> value;

	if (name == "Hash") {
		int mb = std::stoi(value);
		mb = std::clamp(mb, 4, 4096);
		g_engineSettings.hashSizeMB = mb;
		globalTT.~TranspositionTable();
		new(&globalTT) TranspositionTable(mb);
		std::cerr << "Hash size set to " << mb << " MB" << std::endl;
		return;
	}

	if (name == "EvalFile") {
		stopSearch = true;
		try_init_nnue(value);
		return;
	}

	if (name == "Threads") {
		int threads = std::stoi(value);
		threads = std::clamp(threads, 1, 1);
		g_engineSettings.threads = threads;
		std::cerr << "Number of threads set to " << threads << std::endl;
	}
}
