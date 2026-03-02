//
// Created by ECanDo on 2026-01-06.
//

#ifndef CHESSENGINE_SELF_PLAY_TRAINING_H
#define CHESSENGINE_SELF_PLAY_TRAINING_H

// Generates self-play training data in Bullet trainer format:
//   fen | score | result
// where both score and result are WHITE-relative.
//
// outputFile  : path to write to (opened in append mode)
// numGames    : number of self-play games to run
// searchTimeMs: milliseconds per move for each search call
void generateTrainingData(const char *outputFile, int numGames, int searchNodes);

#endif //CHESSENGINE_SELF_PLAY_TRAINING_H