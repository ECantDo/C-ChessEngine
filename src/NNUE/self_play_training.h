//
// Created by ECanDo on 2026-01-06.
//

#ifndef CHESSENGINE_SELF_PLAY_TRAINING_H
#define CHESSENGINE_SELF_PLAY_TRAINING_H

void generateTrainingData(const char *outputFile, int numGames, int depth);

void generateSupervisedData(const std::string &cmd);

#endif //CHESSENGINE_SELF_PLAY_TRAINING_H
