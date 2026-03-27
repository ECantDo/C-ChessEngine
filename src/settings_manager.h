//
// Created by ecando on 3/20/26.
//

#ifndef CHESSENGINE_SETTINGS_MANAGER_H
#define CHESSENGINE_SETTINGS_MANAGER_H
#include <string>

struct EngineSettings {
	int hashSizeMB = 256;
	int threads = 1;
	int maxDepth = 128;
};

extern EngineSettings g_engineSettings;

void setOptionHandler(const std::string &line);

void printOptions();

#endif //CHESSENGINE_SETTINGS_MANAGER_H
